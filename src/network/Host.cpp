#include "Host.h"
#include "../Game.h"
#include "../network/NetworkManager.h"
#include "../utils/MessageHandler.h"
#include "../states/PlayingState.h"
#include <iostream>


HostNetwork::HostNetwork(Game* game, PlayerManager* manager)
    : game(game), playerManager(manager), lastBroadcastTime(std::chrono::steady_clock::now()) {
    // Add host to the player list
    CSteamID hostID = game->GetLocalSteamID();  // Host's SteamID
    std::string hostIDStr = std::to_string(hostID.ConvertToUint64());
    std::string hostName = SteamFriends()->GetPersonaName();  // Host's Steam name

    RemotePlayer hostPlayer;
    hostPlayer.playerID = hostIDStr;
    hostPlayer.isHost = true;
    hostPlayer.player = Player(sf::Vector2f(200.f, 200.f), sf::Color::Blue);  // Initial position and color
    hostPlayer.cubeColor = sf::Color::Blue;
    hostPlayer.nameText.setFont(game->GetFont());
    hostPlayer.nameText.setString(hostName);
    hostPlayer.baseName = hostName;
    hostPlayer.nameText.setCharacterSize(16);
    hostPlayer.nameText.setFillColor(sf::Color::Black);
    hostPlayer.isReady = false;  // Initial ready status

    // Add or update the host in PlayerManager
    playerManager->AddOrUpdatePlayer(hostIDStr, hostPlayer);
    std::cout << "[HOST] Added host to player list: " << hostName << " (" << hostIDStr << ")\n";

    // Immediately broadcast the full player list to ensure clients get the host's data
    BroadcastFullPlayerList();
}

HostNetwork::~HostNetwork() {}

void HostNetwork::ProcessMessage(const std::string& msg, CSteamID sender) {
    ParsedMessage parsed = MessageHandler::ParseMessage(msg);
    switch (parsed.type) {
        case MessageType::Connection:
            ProcessConnectionMessage(parsed);
            break;
        case MessageType::Movement:
            ProcessMovementMessage(parsed, sender);
            break;
        case MessageType::Chat:
            ProcessChatMessage(parsed.chatMessage, sender);
            break;
        case MessageType::ReadyStatus:
            ProcessReadyStatusMessage(parsed);
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
        case MessageType::EnemyClear:
            std::cout << "[HOST] Received enemy clear message" << std::endl;
            {
                // Get the PlayingState and its unified EnemyManager
                PlayingState* playingState = GetPlayingState(game);
                if (playingState) {
                    EnemyManager* enemyManager = playingState->GetEnemyManager();
                    if (enemyManager) {
                        // Clear all enemies
                        enemyManager->ClearAllEnemies();
                        
                        // Broadcast to all clients to ensure everyone is in sync
                        std::string clearMsg = MessageHandler::FormatEnemyClearMessage();
                        game->GetNetworkManager().BroadcastMessage(clearMsg);
                    }
                }
            }
            break;
        case MessageType::EnemyValidationRequest:
            ProcessEnemyValidationRequestMessage(parsed);
            break;
        case MessageType::StartGame:
            std::cout << "[HOST] Received start game message, changing to Playing state" << std::endl;
            if (game->GetCurrentState() != GameState::Playing) {
                game->SetCurrentState(GameState::Playing);
            }
            break;
        // Enemy-related messages
        case MessageType::EnemyHit:
            ProcessEnemyHitMessage(parsed);
            break;
        case MessageType::EnemyDeath:
            ProcessEnemyDeathMessage(parsed);
            break;
        case MessageType::WaveStart:
            ProcessWaveStartMessage(parsed);
            break;
        case MessageType::WaveComplete:
            ProcessWaveCompleteMessage(parsed);
            break;
        case MessageType::TriangleWaveStart:
            ProcessTriangleWaveStartMessage(parsed);
            break;
        case MessageType::EnemyBatchSpawn:
            ProcessEnemyBatchSpawnMessage(parsed);
            break;
        default:
            std::cout << "[HOST] Unknown message type received: " << msg << "\n";
            break;
    }
}

