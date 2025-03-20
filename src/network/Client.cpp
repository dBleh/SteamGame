#include "Client.h"
#include "../core/Game.h"
#include "NetworkManager.h"
#include "messages/MessageHandler.h"
#include "messages/PlayerMessageHandler.h"
#include "messages/EnemyMessageHandler.h"
#include "messages/StateMessageHandler.h"
#include "messages/SystemMessageHandler.h"
#include "../states/PlayingState.h"
#include "../utils/config/Config.h"
#include <iostream>

ClientNetwork::ClientNetwork(Game* game, PlayerManager* manager)
    : game(game), 
      playerManager(manager), 
      lastSendTime(std::chrono::steady_clock::now()) {
    
    hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
    m_lastValidationTime = std::chrono::steady_clock::now();
    m_lastStateRequestTime = std::chrono::steady_clock::now();
    m_validationRequestTimer = 0.5f;
    m_periodicValidationTimer = 10.0f; // Request state every 10 seconds initially
    m_stateRequestCooldown = MIN_STATE_REQUEST_COOLDOWN;
    
    std::cout << "[CLIENT] Initialized, host ID: " << hostID.ConvertToUint64() << "\n";
}

ClientNetwork::~ClientNetwork() {}

void ClientNetwork::ProcessMessage(const std::string& msg, CSteamID sender) {
    // Special handling for chunked messages
    if (msg.compare(0, 11, "CHUNK_START") == 0 || 
        msg.compare(0, 10, "CHUNK_PART") == 0 || 
        msg.compare(0, 9, "CHUNK_END") == 0) {
        
        ParsedMessage parsed = MessageHandler::ParseMessage(msg);
        
        // If this is a CHUNK_END and it returns a valid message type, process the reconstructed message
        if (msg.compare(0, 9, "CHUNK_END") == 0 && parsed.type != MessageType::Unknown) {
            std::cout << "[CLIENT] Processing reconstructed chunked message of type: " 
                      << static_cast<int>(parsed.type) << "\n";
            
            const auto* descriptor = MessageHandler::GetDescriptorByType(parsed.type);
            if (descriptor && descriptor->clientHandler) {
                descriptor->clientHandler(*game, *this, parsed);
                
                // Reset state request counter if we received a complete state message
                if (parsed.type == MessageType::EnemyState) {
                    m_stateRequestPending = false;
                    m_consecutiveStateRequests = 0;
                    m_stateRequestCooldown = MIN_STATE_REQUEST_COOLDOWN;
                    std::cout << "[CLIENT] Received chunked state update, request satisfied\n";
                }
            } else {
                std::cout << "[CLIENT] Unhandled message type in reconstructed message: " 
                          << static_cast<int>(parsed.type) << "\n";
                ProcessUnknownMessage(*game, *this, parsed);
            }
        }
        return;
    }
    
    // Standard message processing
    ParsedMessage parsed = MessageHandler::ParseMessage(msg);
    const auto* descriptor = MessageHandler::GetDescriptorByType(parsed.type);
    
    if (descriptor && descriptor->clientHandler) {
        descriptor->clientHandler(*game, *this, parsed);
        
        // Check if we just received a state update message
        if (parsed.type == MessageType::EnemyState || msg.compare(0, 3, "ECS") == 0) {
            m_stateRequestPending = false;
            m_consecutiveStateRequests = 0;
            m_stateRequestCooldown = MIN_STATE_REQUEST_COOLDOWN;
            std::cout << "[CLIENT] Received state update, request satisfied\n";
        }
    } else {
        std::cout << "[CLIENT] Unhandled message type received: " << msg << "\n";
        ProcessUnknownMessage(*game, *this, parsed);
    }
}

