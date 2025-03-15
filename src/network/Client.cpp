#include "Client.h"
#include "../Game.h"
#include "../network/NetworkManager.h"
#include "../utils/MessageHandler.h"
#include "../states/PlayingState.h"
#include <iostream>


ClientNetwork::ClientNetwork(Game* game, PlayerManager* manager)
    : game(game), playerManager(manager), lastSendTime(std::chrono::steady_clock::now()) {
    hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
    
    // Initialize validation-related variables
    m_lastValidationTime = std::chrono::steady_clock::now();
    m_validationRequestTimer = 0.5f; // Schedule validation soon after connection
    m_periodicValidationTimer = 30.0f;
    
    // Immediately remove any potential ghost enemies at origin
    PlayingState* playingState = GetPlayingState(game);
    if (playingState) {
        EnemyManager* enemyManager = playingState->GetEnemyManager();
        if (enemyManager) {
            // Ensure no enemies exist at start of game for client
            std::cout << "[CLIENT] Initializing Client Network, ensuring clean enemy state\n";
            enemyManager->ClearAllEnemies();
        }
    }
}

ClientNetwork::~ClientNetwork() {}

void ClientNetwork::ProcessMessage(const std::string& msg, CSteamID sender) {
    ParsedMessage parsed = MessageHandler::ParseMessage(msg);
    
    // Handle chunk messages (which are processed internally by ParseMessage)
    // If this is a chunk message that's not yet complete, just return
    if (parsed.type == MessageType::ChunkStart || 
        parsed.type == MessageType::ChunkPart) {
        return;
    }
    
    switch (parsed.type) {
        case MessageType::Chat:
            ProcessChatMessage(parsed);
            break;
        case MessageType::Connection:
            ProcessConnectionMessage(parsed);
            break;
        case MessageType::ReadyStatus:
            ProcessReadyStatusMessage(parsed);
            break;
        case MessageType::Movement:
            ProcessMovementMessage(parsed);
            break;
        case MessageType::Bullet:
            ProcessBulletMessage(parsed);
            break;
        case MessageType::PlayerDeath:
            ProcessPlayerDeathMessage(parsed);
            break;
        case MessageType::PlayerRespawn:
            ProcessPlayerRespawnMessage(parsed);
            break;
        case MessageType::TriangleWaveStart:
            ProcessTriangleWaveStartMessage(parsed);
            break;
        case MessageType::EnemyPositions:
            ProcessEnemyPositionsMessage(parsed);
            break;
        case MessageType::EnemyValidation:
            ProcessEnemyValidationMessage(parsed);
            break;
        case MessageType::StartGame:
            std::cout << "[CLIENT] Received start game message, changing to Playing state" << std::endl;
            if (game->GetCurrentState() != GameState::Playing) {
                game->SetCurrentState(GameState::Playing);
            }
            break;
        // Enemy-related messages
        case MessageType::EnemySpawn:
            ProcessEnemySpawnMessage(parsed);
            break;
        case MessageType::EnemyBatchSpawn:
            ProcessEnemyBatchSpawnMessage(parsed);
            break;
        case MessageType::EnemyHit:
            ProcessEnemyHitMessage(parsed);
            break;
        case MessageType::EnemyDeath:
            ProcessEnemyDeathMessage(parsed);
            break;
        case MessageType::PlayerDamage:
            // Handle player taking damage from enemy
            {
                std::string localSteamIDStr = std::to_string(SteamUser()->GetSteamID().ConvertToUint64());
                if (parsed.steamID == localSteamIDStr) {
                    // This damage is for the local player
                    auto& players = playerManager->GetPlayers();
                    auto it = players.find(localSteamIDStr);
                    if (it != players.end()) {
                        it->second.player.TakeDamage(parsed.damage);
                        std::cout << "[CLIENT] Local player took " << parsed.damage << " damage from enemy " << parsed.enemyId << "\n";
                    }
                }
            }
            break;
        case MessageType::WaveStart:
            ProcessWaveStartMessage(parsed);
            break;
        case MessageType::WaveComplete:
            ProcessWaveCompleteMessage(parsed);
            break;
        default:
            std::cout << "[CLIENT] Unknown message type received: " << msg << "\n";
            break;
    }
}