void HostNetwork::ProcessConnectionMessage(const ParsedMessage& parsed) {
    auto& players = playerManager->GetPlayers();
    auto it = players.find(parsed.steamID);

    if (it == players.end()) {
        // New player: create and add
        RemotePlayer rp;
        rp.playerID = parsed.steamID;
        rp.isHost = false;
        rp.player = Player(sf::Vector2f(200.f, 200.f), parsed.color);
        rp.cubeColor = parsed.color;
        rp.nameText.setFont(game->GetFont());
        rp.nameText.setString(parsed.steamName);
        rp.baseName = parsed.steamName;
        rp.nameText.setCharacterSize(16);
        rp.nameText.setFillColor(sf::Color::Black);
        playerManager->AddOrUpdatePlayer(parsed.steamID, rp);
        std::cout << "[HOST] New player connected: " << parsed.steamName << " (" << parsed.steamID << ")\n";
    } else {
        // Existing player: update fields without overwriting name
        RemotePlayer& rp = it->second;
        rp.isHost = false;  // Always false for remote players on host
        rp.player.SetPosition(sf::Vector2f(200.f, 200.f));  // Initial position (could use parsed.position if sent)
        rp.cubeColor = parsed.color;
        if (rp.baseName != parsed.steamName) {
            rp.baseName = parsed.steamName;
            rp.nameText.setString(parsed.steamName);
            std::cout << "[HOST] Updated name for " << parsed.steamID << " to " << parsed.steamName << "\n";
        }
    }
    playerManager->SetReadyStatus(parsed.steamID, parsed.isReady);
    BroadcastFullPlayerList();
}

void HostNetwork::ProcessMovementMessage(const ParsedMessage& parsed, CSteamID sender) {
    if (parsed.steamID.empty()) {
        std::cout << "[HOST] Invalid movement message from " << sender.ConvertToUint64() << "\n";
        return;
    }
    RemotePlayer rp;
    rp.player = Player(parsed.position, sf::Color::Blue);
    rp.nameText.setFont(game->GetFont());
    auto& playersMap = playerManager->GetPlayers();
    rp.nameText.setCharacterSize(16);
    rp.nameText.setFillColor(sf::Color::Black);
    playerManager->AddOrUpdatePlayer(parsed.steamID, rp);
    std::string broadcastMsg = MessageHandler::FormatMovementMessage(parsed.steamID, parsed.position);
    game->GetNetworkManager().BroadcastMessage(broadcastMsg);
}

void HostNetwork::ProcessBulletMessage(const ParsedMessage& parsed) {
    // Get local player ID (host's ID)
    std::string localSteamIDStr = std::to_string(game->GetLocalSteamID().ConvertToUint64());
    
    // Normalize IDs for consistent comparison
    std::string normalizedShooterID;
    std::string normalizedLocalID;
    
    try {
        uint64_t shooterNum = std::stoull(parsed.steamID);
        normalizedShooterID = std::to_string(shooterNum);
        
        uint64_t localNum = std::stoull(localSteamIDStr);
        normalizedLocalID = std::to_string(localNum);
    } catch (const std::exception& e) {
        std::cout << "[HOST] Error normalizing IDs: " << e.what() << "\n";
        normalizedShooterID = parsed.steamID;
        normalizedLocalID = localSteamIDStr;
    }
    
    // Validate the bullet data before broadcasting
    if (parsed.direction.x == 0.f && parsed.direction.y == 0.f) {
        std::cout << "[HOST] Received invalid bullet direction, ignoring\n";
        return;
    }
    
    // Broadcast this bullet to all clients (including the one who sent it)
    // This ensures all clients see all bullets
    std::string broadcastMsg = MessageHandler::FormatBulletMessage(
        normalizedShooterID, parsed.position, parsed.direction, parsed.velocity);
    
    bool sent = game->GetNetworkManager().BroadcastMessage(broadcastMsg);
    if (sent) {
        std::cout << "[HOST] Broadcast bullet from " << normalizedShooterID << "\n";
    } else {
        std::cout << "[HOST] Failed to broadcast bullet message\n";
    }
    
    // Skip adding bullets that were fired by the host (local player)
    // since they've already been added when the host shot
    if (normalizedShooterID == normalizedLocalID) {
        std::cout << "[HOST] Ignoring own bullet that was received as a message\n";
        return;
    }
    
    // For bullets from other players, add them to the host's game
    playerManager->AddBullet(normalizedShooterID, parsed.position, parsed.direction, parsed.velocity);
}

void HostNetwork::ProcessChatMessage(const std::string& message, CSteamID sender) {
    std::string msg = MessageHandler::FormatChatMessage(std::to_string(sender.ConvertToUint64()), message);
    game->GetNetworkManager().BroadcastMessage(msg);
}