void ClientNetwork::ProcessForceFieldUpdateMessage(Game& game, ClientNetwork& client, const ParsedMessage& parsed) {
    // Get the player ID from the message
    std::string playerID = parsed.steamID;
    
    // Normalize player ID for consistent comparison
    std::string normalizedPlayerID;
    try {
        uint64_t idNum = std::stoull(playerID);
        normalizedPlayerID = std::to_string(idNum);
    } catch (const std::exception& e) {
        std::cout << "[CLIENT] Error normalizing player ID in ProcessForceFieldUpdateMessage: " << e.what() << "\n";
        normalizedPlayerID = playerID;
    }
    
    // Get the local player ID
    std::string localPlayerID = std::to_string(SteamUser()->GetSteamID().ConvertToUint64());
    
    // Ignore updates for the local player (we already have the latest)
    if (normalizedPlayerID == localPlayerID) {
        std::cout << "[CLIENT] Ignoring force field update for local player\n";
        return;
    }
    
    // Look up the player in the players map
    auto& players = playerManager->GetPlayers();
    auto it = players.find(normalizedPlayerID);
    
    if (it != players.end()) {
        RemotePlayer& rp = it->second;
        
        // Make sure the player has a force field
        if (!rp.player.HasForceField()) {
            rp.player.InitializeForceField();
        }
        
        ForceField* forceField = rp.player.GetForceField();
        if (forceField) {
            // Update the force field parameters
            forceField->SetRadius(parsed.ffRadius);
            forceField->SetDamage(parsed.ffDamage);
            forceField->SetCooldown(parsed.ffCooldown);
            forceField->SetChainLightningTargets(parsed.ffChainTargets);
            forceField->SetChainLightningEnabled(parsed.ffChainEnabled);
            forceField->SetPowerLevel(parsed.ffPowerLevel);
            forceField->SetFieldType(static_cast<FieldType>(parsed.ffType));
            
            std::cout << "[CLIENT] Updated force field for player " << normalizedPlayerID 
                      << " - Radius: " << parsed.ffRadius
                      << ", Damage: " << parsed.ffDamage
                      << ", Type: " << parsed.ffType << "\n";
            
            // Make sure the force field is enabled for this player
            rp.player.EnableForceField(true);
        }
    }
}

void ClientNetwork::ProcessChatMessage(Game& game, ClientNetwork& client, const ParsedMessage& parsed) {
    std::cout << "[CLIENT] Received chat message from " << parsed.steamID << ": " << parsed.chatMessage << "\n";
}

void ClientNetwork::ProcessKillMessage(Game& game, ClientNetwork& client, const ParsedMessage& parsed) {
    std::string killerID = parsed.steamID;
    int enemyId = parsed.enemyId;
    
    std::cout << "[CLIENT] Received kill message from host - Player ID: " << killerID 
              << ", Enemy ID: " << enemyId << "\n";
    
    // Normalize killer ID
    std::string normalizedKillerID;
    try {
        uint64_t killerIdNum = std::stoull(killerID);
        normalizedKillerID = std::to_string(killerIdNum);
    } catch (const std::exception& e) {
        std::cout << "[CLIENT] Error normalizing killer ID: " << e.what() << "\n";
        normalizedKillerID = killerID;
    }
    
    // Update kill count based on host's authoritative message
    auto& players = playerManager->GetPlayers();
    auto it = players.find(normalizedKillerID);
    if (it != players.end()) {
        it->second.kills++;
        it->second.money += ENEMY_KILL_REWARD; // Award money for the kill
        
        std::cout << "[CLIENT] Player " << normalizedKillerID << " awarded kill by host for enemy " 
                  << enemyId << " - New kill count: " << it->second.kills << "\n";
        
        // If it's the local player, make sure we process any effects
        std::string localID = std::to_string(SteamUser()->GetSteamID().ConvertToUint64());
        if (normalizedKillerID == localID) {
            std::cout << "[CLIENT] Local player received kill confirmation from host\n";
        }
    } else {
        std::cout << "[CLIENT] WARNING: Kill message for unknown player ID: " << normalizedKillerID << "\n";
    }
    
    // Make sure the enemy is removed locally too
    PlayingState* playingState = GetPlayingState(&game);
    if (playingState && playingState->GetEnemyManager()) {
        EnemyManager* enemyManager = playingState->GetEnemyManager();
        Enemy* enemy = enemyManager->FindEnemy(enemyId);
        if (enemy) {
            enemyManager->RemoveEnemy(enemyId);
        }
    }
}