void ClientNetwork::ProcessChatMessage(const ParsedMessage& parsed) {
    // Placeholder for future chat functionality
}
// Replace the ProcessEnemyPositionsMessage method in Client.cpp
void ClientNetwork::ProcessEnemyPositionsMessage(const ParsedMessage& parsed) {
    // Get the PlayingState and its unified EnemyManager
    PlayingState* playingState = GetPlayingState(game);
    if (!playingState) return;
    
    EnemyManager* enemyManager = playingState->GetEnemyManager();
    if (!enemyManager) return;
    
    // Create a more structured approach to matching enemy IDs with their health
    std::unordered_map<int, int> healthMap;
    for (const auto& healthPair : parsed.enemyHealths) {
        healthMap[healthPair.first] = healthPair.second;
    }
    
    // Convert from vector<pair> to vector<tuple> with proper error handling
    std::vector<std::tuple<int, sf::Vector2f, int>> positionData;
    positionData.reserve(parsed.enemyPositions.size());
    
    // NEW: Track if this is a triangle enemy batch
    bool hasTriangleEnemies = false;
    
    // Process each enemy position and find its corresponding health
    for (size_t i = 0; i < parsed.enemyPositions.size(); ++i) {
        int id = parsed.enemyPositions[i].first;
        sf::Vector2f position = parsed.enemyPositions[i].second;
        
        // Check if this is a triangle enemy
        if (id >= 10000) {
            hasTriangleEnemies = true;
        }
        
        // Perform ghost enemy detection at origin
        // Ignore enemies that appear at exact origin (0,0)
        if (position.x == 0.0f && position.y == 0.0f) {
            std::cout << "[CLIENT] Ignoring potential ghost enemy at origin, ID: " << id << "\n";
            continue;
        }
        
        // Find the health for this enemy id using our map
        int health = 0;
        auto healthIt = healthMap.find(id);
        if (healthIt != healthMap.end()) {
            health = healthIt->second;
        } else {
            // If no health is found, use default values based on enemy type
            health = (id >= 10000) ? TRIANGLE_HEALTH : ENEMY_HEALTH;
        }
        
        // Add to the tuple vector if it has valid health
        if (health > 0) {
            positionData.emplace_back(id, position, health);
        }
    }
    
    // NEW: Use different update methods based on enemy type
    // This avoids potential issues with mixing enemy types in position updates
    if (hasTriangleEnemies) {
        // If this batch contains triangle enemies, use the triangle-specific update method
        if (!positionData.empty()) {
            enemyManager->UpdateTriangleEnemyPositions(positionData);
        }
    } else {
        // For regular enemies, use the standard update method
        if (!positionData.empty()) {
            enemyManager->UpdateEnemyPositions(positionData);
        }
    }
}
void ClientNetwork::ProcessConnectionMessage(const ParsedMessage& parsed) {
    RemotePlayer rp;
    rp.playerID = parsed.steamID;
    rp.isHost = parsed.isHost;
    rp.player = Player(parsed.position, parsed.color);
    rp.cubeColor = parsed.color;
    rp.nameText.setFont(game->GetFont());
    rp.nameText.setString(parsed.steamName);
    rp.baseName = parsed.steamName;
    rp.nameText.setCharacterSize(16);
    rp.nameText.setFillColor(sf::Color::Black);
    playerManager->AddOrUpdatePlayer(parsed.steamID, rp);
    playerManager->SetReadyStatus(parsed.steamID, parsed.isReady);
}

void ClientNetwork::ProcessReadyStatusMessage(const ParsedMessage& parsed) {
    playerManager->SetReadyStatus(parsed.steamID, parsed.isReady);
}

void ClientNetwork::ProcessMovementMessage(const ParsedMessage& parsed) {
    std::string localSteamIDStr = std::to_string(SteamUser()->GetSteamID().ConvertToUint64());
    if (parsed.steamID != localSteamIDStr) {
        auto& playersMap = playerManager->GetPlayers();
        auto existingPlayer = playersMap.find(parsed.steamID);
        
        if (existingPlayer != playersMap.end()) {
            // Keep existing player but update position
            RemotePlayer rp = existingPlayer->second;
            rp.player.SetPosition(parsed.position);
            playerManager->AddOrUpdatePlayer(parsed.steamID, rp);
        } else {
            // If we don't know this player yet, create a new entry
            // This normally shouldn't happen as connection messages should come first
            RemotePlayer rp;
            rp.playerID = parsed.steamID;
            rp.player = Player(parsed.position, sf::Color::Blue);
            rp.nameText.setFont(game->GetFont());
            rp.nameText.setCharacterSize(16);
            rp.nameText.setFillColor(sf::Color::Black);
            
            // Try to get name from Steam
            CSteamID id = CSteamID(std::stoull(parsed.steamID));
            const char* name = SteamFriends()->GetFriendPersonaName(id);
            std::string steamName = name ? name : "Unknown Player";
            
            rp.nameText.setString(steamName);
            rp.baseName = steamName;  // Make sure to set baseName!
            
            playerManager->AddOrUpdatePlayer(parsed.steamID, rp);
        }
    }
}

