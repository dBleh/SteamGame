#include "Client.h"
#include "../Game.h"
#include "../network/NetworkManager.h"
#include "../utils/MessageHandler.h"
#include "../states/PlayingState.h"
#include "../entities/TriangleManager.h"
#include <iostream>

ClientNetwork::ClientNetwork(Game* game, PlayerManager* manager)
    : game(game), playerManager(manager), lastSendTime(std::chrono::steady_clock::now()) {
    hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
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
    
    TriangleEnemyManager* triangleManager = playingState->GetTriangleEnemyManager();
    if (!triangleManager) return;
    
    // Generate enemies with the same seed the host used
    triangleManager->GenerateEnemiesWithSeed(parsed.seed, parsed.enemyCount);
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
        
        // Set respawn position and call respawn
        player.player.SetRespawnPosition(parsed.position);
        player.player.Respawn();
        
        // Check health after respawn
        int newHealth = player.player.GetHealth();
        bool isDeadNow = player.player.IsDead();
  
                  
        // If health is not 100 after respawn, force it
        if (newHealth < 100) {
            std::cout << "[CLIENT] WARNING: Player health not fully restored after respawn, forcing to 100\n";
            player.player.TakeDamage(-100);  // Hack to add health
        }
    } else {
        std::cout << "[CLIENT] Could not find player " << normalizedID << " to respawn from network message\n";
    }
}

void ClientNetwork::ProcessEnemySpawnMessage(const ParsedMessage& parsed) {
    // Access the PlayingState's EnemyManager through the Game
    PlayingState* playingState = GetPlayingState(game);
    if (playingState) {
        // Determine if this is for a regular enemy or triangle enemy based on ID range
        // Negative IDs are used for triangle enemies (optional - if you want to distinguish)
        if (parsed.enemyId < 0) {
            TriangleEnemyManager* triangleManager = playingState->GetTriangleEnemyManager();
            if (triangleManager) {
                triangleManager->AddEnemy(parsed.enemyId, parsed.position);
            }
        } else {
            EnemyManager* enemyManager = playingState->GetEnemyManager();
            if (enemyManager) {
                enemyManager->AddEnemy(parsed.enemyId, parsed.position);
            }
        }
    }
}

void ClientNetwork::ProcessEnemyHitMessage(const ParsedMessage& parsed) {
    // Access the PlayingState's EnemyManager
    PlayingState* playingState = GetPlayingState(game);
    if (!playingState) return;

    // Check if this hit was from the local player
    std::string localSteamIDStr = std::to_string(SteamUser()->GetSteamID().ConvertToUint64());
    bool isLocalHit = (parsed.steamID == localSteamIDStr);
    
    // Determine which enemy manager to use based on enemy ID
    if (parsed.enemyId < 0) {
        // Triangle enemy
        TriangleEnemyManager* triangleManager = playingState->GetTriangleEnemyManager();
        if (triangleManager) {
            if (isLocalHit) {
                std::cout << "[CLIENT] Received confirmation of local player's hit on triangle enemy " << parsed.enemyId << "\n";
            }
            triangleManager->HandleEnemyHit(parsed.enemyId, parsed.damage, parsed.killed);
        }
    } else {
        // Regular enemy
        EnemyManager* enemyManager = playingState->GetEnemyManager();
        if (enemyManager) {
            if (isLocalHit) {
                std::cout << "[CLIENT] Received confirmation of local player's hit on enemy " << parsed.enemyId << "\n";
            }
            enemyManager->HandleEnemyHit(parsed.enemyId, parsed.damage, parsed.killed);
        }
    }
}