void ClientNetwork::ProcessConnectionMessage(Game& game, ClientNetwork& client, const ParsedMessage& parsed) {
    // Create a new RemotePlayer directly with its fields set, don't copy
    RemotePlayer rp;
    rp.playerID = parsed.steamID;
    rp.isHost = parsed.isHost;
    // Create player with position and color
    rp.player = Player(parsed.position, parsed.color);
    rp.cubeColor = parsed.color;
    rp.nameText.setFont(game.GetFont());
    rp.nameText.setString(parsed.steamName);
    rp.baseName = parsed.steamName;
    rp.nameText.setCharacterSize(PLAYER_NAME_FONT_SIZE);
    rp.nameText.setFillColor(PLAYER_NAME_COLOR);
    
    // Move into the players map
    playerManager->AddOrUpdatePlayer(parsed.steamID, std::move(rp));
    playerManager->SetReadyStatus(parsed.steamID, parsed.isReady);
}

void ClientNetwork::ProcessReadyStatusMessage(Game& game, ClientNetwork& client, const ParsedMessage& parsed) {
    playerManager->SetReadyStatus(parsed.steamID, parsed.isReady);
}

void ClientNetwork::ProcessMovementMessage(Game& game, ClientNetwork& client, const ParsedMessage& parsed) {
    std::string localSteamIDStr = std::to_string(SteamUser()->GetSteamID().ConvertToUint64());
    if (parsed.steamID != localSteamIDStr) {
        auto& playersMap = playerManager->GetPlayers();
        auto existingPlayer = playersMap.find(parsed.steamID);
        if (existingPlayer != playersMap.end()) {
            // Update existing player's position
            existingPlayer->second.previousPosition = existingPlayer->second.player.GetPosition();
            existingPlayer->second.targetPosition = parsed.position;
            existingPlayer->second.lastUpdateTime = std::chrono::steady_clock::now();
        } else {
            // Create a new player
            RemotePlayer rp;
            rp.playerID = parsed.steamID;
            rp.player = Player(parsed.position, PLAYER_DEFAULT_COLOR);
            rp.nameText.setFont(game.GetFont());
            rp.nameText.setCharacterSize(PLAYER_NAME_FONT_SIZE);
            rp.nameText.setFillColor(PLAYER_NAME_COLOR);
            CSteamID id = CSteamID(std::stoull(parsed.steamID));
            const char* name = SteamFriends()->GetFriendPersonaName(id);
            std::string steamName = name ? name : "Unknown Player";
            rp.nameText.setString(steamName);
            rp.baseName = steamName;
            // Add to the player manager using move semantics
            playerManager->AddOrUpdatePlayer(parsed.steamID, std::move(rp));
        }
    }
}

void ClientNetwork::ProcessBulletMessage(Game& game, ClientNetwork& client, const ParsedMessage& parsed) {
    std::string localSteamIDStr = std::to_string(SteamUser()->GetSteamID().ConvertToUint64());
    std::string normalizedShooterID, normalizedLocalID;
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
    
    if (normalizedShooterID == normalizedLocalID) {
        std::cout << "[CLIENT] Ignoring own bullet that was bounced back from server\n";
        return;
    }
    
    if (parsed.direction.x != 0.f || parsed.direction.y != 0.f) {
        playerManager->AddBullet(normalizedShooterID, parsed.position, parsed.direction, parsed.velocity);
    } else {
        std::cout << "[CLIENT] Received bullet with invalid direction\n";
    }
}

void ClientNetwork::SendMovementUpdate(const sf::Vector2f& position) {
    std::string msg = PlayerMessageHandler::FormatMovementMessage(std::to_string(SteamUser()->GetSteamID().ConvertToUint64()), position);
    game->GetNetworkManager().SendMessage(hostID, msg);
}

void ClientNetwork::SendChatMessage(const std::string& message) {
    std::string msg = SystemMessageHandler::FormatChatMessage(std::to_string(SteamUser()->GetSteamID().ConvertToUint64()), message);
    game->GetNetworkManager().SendMessage(hostID, msg);
}