void ClientNetwork::ProcessTriangleWaveStartMessage(const ParsedMessage& parsed) {
    PlayingState* playingState = GetPlayingState(game);
    if (!playingState) return;
    
    EnemyManager* enemyManager = playingState->GetEnemyManager();
    if (!enemyManager) return;
    
    // Generate triangle enemies with the same seed the host used
    enemyManager->SpawnTriangleWave(parsed.enemyCount, parsed.seed);
}

void ClientNetwork::ProcessBulletMessage(const ParsedMessage& parsed) {
    // Get local player ID
    std::string localSteamIDStr = std::to_string(SteamUser()->GetSteamID().ConvertToUint64());
    
    // Normalize IDs
    std::string normalizedShooterID;
    std::string normalizedLocalID;
    
    try {
        uint64_t shooterNum = std::stoull(parsed.steamID);
        normalizedShooterID = std::to_string(shooterNum);
        
        uint64_t localNum = std::stoull(localSteamIDStr);
        normalizedLocalID = std::to_string(localNum);
    } catch (const std::exception& e) {
        std::cout << "[CLIENT] Error normalizing IDs: " << e.what() << "\n";
        normalizedShooterID = parsed.steamID;
        normalizedLocalID = localSteamIDStr;
    }
    
    // Compare normalized IDs
    std::cout << "[CLIENT] Bullet ownership check - Message ID: '" << normalizedShooterID 
              << "', Local ID: '" << normalizedLocalID 
              << "', Same? " << (normalizedShooterID == normalizedLocalID ? "YES" : "NO") << "\n";
    
    // We need to check if this bullet was already processed locally
    // If the shooter is the local player and we already created this bullet locally,
    // we don't need to create it again
    if (normalizedShooterID == normalizedLocalID) {
        std::cout << "[CLIENT] Ignoring own bullet that was bounced back from server\n";
        return;
    }
    
    // For bullets from other players, add them to our game
    // Make sure the bullet has valid data (non-zero direction)
    if (parsed.direction.x != 0.f || parsed.direction.y != 0.f) {
        playerManager->AddBullet(normalizedShooterID, parsed.position, parsed.direction, parsed.velocity);
        std::cout << "[CLIENT] Added bullet from " << normalizedShooterID 
                  << " at pos (" << parsed.position.x << "," << parsed.position.y << ")\n";
    } else {
        std::cout << "[CLIENT] Received bullet with invalid direction\n";
    }
}

void ClientNetwork::SendMovementUpdate(const sf::Vector2f& position) {
    std::string msg = MessageHandler::FormatMovementMessage(
        std::to_string(SteamUser()->GetSteamID().ConvertToUint64()),
        position
    );
    game->GetNetworkManager().SendMessage(hostID, msg);
}

void ClientNetwork::SendChatMessage(const std::string& message) {
    std::string msg = MessageHandler::FormatChatMessage(
        std::to_string(SteamUser()->GetSteamID().ConvertToUint64()),
        message
    );
    game->GetNetworkManager().SendMessage(hostID, msg);
}

void ClientNetwork::SendConnectionMessage() {
    CSteamID myID = SteamUser()->GetSteamID();
    std::string steamIDStr = std::to_string(myID.ConvertToUint64());
    std::string steamName = SteamFriends()->GetPersonaName();
    sf::Color color = sf::Color::Blue;
    std::string connectMsg = MessageHandler::FormatConnectionMessage(steamIDStr, steamName, color, false, false);
    game->GetNetworkManager().SendMessage(hostID, connectMsg);
}

void ClientNetwork::SendReadyStatus(bool isReady) {
    std::string steamIDStr = std::to_string(SteamUser()->GetSteamID().ConvertToUint64());
    std::string msg = MessageHandler::FormatReadyStatusMessage(steamIDStr, isReady);
    if (game->GetNetworkManager().SendMessage(hostID, msg)) {
        std::cout << "[CLIENT] Sent ready status: " << (isReady ? "true" : "false") << " (" << msg << ")\n";
    } else {
        std::cout << "[CLIENT] Failed to send ready status: " << msg << " - will retry\n";
        pendingReadyMessage = msg;
    }
}