void HostNetwork::ProcessReadyStatusMessage(const ParsedMessage& parsed) {
    std::string localSteamIDStr = std::to_string(game->GetLocalSteamID().ConvertToUint64());
    if (localSteamIDStr != parsed.steamID) {
        auto& players = playerManager->GetPlayers();
        if (players.find(parsed.steamID) != players.end() && players[parsed.steamID].isReady != parsed.isReady) {
            playerManager->SetReadyStatus(parsed.steamID, parsed.isReady);
        }
    }
    std::string broadcastMsg = MessageHandler::FormatReadyStatusMessage(parsed.steamID, parsed.isReady);
    game->GetNetworkManager().BroadcastMessage(broadcastMsg);
}

void HostNetwork::BroadcastFullPlayerList() {
    auto& players = playerManager->GetPlayers();
    for (const auto& pair : players) {
        const RemotePlayer& rp = pair.second;
        std::string msg = MessageHandler::FormatConnectionMessage(
            rp.playerID,
            rp.baseName,
            rp.cubeColor,
            rp.isReady,
            rp.isHost
        );
        game->GetNetworkManager().BroadcastMessage(msg);
    }
}

void HostNetwork::BroadcastPlayersList() {
    auto& players = playerManager->GetPlayers();
    for (auto& pair : players) {
        std::string msg = MessageHandler::FormatMovementMessage(pair.first, pair.second.player.GetPosition());
        game->GetNetworkManager().BroadcastMessage(msg);
    }
}

void HostNetwork::Update() {
    auto now = std::chrono::steady_clock::now();
    float elapsed = std::chrono::duration<float>(now - lastBroadcastTime).count();
    if (elapsed >= BROADCAST_INTERVAL) {
        BroadcastPlayersList();
        lastBroadcastTime = now;
    }
}

void HostNetwork::ProcessPlayerDeathMessage(const ParsedMessage& parsed) {
    std::string playerID = parsed.steamID;
    std::string killerID = parsed.killerID;
    
    // Update the player's state in the PlayerManager
    auto& players = playerManager->GetPlayers();
    if (players.find(playerID) != players.end()) {
        RemotePlayer& player = players[playerID];
        player.player.TakeDamage(100); // Kill the player
        player.respawnTimer = 3.0f;    // Set respawn timer
    }
    
    // Increment the killer's kill count
    if (players.find(killerID) != players.end()) {
        playerManager->IncrementPlayerKills(killerID);
    }
    
    // Broadcast the death message to all clients
    std::string deathMsg = MessageHandler::FormatPlayerDeathMessage(playerID, killerID);
    game->GetNetworkManager().BroadcastMessage(deathMsg);
}

void HostNetwork::ProcessPlayerRespawnMessage(const ParsedMessage& parsed) {
    std::string playerID = parsed.steamID;
    sf::Vector2f respawnPos = parsed.position;
    
    // Update the player's state in the PlayerManager
    auto& players = playerManager->GetPlayers();
    if (players.find(playerID) != players.end()) {
        RemotePlayer& player = players[playerID];
        player.player.SetRespawnPosition(respawnPos);
        player.player.Respawn();
    }
    
    // Broadcast the respawn message to all clients
    std::string respawnMsg = MessageHandler::FormatPlayerRespawnMessage(playerID, respawnPos);
    game->GetNetworkManager().BroadcastMessage(respawnMsg);
}