void ClientNetwork::SendConnectionMessage() {
    CSteamID myID = SteamUser()->GetSteamID();
    std::string steamIDStr = std::to_string(myID.ConvertToUint64());
    std::string steamName = SteamFriends()->GetPersonaName();
    sf::Color color = PLAYER_DEFAULT_COLOR;
    std::string connectMsg = PlayerMessageHandler::FormatConnectionMessage(steamIDStr, steamName, color, false, false);
    
    if (game->GetNetworkManager().SendMessage(hostID, connectMsg)) {
        std::cout << "[CLIENT] Sent connection message\n";
    } else {
        std::cout << "[CLIENT] Failed to send connection message, will retry\n";
        pendingConnectionMessage = true;
        pendingMessage = connectMsg;
    }
}

void ClientNetwork::SendReadyStatus(bool isReady) {
    std::string steamIDStr = std::to_string(SteamUser()->GetSteamID().ConvertToUint64());
    std::string msg = StateMessageHandler::FormatReadyStatusMessage(steamIDStr, isReady);
    
    if (game->GetNetworkManager().SendMessage(hostID, msg)) {
        std::cout << "[CLIENT] Sent ready status: " << (isReady ? "true" : "false") << "\n";
    } else {
        std::cout << "[CLIENT] Failed to send ready status - will retry\n";
        pendingReadyMessage = msg;
    }
}

void ClientNetwork::RequestEnemyState() {
    auto now = std::chrono::steady_clock::now();
    float secondsSinceLastRequest = std::chrono::duration<float>(now - m_lastStateRequestTime).count();
    
    // Only send a request if we're not in cooldown and don't have a pending request
    if (secondsSinceLastRequest >= m_stateRequestCooldown && !m_stateRequestPending) {
        m_lastStateRequestTime = now;
        m_stateRequestPending = true;
        m_consecutiveStateRequests++;
        
        // Increase cooldown with each consecutive request to prevent spam
        if (m_consecutiveStateRequests > 1) {
            m_stateRequestCooldown = std::min(
                MAX_STATE_REQUEST_COOLDOWN,
                m_stateRequestCooldown * 1.5f
            );
        }
        
        std::string requestMsg = EnemyMessageHandler::FormatEnemyStateRequestMessage();
        if (game->GetNetworkManager().SendMessage(hostID, requestMsg)) {
            std::cout << "[CLIENT] Sent enemy state request to host (request #" 
                      << m_consecutiveStateRequests << ", next cooldown: " 
                      << m_stateRequestCooldown << "s)\n";
        } else {
            std::cout << "[CLIENT] Failed to send enemy state request\n";
            m_stateRequestPending = false;  // Allow retry on next update
        }
    }
}

void ClientNetwork::Update() {
    auto now = std::chrono::steady_clock::now();
    float elapsed = std::chrono::duration<float>(now - lastSendTime).count();
    
    // Update movement sending
    if (elapsed >= SEND_INTERVAL) {
        auto& localPlayer = playerManager->GetLocalPlayer().player;
        SendMovementUpdate(localPlayer.GetPosition());
        lastSendTime = now;
    }
    
    // Handle pending connection message
    if (pendingConnectionMessage) {
        if (game->GetNetworkManager().SendMessage(hostID, pendingMessage)) {
            std::cout << "[CLIENT] Pending connection message sent successfully\n";
            pendingConnectionMessage = false;
        }
    }
    
    // Handle pending ready status message
    if (!pendingReadyMessage.empty()) {
        if (game->GetNetworkManager().SendMessage(hostID, pendingReadyMessage)) {
            std::cout << "[CLIENT] Pending ready status sent\n";
            pendingReadyMessage.clear();
        }
    }
    
    // Periodic enemy state validation
    float timeSinceValidation = std::chrono::duration<float>(now - m_lastValidationTime).count();
    if (timeSinceValidation >= m_periodicValidationTimer) {
        m_lastValidationTime = now;
        
        // Request enemy state periodically
        PlayingState* playingState = GetPlayingState(game);
        if (playingState && playingState->GetEnemyManager()) {
            size_t enemyCount = playingState->GetEnemyManager()->GetEnemyCount();
            
            // Only request if we have fewer than expected enemies
            // This helps reduce unnecessary network traffic
            bool isWave = playingState->GetEnemyManager()->GetCurrentWave() > 0;
            
            if (isWave && enemyCount == 0) {
                std::cout << "[CLIENT] No enemies found but wave is active, requesting state update\n";
                RequestEnemyState();
            } else {
                std::cout << "[CLIENT] Periodic validation - enemies: " << enemyCount << "\n";
                
                // Even if we have enemies, occasionally request an update just to be sure
                if (m_consecutiveStateRequests < MAX_CONSECUTIVE_REQUESTS) {
                    RequestEnemyState();
                }
            }
        }
    }
    
    // Reset consecutive request counter over time to allow new requests
    float timeSinceLastRequest = std::chrono::duration<float>(now - m_lastStateRequestTime).count();
    if (timeSinceLastRequest > MAX_STATE_REQUEST_COOLDOWN && m_consecutiveStateRequests > 0) {
        m_consecutiveStateRequests = 0;
        m_stateRequestCooldown = MIN_STATE_REQUEST_COOLDOWN;
        std::cout << "[CLIENT] Reset state request counters after long period of inactivity\n";
    }
}