void ClientNetwork::Update() {
    auto now = std::chrono::steady_clock::now();
    float elapsed = std::chrono::duration<float>(now - lastSendTime).count();
    
    if (elapsed >= SEND_INTERVAL) {
        auto& localPlayer = playerManager->GetLocalPlayer().player;
        SendMovementUpdate(localPlayer.GetPosition());
        lastSendTime = now;
    }
    
    if (pendingConnectionMessage) {
        if (game->GetNetworkManager().SendMessage(hostID, pendingMessage)) {
            std::cout << "[CLIENT] Pending connection message sent successfully.\n";
            pendingConnectionMessage = false;
        }
    }
    
    if (!pendingReadyMessage.empty()) {
        if (game->GetNetworkManager().SendMessage(hostID, pendingReadyMessage)) {
            std::cout << "[CLIENT] Pending ready status sent: " << pendingReadyMessage << "\n";
            pendingReadyMessage.clear();
        }
    }
    
    // Handle scheduled validation requests with rate limiting
    if (m_validationRequestTimer > 0) {
        m_validationRequestTimer -= elapsed;
        if (m_validationRequestTimer <= 0) {
            // Check if enough time has passed since the last validation
            float timeSinceLast = std::chrono::duration<float>(now - m_lastValidationTime).count();
            
            if (timeSinceLast >= 5.0f) {
                // Send validation request
                std::string validationRequest = MessageHandler::FormatEnemyValidationRequestMessage();
                if (game->GetNetworkManager().SendMessage(hostID, validationRequest)) {
                    std::cout << "[CLIENT] Sent scheduled validation request\n";
                    m_lastValidationTime = now;
                }
            } else {
                std::cout << "[CLIENT] Skipped validation request - too soon after previous (" 
                         << timeSinceLast << "s)\n";
            }
            
            // Mark request as handled
            m_validationRequestTimer = -1.0f;
        }
    }
    
    // Reduce frequency of periodic validation checks
    m_periodicValidationTimer -= elapsed;
    
    if (m_periodicValidationTimer <= 0) {
        // Reset timer - check every 60 seconds
        m_periodicValidationTimer = 60.0f;
        
        // Only request validation if we haven't requested in the last 10 seconds
        float timeSinceLast = std::chrono::duration<float>(now - m_lastValidationTime).count();
        
        if (timeSinceLast >= 10.0f) {
            // Check if we're in the Playing state before requesting validation
            if (game->GetCurrentState() == GameState::Playing) {
                std::cout << "[CLIENT] Performing periodic ghost enemy check\n";
                
                // Request validation data from host
                std::string validationRequest = MessageHandler::FormatEnemyValidationRequestMessage();
                if (game->GetNetworkManager().SendMessage(hostID, validationRequest)) {
                    m_lastValidationTime = now;
                }
            }
        }
    }
    
    // Improved ghost enemy detection near origin
    static float ghostCheckTimer = 0.0f;
    ghostCheckTimer -= elapsed;

    if (ghostCheckTimer <= 0.0f) {
        // Reset timer - check every 5 seconds
        ghostCheckTimer = 5.0f;
        
        if (game->GetCurrentState() == GameState::Playing) {
            PlayingState* playingState = GetPlayingState(game);
            if (playingState) {
                EnemyManager* enemyManager = playingState->GetEnemyManager();
                if (enemyManager) {
                    // Check for enemies near the origin or top-left corner with expanded detection
                    bool foundGhosts = false;
                    std::vector<int> enemyIdsToRemove;
                    
                    // Get all enemies and inspect their positions
                    for (int id : enemyManager->GetAllEnemyIds()) {
                        if (enemyManager->HasEnemy(id)) {
                            sf::Vector2f pos = enemyManager->GetEnemyPosition(id);
                            
                            // More sophisticated ghost detection:
                            // 1. Enemies at exact origin (0,0)
                            // 2. Enemies near origin (within 10 units)
                            // 3. Enemies at or near top-left corner (within 15 units)
                            // 4. Enemies that haven't moved in a long time (could add this with timestamp tracking)
                            if ((pos.x == 0.0f && pos.y == 0.0f) ||                       // Exact origin
                                (std::abs(pos.x) < 10.0f && std::abs(pos.y) < 10.0f) ||   // Near origin
                                (pos.x < 15.0f && pos.y < 15.0f)) {                        // Top-left corner
                                
                                enemyIdsToRemove.push_back(id);
                                foundGhosts = true;
                            }
                        }
                    }
                    
                    // Remove any ghost enemies
                    for (int id : enemyIdsToRemove) {
                        std::cout << "[CLIENT] Removing ghost enemy, ID: " << id << "\n";
                        enemyManager->RemoveEnemy(id);
                    }
                    
                    // Also check triangle enemies
                    std::vector<int> triangleIdsToRemove;
                    for (int id : enemyManager->GetAllTriangleEnemyIds()) {
                        if (enemyManager->HasTriangleEnemy(id)) {
                            sf::Vector2f pos = enemyManager->GetTriangleEnemyPosition(id);
                            
                            // Same detection criteria as regular enemies
                            if ((pos.x == 0.0f && pos.y == 0.0f) ||
                                (std::abs(pos.x) < 10.0f && std::abs(pos.y) < 10.0f) ||
                                (pos.x < 15.0f && pos.y < 15.0f)) {
                                
                                triangleIdsToRemove.push_back(id);
                                foundGhosts = true;
                            }
                        }
                    }
                    
                    // Remove any ghost triangle enemies
                    for (int id : triangleIdsToRemove) {
                        std::cout << "[CLIENT] Removing ghost triangle enemy, ID: " << id << "\n";
                        enemyManager->HandleTriangleEnemyHit(id, 1000, true, "");
                    }
                    
                    // If we found ghosts, request a validation only if significant number found
                    if (foundGhosts && (enemyIdsToRemove.size() + triangleIdsToRemove.size() > 2)) {
                        // Request validation data from host
                        std::string validationRequest = MessageHandler::FormatEnemyValidationRequestMessage();
                        game->GetNetworkManager().SendMessage(hostID, validationRequest);
                        m_lastValidationTime = std::chrono::steady_clock::now();
                    }
                }
            }
        }
    }
}