void HostNetwork::ProcessEnemyHitMessage(const ParsedMessage& parsed) {
    // Access the PlayingState
    PlayingState* playingState = GetPlayingState(game);
    if (!playingState) return;

    // Get the unified EnemyManager
    EnemyManager* enemyManager = playingState->GetEnemyManager();
    if (!enemyManager) return;
    
    // Check if enemy exists
    bool enemyFound = false;
    int enemyHealthBefore = 0;
    
    if (parsed.enemyType == ParsedMessage::EnemyType::Triangle) {
        enemyFound = enemyManager->HasTriangleEnemy(parsed.enemyId);
        if (enemyFound) {
            enemyHealthBefore = enemyManager->GetTriangleEnemyHealth(parsed.enemyId);
        }
    } else {
        enemyFound = enemyManager->HasEnemy(parsed.enemyId);
        if (enemyFound) {
            enemyHealthBefore = enemyManager->GetEnemyHealth(parsed.enemyId);
        }
    }
    
    if (!enemyFound) {
        std::cout << "[HOST] Received hit for non-existent " 
                 << (parsed.enemyType == ParsedMessage::EnemyType::Triangle ? "triangle " : "")
                 << "enemy " << parsed.enemyId << "\n";
        return;
    }
    
    // Apply damage if this is a hit message from a client
    std::string localSteamIDStr = std::to_string(SteamUser()->GetSteamID().ConvertToUint64());
    
    if (parsed.steamID != localSteamIDStr) {
        // Process the enemy hit from client
        enemyManager->HandleEnemyHit(parsed.enemyId, parsed.damage, parsed.killed, parsed.steamID, parsed.enemyType);
        
        // Get result of hit (for debugging)
        int enemyHealthAfter = 0;
        bool killed = false;
        
        if (parsed.enemyType == ParsedMessage::EnemyType::Triangle) {
            enemyHealthAfter = enemyManager->GetTriangleEnemyHealth(parsed.enemyId);
            killed = (enemyHealthAfter <= 0);
        } else {
            enemyHealthAfter = enemyManager->GetEnemyHealth(parsed.enemyId);
            killed = (enemyHealthAfter <= 0);
        }
        
        std::cout << "[HOST] Enemy " << parsed.enemyId 
                  << " health changed from " << enemyHealthBefore 
                  << " to " << enemyHealthAfter 
                  << " after hit from client " << parsed.steamID << "\n";
        
        // Broadcast the hit to all clients (including original sender)
        std::string hitMsg = MessageHandler::FormatEnemyHitMessage(
            parsed.enemyId, parsed.damage, killed, parsed.steamID, parsed.enemyType);
        game->GetNetworkManager().BroadcastMessage(hitMsg);
        
        // If enemy was killed, handle death separately
        if (killed) {
            // Broadcast enemy death message
            std::string deathMsg = MessageHandler::FormatEnemyDeathMessage(
                parsed.enemyId, parsed.steamID, true, parsed.enemyType);
            game->GetNetworkManager().BroadcastMessage(deathMsg);
        }
    } else {
        // For host's own hits, we've already applied damage in CheckBulletCollisions
        // and sent the network message, so we don't need to do anything here
        std::cout << "[HOST] Ignoring redundant hit message for own bullet\n";
    }
}

void HostNetwork::ProcessEnemyDeathMessage(const ParsedMessage& parsed) {
    // Get the PlayingState
    PlayingState* playingState = GetPlayingState(game);
    if (!playingState) {
        std::cout << "[HOST] Cannot process enemy death: PlayingState not available" << std::endl;
        return;
    }

    // Get the unified EnemyManager
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
        std::cout << "[HOST] Received death message for non-existent " 
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
    
    std::cout << "[HOST] Removed " 
              << (parsed.enemyType == ParsedMessage::EnemyType::Triangle ? "triangle " : "")
              << "enemy " << parsed.enemyId << " after death message" << std::endl;
    
    // Broadcast to ensure all clients know - send multiple times for reliability
    std::string deathMsg = MessageHandler::FormatEnemyDeathMessage(
        parsed.enemyId, parsed.killerID, parsed.rewardKill, parsed.enemyType);
    
    // Send the message twice for redundancy
    game->GetNetworkManager().BroadcastMessage(deathMsg);
    game->GetNetworkManager().BroadcastMessage(deathMsg);
}

void HostNetwork::ProcessWaveStartMessage(const ParsedMessage& parsed) {
    // Get the PlayingState and its unified EnemyManager
    PlayingState* playingState = GetPlayingState(game);
    if (!playingState) return;
    
    EnemyManager* enemyManager = playingState->GetEnemyManager();
    if (!enemyManager) return;
    
    // Start the wave if we're the host
    if (SteamUser()->GetSteamID() == SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID())) {
        // Start the wave
        enemyManager->StartNextWave();
        
        // Broadcast wave start to ensure all clients know
        std::string waveMsg = MessageHandler::FormatWaveStartMessage(
            enemyManager->GetCurrentWave());
            
        // Broadcast multiple times for reliability
        game->GetNetworkManager().BroadcastMessage(waveMsg);
        game->GetNetworkManager().BroadcastMessage(waveMsg);
        
        std::cout << "[HOST] Broadcast wave start for wave " << enemyManager->GetCurrentWave() << " (duplicated for reliability)" << std::endl;
        
        // Force a full sync right after spawning
        enemyManager->SyncFullEnemyList();
    }
}

