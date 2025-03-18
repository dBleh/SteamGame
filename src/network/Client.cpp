#include "Client.h"
#include "../core/Game.h"
#include "NetworkManager.h"
#include "messages/MessageHandler.h"
#include "../states/PlayingState.h"
#include <iostream>

ClientNetwork::ClientNetwork(Game* game, PlayerManager* manager)
    : game(game), playerManager(manager), lastSendTime(std::chrono::steady_clock::now()) {
    hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
    m_lastValidationTime = std::chrono::steady_clock::now();
    m_validationRequestTimer = 0.5f;
    m_periodicValidationTimer = 30.0f;
    PlayingState* playingState = GetPlayingState(game);
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
    if (playerManager->GetPlayers().find(normalizedKillerID) != playerManager->GetPlayers().end()) {
        playerManager->IncrementPlayerKills(normalizedKillerID);
        std::cout << "[CLIENT] Player " << normalizedKillerID << " awarded kill by host for enemy " << enemyId << "\n";
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
    rp.nameText.setCharacterSize(16);
    rp.nameText.setFillColor(sf::Color::Black);
    
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
            rp.player = Player(parsed.position, sf::Color::Blue);
            rp.nameText.setFont(game.GetFont());
            rp.nameText.setCharacterSize(16);
            rp.nameText.setFillColor(sf::Color::Black);
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
    std::cout << "[CLIENT] Bullet ownership check - Message ID: '" << normalizedShooterID 
              << "', Local ID: '" << normalizedLocalID 
              << "', Same? " << (normalizedShooterID == normalizedLocalID ? "YES" : "NO") << "\n";
    if (normalizedShooterID == normalizedLocalID) {
        std::cout << "[CLIENT] Ignoring own bullet that was bounced back from server\n";
        return;
    }
    if (parsed.direction.x != 0.f || parsed.direction.y != 0.f) {
        playerManager->AddBullet(normalizedShooterID, parsed.position, parsed.direction, parsed.velocity);
        std::cout << "[CLIENT] Added bullet from " << normalizedShooterID 
                  << " at pos (" << parsed.position.x << "," << parsed.position.y << ")\n";
    } else {
        std::cout << "[CLIENT] Received bullet with invalid direction\n";
    }
}

void ClientNetwork::SendMovementUpdate(const sf::Vector2f& position) {
    std::string msg = MessageHandler::FormatMovementMessage(std::to_string(SteamUser()->GetSteamID().ConvertToUint64()), position);
    game->GetNetworkManager().SendMessage(hostID, msg);
}

void ClientNetwork::SendChatMessage(const std::string& message) {
    std::string msg = MessageHandler::FormatChatMessage(std::to_string(SteamUser()->GetSteamID().ConvertToUint64()), message);
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
        player.player.TakeDamage(100);
        player.respawnTimer = 3.0f;
        std::cout << "[CLIENT] Player " << normalizedID 
                  << " died at position (" << currentPos.x << "," << currentPos.y 
                  << "), respawn position set to same location\n";
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
        if (player.player.GetHealth() < 100) {
            std::cout << "[CLIENT] WARNING: Player health not fully restored after respawn, forcing to 100\n";
            player.player.TakeDamage(-100);
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
        
        // If we found the enemy, apply the visual effect
        if (enemy) {
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
                sf::Vector2f playerPos = rp.player.GetPosition() + sf::Vector2f(25.0f, 25.0f); // Assuming 50x50 player
                sf::Vector2f enemyPos = enemy->GetPosition();
                
                // Create the zap effect
                rp.player.GetForceField()->CreateZapEffect(playerPos, enemyPos);
                rp.player.GetForceField()->SetIsZapping(true);
                rp.player.GetForceField()->SetZapEffectTimer(0.3f); // Show effect for 0.3 seconds
            }
        }
    }
}