void ClientNetwork::ProcessPlayerDeathMessage(Game& game, ClientNetwork& client, const ParsedMessage& parsed) {
    std::string normalizedID = parsed.steamID;
    try {
        uint64_t idNum = std::stoull(parsed.steamID);
        normalizedID = std::to_string(idNum);
    } catch (const std::exception& e) {
        std::cout << "[CLIENT] Error normalizing player ID in ProcessPlayerDeathMessage: " << e.what() << "\n";
        normalizedID = parsed.steamID;
    }
    
    auto& players = playerManager->GetPlayers();
    auto it = players.find(normalizedID);
    if (it != players.end()) {
        RemotePlayer& player = it->second;
        sf::Vector2f currentPos = player.player.GetPosition();
        player.player.SetRespawnPosition(currentPos);
        player.player.TakeDamage(player.player.GetHealth()); // Take full damage to ensure death
        player.respawnTimer = RESPAWN_TIME;
        std::cout << "[CLIENT] Player " << normalizedID << " died\n";
    }
    
    if (!parsed.killerID.empty()) {
        std::string normalizedKillerID = parsed.killerID;
        try {
            uint64_t killerIdNum = std::stoull(parsed.killerID);
            normalizedKillerID = std::to_string(killerIdNum);
        } catch (const std::exception& e) {
            normalizedKillerID = parsed.killerID;
        }
        
        auto killerIt = players.find(normalizedKillerID);
        if (killerIt != players.end()) {
            playerManager->IncrementPlayerKills(normalizedKillerID);
        }
    }
    
    std::string localPlayerID = std::to_string(SteamUser()->GetSteamID().ConvertToUint64());
    if (normalizedID == localPlayerID) {
        std::cout << "[CLIENT] Local player died, will request validation after 1 second\n";
        m_validationRequestTimer = 1.0f;
    } else {
        m_validationRequestTimer = 0.5f;
    }
}

void ClientNetwork::ProcessPlayerRespawnMessage(Game& game, ClientNetwork& client, const ParsedMessage& parsed) {
    std::string normalizedID = parsed.steamID;
    try {
        uint64_t idNum = std::stoull(parsed.steamID);
        normalizedID = std::to_string(idNum);
    } catch (const std::exception& e) {
        std::cout << "[CLIENT] Error normalizing player ID in ProcessPlayerRespawnMessage: " << e.what() << "\n";
    }
    
    std::string localSteamIDStr = std::to_string(SteamUser()->GetSteamID().ConvertToUint64());
    auto& players = playerManager->GetPlayers();
    auto it = players.find(normalizedID);
    
    if (it != players.end()) {
        RemotePlayer& player = it->second;
        int oldHealth = player.player.GetHealth();
        bool wasDead = player.player.IsDead();
        
        player.player.SetRespawnPosition(parsed.position);
        player.player.Respawn();
        
        if (player.player.GetHealth() < PLAYER_HEALTH) {
            std::cout << "[CLIENT] WARNING: Player health not fully restored after respawn, forcing to full health\n";
            player.player.TakeDamage(-PLAYER_HEALTH); // Heal to full health
        }
    } else {
        std::cout << "[CLIENT] Could not find player " << normalizedID << " to respawn from network message\n";
    }
}