void ClientNetwork::ProcessPlayerDeathMessage(const ParsedMessage& parsed) {
    auto& players = playerManager->GetPlayers();
    if (players.find(parsed.steamID) != players.end()) {
        RemotePlayer& player = players[parsed.steamID];
        player.player.TakeDamage(100); // Kill the player
        player.respawnTimer = 3.0f;    // Set respawn timer
    }
    
    // Increment the killer's kill count
    if (players.find(parsed.killerID) != players.end()) {
        playerManager->IncrementPlayerKills(parsed.killerID);
    }
}

void ClientNetwork::ProcessPlayerRespawnMessage(const ParsedMessage& parsed) {
    // Normalize the player ID
    std::string normalizedID = parsed.steamID;
    try {
        uint64_t idNum = std::stoull(parsed.steamID);
        normalizedID = std::to_string(idNum);
    } catch (const std::exception& e) {
        std::cout << "[CLIENT] Error normalizing player ID in ProcessPlayerRespawnMessage: " << e.what() << "\n";
    }
    
    // Get local player ID for comparison
    std::string localSteamIDStr = std::to_string(SteamUser()->GetSteamID().ConvertToUint64());
    
    // Find player in map
    auto& players = playerManager->GetPlayers();
    auto it = players.find(normalizedID);
    
    if (it != players.end()) {
        RemotePlayer& player = it->second;
        
        // Save health before respawn for debugging
        int oldHealth = player.player.GetHealth();
        bool wasDead = player.player.IsDead();
        
        // Set respawn position to the position received in message
        player.player.SetRespawnPosition(parsed.position);
        player.player.Respawn();
        
        // Log the position for debugging
        std::cout << "[CLIENT] Player " << normalizedID 
                  << " respawned at position (" << parsed.position.x << "," << parsed.position.y << ")"
                  << " Was dead: " << (wasDead ? "yes" : "no") 
                  << ", Health: " << oldHealth << " -> " << player.player.GetHealth() << "\n";
                  
        // If health is not 100 after respawn, force it
        if (player.player.GetHealth() < 100) {
            std::cout << "[CLIENT] WARNING: Player health not fully restored after respawn, forcing to 100\n";
            player.player.TakeDamage(-100);  // Hack to add health
        }
    } else {
        std::cout << "[CLIENT] Could not find player " << normalizedID << " to respawn from network message\n";
    }
    if (normalizedID == localSteamIDStr) {
        // Schedule an immediate validation request to clean up any ghost enemies
        std::cout << "[CLIENT] Scheduling enemy validation after respawn to clean up ghosts\n";
        
        // Request enemy validation from host
        std::string validationRequest = MessageHandler::FormatEnemyValidationRequestMessage();
        game->GetNetworkManager().SendMessage(hostID, validationRequest);
        
        // Reset the validation timer so we don't request again too soon
        m_validationRequestTimer = -1.0f;
        m_lastValidationTime = std::chrono::steady_clock::now();
        
        PlayingState* playingState = GetPlayingState(game);
        if (playingState) {
            EnemyManager* enemyManager = playingState->GetEnemyManager();
            if (enemyManager) {
                // Check for and remove any enemies at origin (0,0) which are likely ghosts
                std::vector<int> enemyIdsToRemove;
                for (int id : enemyManager->GetAllEnemyIds()) {
                    if (enemyManager->HasEnemy(id)) {
                        // Check if this enemy is at origin or top-left corner
                        sf::Vector2f pos = enemyManager->GetEnemyPosition(id);
                        if ((pos.x == 0.0f && pos.y == 0.0f) || 
                            (pos.x < 10.0f && pos.y < 10.0f)) {
                            enemyIdsToRemove.push_back(id);
                        }
                    }
                }
                
                // Remove any suspicious enemies
                for (int id : enemyIdsToRemove) {
                    std::cout << "[CLIENT] Removing suspected ghost enemy at origin, ID: " << id << "\n";
                    enemyManager->RemoveEnemy(id);
                }
                
                // Also check triangle enemies
                std::vector<int> triangleIdsToRemove;
                for (int id : enemyManager->GetAllTriangleEnemyIds()) {
                    if (enemyManager->HasTriangleEnemy(id)) {
                        sf::Vector2f pos = enemyManager->GetTriangleEnemyPosition(id);
                        if ((pos.x == 0.0f && pos.y == 0.0f) || 
                            (pos.x < 10.0f && pos.y < 10.0f)) {
                            triangleIdsToRemove.push_back(id);
                        }
                    }
                }
                
                // Remove any suspicious triangle enemies
                for (int id : triangleIdsToRemove) {
                    std::cout << "[CLIENT] Removing suspected ghost triangle enemy at origin, ID: " << id << "\n";
                    // Handle triangle enemy removal
                    enemyManager->HandleTriangleEnemyHit(id, 1000, true, "");
                }
            }
        }
    }
}