void HostNetwork::ProcessWaveCompleteMessage(const ParsedMessage& parsed) {
    // Wave complete messages are primarily for clients
    // But we'll process it anyway for consistency
    
    // Broadcast to ensure all clients know
    std::string waveMsg = MessageHandler::FormatWaveCompleteMessage(parsed.waveNumber);
    game->GetNetworkManager().BroadcastMessage(waveMsg);
}


void HostNetwork::ProcessEnemyValidationRequestMessage(const ParsedMessage& parsed) {
    // Get the PlayingState and its unified EnemyManager
    PlayingState* playingState = GetPlayingState(game);
    if (!playingState) return;
    
    EnemyManager* enemyManager = playingState->GetEnemyManager();
    if (!enemyManager) return;
    
    // Add rate limiting for validation responses
    static auto lastValidationResponse = std::chrono::steady_clock::now() - std::chrono::seconds(10);
    auto now = std::chrono::steady_clock::now();
    float timeSinceLast = std::chrono::duration<float>(now - lastValidationResponse).count();
    
    if (timeSinceLast < 1.0f) {
        std::cout << "[HOST] Ignoring validation request - too soon after last response\n";
        return;
    }
    lastValidationResponse = now;
    
    std::cout << "[HOST] Received enemy validation request, sending full list" << std::endl;
    
    // Send with slight delay to ensure it goes after other messages
    // This is a simple way to improve the timing without complex task scheduling
    sf::sleep(sf::milliseconds(20)); // Reduced from 50ms
    
    // Get all regular enemy IDs for validation
    std::vector<int> regularIds = enemyManager->GetAllEnemyIds();
    
    // Send regular enemy validation list first
    if (!regularIds.empty()) {
        std::string regularMsg = MessageHandler::FormatEnemyValidationMessage(regularIds);
        game->GetNetworkManager().BroadcastMessage(regularMsg);
        std::cout << "[HOST] Sent validation list with " << regularIds.size() << " regular enemies" << std::endl;
    }
    
    // Add a delay between messages
    sf::sleep(sf::milliseconds(20)); // Reduced from 50ms
    
    // Get all triangle enemy IDs for validation
    std::vector<int> triangleIds = enemyManager->GetAllTriangleEnemyIds();
    
    // Send triangle enemy validation list
    if (!triangleIds.empty()) {
        std::string triangleMsg = MessageHandler::FormatEnemyValidationMessage(triangleIds);
        game->GetNetworkManager().BroadcastMessage(triangleMsg);
        std::cout << "[HOST] Sent validation list with " << triangleIds.size() << " triangle enemies" << std::endl;
    }
    
    // Add a delay before position updates
    sf::sleep(sf::milliseconds(20)); // Reduced from 50ms
    
    // Now send actual position data in small batches
    // This is more efficient than using SyncFullEnemyList directly
    
    // Calculate center of all players for prioritization
    sf::Vector2f playerCenter(0, 0);
    int playerCount = 0;
    
    auto& players = playerManager->GetPlayers();
    for (const auto& pair : players) {
        if (!pair.second.player.IsDead()) {
            playerCenter += pair.second.player.GetPosition();
            playerCount++;
        }
    }
    
    if (playerCount > 0) {
        playerCenter /= static_cast<float>(playerCount);
    }
    
    // Create a list of all enemies with priority data
    std::vector<std::tuple<int, sf::Vector2f, int, float, bool>> priorityEnemies;
    
    // Add regular enemies with priority based on distance to players
    for (auto& entry : enemyManager->GetRegularEnemyDataForSync()) {
        int id = std::get<0>(entry);
        sf::Vector2f pos = std::get<1>(entry);
        int health = std::get<2>(entry);
        
        // Calculate distance to players
        float distToPlayers = std::sqrt(
            std::pow(pos.x - playerCenter.x, 2) + 
            std::pow(pos.y - playerCenter.y, 2)
        );
        
        // Calculate priority (higher = more important)
        float priority = 1000.0f / (distToPlayers + 10.0f);
        
        // Add to priority list
        priorityEnemies.emplace_back(id, pos, health, priority, false);
    }
    
    // Add triangle enemies with priority
    for (auto& entry : enemyManager->GetTriangleEnemyDataForSync()) {
        int id = std::get<0>(entry);
        sf::Vector2f pos = std::get<1>(entry);
        int health = std::get<2>(entry);
        
        // Calculate distance to players
        float distToPlayers = std::sqrt(
            std::pow(pos.x - playerCenter.x, 2) + 
            std::pow(pos.y - playerCenter.y, 2)
        );
        
        // Calculate priority (higher = more important)
        float priority = 1000.0f / (distToPlayers + 10.0f);
        
        // Add to priority list
        priorityEnemies.emplace_back(id, pos, health, priority, true);
    }
    
    // Sort by priority (highest first)
    std::sort(priorityEnemies.begin(), priorityEnemies.end(),
        [](const auto& a, const auto& b) {
            return std::get<3>(a) > std::get<3>(b);
        }
    );
    
    // Separate into regular and triangle lists
    std::vector<std::tuple<int, sf::Vector2f, int>> regularBatch;
    std::vector<std::tuple<int, sf::Vector2f, int>> triangleBatch;
    
    // Smaller batch size for more frequent, smoother updates
    const int BATCH_SIZE = 6; // Smaller batch size for validation response
    
    for (const auto& entry : priorityEnemies) {
        int id = std::get<0>(entry);
        sf::Vector2f pos = std::get<1>(entry);
        int health = std::get<2>(entry);
        bool isTriangle = std::get<4>(entry);
        
        if (isTriangle) {
            triangleBatch.emplace_back(id, pos, health);
            
            // Send batch if full
            if (triangleBatch.size() >= BATCH_SIZE) {
                std::string triangleMsg = MessageHandler::FormatEnemyBatchSpawnMessage(
                    triangleBatch, ParsedMessage::EnemyType::Triangle);
                game->GetNetworkManager().BroadcastMessage(triangleMsg);
                
                // Clear batch and add small delay
                triangleBatch.clear();
                sf::sleep(sf::milliseconds(10));
            }
        } else {
            regularBatch.emplace_back(id, pos, health);
            
            // Send batch if full
            if (regularBatch.size() >= BATCH_SIZE) {
                std::string regularMsg = MessageHandler::FormatEnemyBatchSpawnMessage(
                    regularBatch, ParsedMessage::EnemyType::Regular);
                game->GetNetworkManager().BroadcastMessage(regularMsg);
                
                // Clear batch and add small delay
                regularBatch.clear();
                sf::sleep(sf::milliseconds(10));
            }
        }
    }
    
    // Send any remaining batches
    if (!triangleBatch.empty()) {
        std::string triangleMsg = MessageHandler::FormatEnemyBatchSpawnMessage(
            triangleBatch, ParsedMessage::EnemyType::Triangle);
        game->GetNetworkManager().BroadcastMessage(triangleMsg);
    }
    
    if (!regularBatch.empty()) {
        std::string regularMsg = MessageHandler::FormatEnemyBatchSpawnMessage(
            regularBatch, ParsedMessage::EnemyType::Regular);
        game->GetNetworkManager().BroadcastMessage(regularMsg);
    }
    
    std::cout << "[HOST] Completed validation response with prioritized enemies" << std::endl;
}