void ClientNetwork::ProcessEnemyDeathMessage(const ParsedMessage& parsed) {
    // Get the PlayingState
    PlayingState* playingState = GetPlayingState(game);
    if (!playingState) {
        std::cout << "[CLIENT] Cannot process enemy death: PlayingState not available" << std::endl;
        return;
    }

    // Determine which enemy manager to use based on enemy ID
    if (parsed.enemyId < 0) {
        // Triangle enemy
        TriangleEnemyManager* triangleManager = playingState->GetTriangleEnemyManager();
        if (triangleManager && triangleManager->HasEnemy(parsed.enemyId)) {
            triangleManager->RemoveEnemy(parsed.enemyId);
            std::cout << "[CLIENT] Removed triangle enemy " << parsed.enemyId << " after death message" << std::endl;
            
            // Check if this was a rewarded kill for the local player
            std::string localSteamIDStr = std::to_string(SteamUser()->GetSteamID().ConvertToUint64());
            if (parsed.rewardKill && parsed.killerID == localSteamIDStr) {
                std::cout << "[CLIENT] Local player killed triangle enemy " << parsed.enemyId << "\n";
            }
        }
    } else {
        // Regular enemy
        EnemyManager* enemyManager = playingState->GetEnemyManager();
        if (enemyManager && enemyManager->HasEnemy(parsed.enemyId)) {
            enemyManager->RemoveEnemy(parsed.enemyId);
            std::cout << "[CLIENT] Removed enemy " << parsed.enemyId << " after death message" << std::endl;
            
            // Check if this was a rewarded kill for the local player
            std::string localSteamIDStr = std::to_string(SteamUser()->GetSteamID().ConvertToUint64());
            if (parsed.rewardKill && parsed.killerID == localSteamIDStr) {
                std::cout << "[CLIENT] Local player killed enemy " << parsed.enemyId << "\n";
            }
        }
    }
}

void ClientNetwork::ProcessWaveStartMessage(const ParsedMessage& parsed) {
    // Get the PlayingState and its EnemyManager
    PlayingState* playingState = GetPlayingState(game);
    if (playingState) {
        EnemyManager* enemyManager = playingState->GetEnemyManager();
        if (enemyManager) {
            // Clear any lingering enemies before new wave starts
            std::vector<int> emptyList;
            enemyManager->RemoveStaleEnemies(emptyList);
            
            // As a client, we don't spawn enemies, but we update our wave number
            if (parsed.waveNumber > enemyManager->GetCurrentWave()) {
                std::cout << "[CLIENT] Starting wave " << parsed.waveNumber << "\n";
                // The enemies will be spawned via EnemySpawn messages from the host
            }
        }
    }
}

void ClientNetwork::ProcessWaveCompleteMessage(const ParsedMessage& parsed) {
    // Get the PlayingState and its EnemyManager
    if (game->GetCurrentState() == GameState::Playing) {
        PlayingState* playingState = dynamic_cast<PlayingState*>(game->GetState());
        if (playingState) {
            EnemyManager* enemyManager = playingState->GetEnemyManager();
            if (enemyManager) {
                std::cout << "[CLIENT] Wave " << parsed.waveNumber << " complete\n";
                // Client-side wave completion logic (if any)
            }
        }
    }
}

void ClientNetwork::ProcessEnemyPositionsMessage(const ParsedMessage& parsed) {
    PlayingState* playingState = GetPlayingState(game);
    if (!playingState) return;
    
    // Process regular enemies
    EnemyManager* enemyManager = playingState->GetEnemyManager();
    if (enemyManager) {
        // First update positions
        enemyManager->UpdateEnemyPositions(parsed.enemyPositions);
        
        // Now update health values
        for (const auto& healthPair : parsed.enemyHealths) {
            int id = healthPair.first;
            int health = healthPair.second;
            
            enemyManager->UpdateEnemyHealth(id, health);
        }
    }
    
    // Process triangle enemies
    TriangleEnemyManager* triangleManager = playingState->GetTriangleEnemyManager();
    if (triangleManager && !parsed.triangleEnemyPositions.empty()) {
        triangleManager->UpdateEnemyPositions(parsed.triangleEnemyPositions);
    }
}

void ClientNetwork::ProcessEnemyValidationMessage(const ParsedMessage& parsed) {
    // Access the PlayingState
    PlayingState* playingState = GetPlayingState(game);
    if (!playingState) return;

    // Process regular enemies
    EnemyManager* enemyManager = playingState->GetEnemyManager();
    if (enemyManager) {
        enemyManager->ValidateEnemyList(parsed.validEnemyIds);
        std::cout << "[CLIENT] Processed enemy validation with " << parsed.validEnemyIds.size() << " enemies" << std::endl;
    }
    
    // Process triangle enemies
    TriangleEnemyManager* triangleManager = playingState->GetTriangleEnemyManager();
    if (triangleManager && !parsed.triangleValidEnemyIds.empty()) {
        triangleManager->ValidateEnemyList(parsed.triangleValidEnemyIds);
        std::cout << "[CLIENT] Processed triangle enemy validation with " << parsed.triangleValidEnemyIds.size() << " enemies" << std::endl;
    }
}