void ClientNetwork::ProcessEnemySpawnMessage(const ParsedMessage& parsed) {
    // Access the PlayingState's unified EnemyManager
    PlayingState* playingState = GetPlayingState(game);
    if (!playingState) return;
    
    EnemyManager* enemyManager = playingState->GetEnemyManager();
    if (!enemyManager) return;
    
    // Determine health to use - either from the message or use appropriate default
    
    
    int health = (parsed.enemyType == ParsedMessage::EnemyType::Triangle) ? 
            TRIANGLE_HEALTH : ENEMY_HEALTH;
    
    
    // Add the enemy based on type
    if (parsed.enemyType == ParsedMessage::EnemyType::Triangle) {
        enemyManager->AddTriangleEnemy(parsed.enemyId, parsed.position, TRIANGLE_HEALTH);
        std::cout << "[CLIENT] Added triangle enemy #" << parsed.enemyId 
                  << " at (" << parsed.position.x << "," << parsed.position.y 
                  << ") with health: " << health << "\n";
    } else {
        enemyManager->AddEnemy(parsed.enemyId, parsed.position, EnemyType::Rectangle, ENEMY_HEALTH);
        std::cout << "[CLIENT] Added regular enemy #" << parsed.enemyId 
                  << " at (" << parsed.position.x << "," << parsed.position.y 
                  << ") with health: " << health << "\n";
    }
}

void ClientNetwork::ProcessEnemyBatchSpawnMessage(const ParsedMessage& parsed) {
    // Access the PlayingState's unified EnemyManager
    PlayingState* playingState = GetPlayingState(game);
    if (!playingState) return;
    
    EnemyManager* enemyManager = playingState->GetEnemyManager();
    if (!enemyManager) return;
    
    // Process each enemy in the batch
    std::cout << "[CLIENT] Processing batch spawn with " << parsed.enemyPositions.size() << " enemies (type: " 
              << (parsed.enemyType == ParsedMessage::EnemyType::Triangle ? "Triangle" : "Regular") << ")\n";
    
    int successCount = 0;
    int updatedCount = 0;
    
    for (size_t i = 0; i < parsed.enemyPositions.size(); ++i) {
        int id = parsed.enemyPositions[i].first;
        sf::Vector2f position = parsed.enemyPositions[i].second;
        
        // Ensure we get the correct health value
        int health = 0;
        if (i < parsed.enemyHealths.size()) {
            health = parsed.enemyHealths[i].second;
        } else {
            // Search for the health value in the enemyHealths array by matching ID
            for (const auto& healthPair : parsed.enemyHealths) {
                if (healthPair.first == id) {
                    health = healthPair.second;
                    break;
                }
            }
        }
        
        // Use appropriate default health if not provided
        if (health <= 0) {
            health = (parsed.enemyType == ParsedMessage::EnemyType::Triangle) ? 
                TRIANGLE_HEALTH : ENEMY_HEALTH;
        }
        
        // Check if this enemy already exists
        bool exists = false;
        if (parsed.enemyType == ParsedMessage::EnemyType::Triangle) {
            exists = enemyManager->HasTriangleEnemy(id);
        } else {
            exists = enemyManager->HasEnemy(id);
        }
        
        if (exists) {
            // Update existing enemy's health and position
            if (parsed.enemyType == ParsedMessage::EnemyType::Triangle) {
                enemyManager->UpdateTriangleEnemyHealth(id, health);
                // We'll update positions through the regular position update mechanism
            } else {
                enemyManager->UpdateEnemyHealth(id, health);
            }
            updatedCount++;
        } else {
            // Add the enemy based on type with explicit health value
            if (parsed.enemyType == ParsedMessage::EnemyType::Triangle) {
                enemyManager->AddTriangleEnemy(id, position, health);
                std::cout << "[CLIENT] Added triangle enemy #" << id 
                          << " at (" << position.x << "," << position.y 
                          << ") with health: " << health << "\n";
            } else {
                enemyManager->AddEnemy(id, position, EnemyType::Rectangle, health);
                std::cout << "[CLIENT] Added regular enemy #" << id 
                          << " at (" << position.x << "," << position.y 
                          << ") with health: " << health << "\n";
            }
            successCount++;
        }
    }
    
    std::cout << "[CLIENT] Batch spawn complete: " << successCount << " new enemies, " 
              << updatedCount << " updated enemies\n";
}