void HostNetwork::ProcessTriangleWaveStartMessage(const ParsedMessage& parsed) {
    // Get the PlayingState and its unified EnemyManager
    PlayingState* playingState = GetPlayingState(game);
    if (!playingState) return;
    
    EnemyManager* enemyManager = playingState->GetEnemyManager();
    if (!enemyManager) return;
    
    // Extract seed and count from message
    uint32_t seed = parsed.seed;
    int count = parsed.enemyCount;
    
    // Spawn triangle enemies with the provided seed
    enemyManager->SpawnTriangleWave(count, seed);
}

void HostNetwork::ProcessEnemyBatchSpawnMessage(const ParsedMessage& parsed) {
    // Get the PlayingState and its unified EnemyManager
    PlayingState* playingState = GetPlayingState(game);
    if (!playingState) return;
    
    EnemyManager* enemyManager = playingState->GetEnemyManager();
    if (!enemyManager) return;
    
    // Process each enemy in the batch
    for (size_t i = 0; i < parsed.enemyPositions.size(); ++i) {
        int id = parsed.enemyPositions[i].first;
        sf::Vector2f position = parsed.enemyPositions[i].second;
        int health = parsed.enemyHealths[i].second;
        
        // Add the enemy based on type
        if (parsed.enemyType == ParsedMessage::EnemyType::Triangle) {
            enemyManager->AddTriangleEnemy(id, position, health);
        } else {
            enemyManager->AddEnemy(id, position, EnemyType::Rectangle, health);
        }
    }
    
    // Sync positions to ensure all clients are updated
    enemyManager->SyncEnemyPositions();
}