void ClientNetwork::ProcessStartGameMessage(Game& game, ClientNetwork& client, const ParsedMessage& parsed) {
    std::cout << "[CLIENT] Received start game message, changing to Playing state\n";
    if (game.GetCurrentState() != GameState::Playing) {
        game.SetCurrentState(GameState::Playing);
    }
}

void ClientNetwork::ProcessPlayerDamageMessage(Game& game, ClientNetwork& client, const ParsedMessage& parsed) {
    std::string localSteamIDStr = std::to_string(SteamUser()->GetSteamID().ConvertToUint64());
    if (parsed.steamID == localSteamIDStr) {
        auto& players = playerManager->GetPlayers();
        auto it = players.find(localSteamIDStr);
        if (it != players.end()) {
            it->second.player.TakeDamage(parsed.damage);
        }
    }
}

void ClientNetwork::ProcessUnknownMessage(Game& game, ClientNetwork& client, const ParsedMessage& parsed) {
    std::cout << "[CLIENT] Unknown message type received\n";
}

void ClientNetwork::ProcessForceFieldZapMessage(Game& game, ClientNetwork& client, const ParsedMessage& parsed) {
    std::string zapperID = parsed.steamID;
    int enemyId = parsed.enemyId;
    float damage = parsed.damage;
    
    // Normalize player ID for consistent comparison
    std::string normalizedZapperID;
    try {
        uint64_t zapperIdNum = std::stoull(zapperID);
        normalizedZapperID = std::to_string(zapperIdNum);
    } catch (const std::exception& e) {
        std::cout << "[CLIENT] Error normalizing zapper ID: " << e.what() << "\n";
        normalizedZapperID = zapperID;
    }
    
    // Get the local player ID
    std::string localID = std::to_string(SteamUser()->GetSteamID().ConvertToUint64());
    std::string normalizedLocalID;
    try {
        uint64_t localIdNum = std::stoull(localID);
        normalizedLocalID = std::to_string(localIdNum);
    } catch (const std::exception& e) {
        std::cout << "[CLIENT] Error normalizing local ID: " << e.what() << "\n";
        normalizedLocalID = localID;
    }
    
    // Don't process our own zap messages that are echoed back from host
    if (normalizedZapperID == normalizedLocalID) {
        return;
    }
    
    // Apply damage to the enemy (if we have access to it)
    PlayingState* playingState = GetPlayingState(&game);
    if (playingState && playingState->GetEnemyManager()) {
        EnemyManager* enemyManager = playingState->GetEnemyManager();
        Enemy* enemy = enemyManager->FindEnemy(enemyId);
        
        // If we found the enemy, apply the visual effect and possibly remove it
        if (enemy) {
            // Apply the damage locally to keep in sync with the host
            bool killed = enemyManager->InflictDamage(enemyId, damage);
            
            // Get the player who owns the force field
            auto& players = playerManager->GetPlayers();
            auto it = players.find(normalizedZapperID);
            if (it != players.end()) {
                RemotePlayer& rp = it->second;
                
                // Make sure the player has a force field
                if (!rp.player.HasForceField()) {
                    rp.player.InitializeForceField();
                }
                
                // Get player and enemy positions
                sf::Vector2f playerPos = rp.player.GetPosition() + sf::Vector2f(PLAYER_WIDTH/2.0f, PLAYER_HEIGHT/2.0f);
                sf::Vector2f enemyPos = enemy->GetPosition();
                
                // Create the zap effect
                rp.player.GetForceField()->CreateZapEffect(playerPos, enemyPos);
                rp.player.GetForceField()->SetIsZapping(true);
                rp.player.GetForceField()->SetZapEffectTimer(FIELD_ZAP_EFFECT_DURATION);
            }
        } else {
            // We don't have this enemy locally - request a state update
            if (!m_stateRequestPending) {
                std::cout << "[CLIENT] Force field zap references unknown enemy " << enemyId 
                          << ", requesting state update\n";
                RequestEnemyState();
            }
        }
    }
}