void ClientNetwork::ProcessEnemyHitMessage(const ParsedMessage& parsed) {
    // Access the PlayingState's unified EnemyManager
    PlayingState* playingState = GetPlayingState(game);
    if (!playingState) return;

    EnemyManager* enemyManager = playingState->GetEnemyManager();
    if (!enemyManager) return;
    
    // Check if this hit was from the local player
    std::string localSteamIDStr = std::to_string(SteamUser()->GetSteamID().ConvertToUint64());
    bool isLocalHit = (parsed.steamID == localSteamIDStr);
    
    if (isLocalHit) {
        std::cout << "[CLIENT] Received confirmation of local player's hit on " 
                 << (parsed.enemyType == ParsedMessage::EnemyType::Triangle ? "triangle " : "")
                 << "enemy " << parsed.enemyId << "\n";
    }
    
    // Handle the hit in the unified EnemyManager
    enemyManager->HandleEnemyHit(parsed.enemyId, parsed.damage, parsed.killed, parsed.steamID, parsed.enemyType);
}

void ClientNetwork::ProcessEnemyDeathMessage(const ParsedMessage& parsed) {
    // Get the PlayingState and the unified EnemyManager
    PlayingState* playingState = GetPlayingState(game);
    if (!playingState) {
        std::cout << "[CLIENT] Cannot process enemy death: PlayingState not available" << std::endl;
        return;
    }

    EnemyManager* enemyManager = playingState->GetEnemyManager();
    if (!enemyManager) return;
    
    // Check if the enemy exists
    bool enemyExists = false;
    
    if (parsed.enemyType == ParsedMessage::EnemyType::Triangle) {
        enemyExists = enemyManager->HasTriangleEnemy(parsed.enemyId);
    } else {
        enemyExists = enemyManager->HasEnemy(parsed.enemyId);
    }
    
    if (!enemyExists) {
        std::cout << "[CLIENT] Received death message for non-existent " 
                 << (parsed.enemyType == ParsedMessage::EnemyType::Triangle ? "triangle " : "")
                 << "enemy " << parsed.enemyId << "\n";
        return;
    }
    
    // Handle enemy death based on type
    if (parsed.enemyType == ParsedMessage::EnemyType::Triangle) {
        // Handle triangle enemy death
        enemyManager->HandleTriangleEnemyHit(parsed.enemyId, 1000, true, parsed.killerID); // Force kill
    } else {
        // Handle regular enemy death
        enemyManager->RemoveEnemy(parsed.enemyId);
    }
    
    std::cout << "[CLIENT] Removed " 
              << (parsed.enemyType == ParsedMessage::EnemyType::Triangle ? "triangle " : "")
              << "enemy " << parsed.enemyId << " after death message" << std::endl;
    
    // Check if this was a rewarded kill for the local player
    std::string localSteamIDStr = std::to_string(SteamUser()->GetSteamID().ConvertToUint64());
    if (parsed.rewardKill && parsed.killerID == localSteamIDStr) {
        std::cout << "[CLIENT] Local player killed " 
                 << (parsed.enemyType == ParsedMessage::EnemyType::Triangle ? "triangle " : "")
                 << "enemy " << parsed.enemyId << "\n";
        
        // We'll let the host update our kills/money since they're the authority
    }
}

void ClientNetwork::ProcessWaveStartMessage(const ParsedMessage& parsed) {
    // Get the PlayingState and its unified EnemyManager
    PlayingState* playingState = GetPlayingState(game);
    if (!playingState) return;
    
    EnemyManager* enemyManager = playingState->GetEnemyManager();
    if (!enemyManager) return;
    
    std::cout << "[CLIENT] Received wave start for wave " << parsed.waveNumber << "\n";
    
    // Clear any remaining enemies as a precaution
    enemyManager->ClearAllEnemies();
    
    // Schedule a single validation request after wave start, with a longer delay
    // This ensures we get a comprehensive list of enemies from the host
    m_validationRequestTimer = 2.0f;  // Increased from 0.5f to 2.0f
    
    // Don't update lastValidationTime here - that will be done when the actual request is sent
    std::cout << "[CLIENT] Scheduled validation request " << m_validationRequestTimer << "s after wave start\n";
}

void ClientNetwork::ProcessWaveCompleteMessage(const ParsedMessage& parsed) {
    // Get the PlayingState and its unified EnemyManager
    PlayingState* playingState = GetPlayingState(game);
    if (!playingState) return;
    
    EnemyManager* enemyManager = playingState->GetEnemyManager();
    if (!enemyManager) return;
    
    std::cout << "[CLIENT] Wave " << parsed.waveNumber << " complete\n";
    
    // Client-side wave completion logic (if any)
    // This might be informing the EnemyManager for UI updates
}



void ClientNetwork::ProcessEnemyValidationMessage(const ParsedMessage& parsed) {
    // Get the PlayingState and its unified EnemyManager
    PlayingState* playingState = GetPlayingState(game);
    if (!playingState) return;
    
    EnemyManager* enemyManager = playingState->GetEnemyManager();
    if (!enemyManager) return;
    
    // Store IDs before validation (for debug)
    std::vector<int> beforeIds;
    if (parsed.enemyType == ParsedMessage::EnemyType::Triangle) {
        beforeIds = enemyManager->GetAllTriangleEnemyIds();
    } else {
        beforeIds = enemyManager->GetAllEnemyIds();
    }
    
    // Validate enemy list based on type
    if (parsed.enemyType == ParsedMessage::EnemyType::Triangle) {
        enemyManager->ValidateTriangleEnemyList(parsed.validEnemyIds);
        
        // Compare before and after for debug
        std::vector<int> afterIds = enemyManager->GetAllTriangleEnemyIds();
        std::cout << "[CLIENT] Processed triangle enemy validation. Before: " 
                 << beforeIds.size() << " enemies, After: " << afterIds.size() 
                 << " enemies" << std::endl;
                 
        // Look for removed ghost enemies
        int removedCount = 0;
        for (int id : beforeIds) {
            bool stillExists = false;
            for (int validId : afterIds) {
                if (id == validId) {
                    stillExists = true;
                    break;
                }
            }
            if (!stillExists) {
                std::cout << "[CLIENT] Removed ghost triangle enemy: " << id << std::endl;
                removedCount++;
            }
        }
        
        if (removedCount > 0) {
            std::cout << "[CLIENT] Removed " << removedCount << " ghost triangle enemies" << std::endl;
        }
    } else {
        enemyManager->ValidateEnemyList(parsed.validEnemyIds);
        
        // Compare before and after for debug
        std::vector<int> afterIds = enemyManager->GetAllEnemyIds();
        std::cout << "[CLIENT] Processed regular enemy validation. Before: " 
                 << beforeIds.size() << " enemies, After: " << afterIds.size() 
                 << " enemies" << std::endl;
                 
        // Look for removed ghost enemies
        int removedCount = 0;
        for (int id : beforeIds) {
            bool stillExists = false;
            for (int validId : afterIds) {
                if (id == validId) {
                    stillExists = true;
                    break;
                }
            }
            if (!stillExists) {
                std::cout << "[CLIENT] Removed ghost regular enemy: " << id << std::endl;
                removedCount++;
            }
        }
        
        if (removedCount > 0) {
            std::cout << "[CLIENT] Removed " << removedCount << " ghost regular enemies" << std::endl;
        }
    }
    
    // Request a position update after validation
    if (!parsed.validEnemyIds.empty()) {
        std::cout << "[CLIENT] Requesting position update after enemy validation" << std::endl;
        std::string validationRequest = MessageHandler::FormatEnemyValidationRequestMessage();
        game->GetNetworkManager().SendMessage(hostID, validationRequest);
    }
}