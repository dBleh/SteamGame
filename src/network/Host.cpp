#include "Host.h"
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

HostNetwork::HostNetwork(Game* game, PlayerManager* manager)
    : game(game), playerManager(manager), lastBroadcastTime(std::chrono::steady_clock::now()) {
    CSteamID hostID = game->GetLocalSteamID();
    std::string hostIDStr = std::to_string(hostID.ConvertToUint64());
    std::string hostName = SteamFriends()->GetPersonaName();
    RemotePlayer hostPlayer;
    hostPlayer.playerID = hostIDStr;
    hostPlayer.isHost = true;
    hostPlayer.player = Player(sf::Vector2f(PLAYER_DEFAULT_START_X * 2, PLAYER_DEFAULT_START_Y * 2), PLAYER_DEFAULT_COLOR);
    hostPlayer.cubeColor = PLAYER_DEFAULT_COLOR;
    hostPlayer.nameText.setFont(game->GetFont());
    hostPlayer.nameText.setString(hostName);
    hostPlayer.baseName = hostName;
    hostPlayer.nameText.setCharacterSize(PLAYER_NAME_FONT_SIZE);
    hostPlayer.nameText.setFillColor(PLAYER_NAME_COLOR);
    hostPlayer.isReady = false;
    playerManager->AddOrUpdatePlayer(hostIDStr, std::move(hostPlayer));
    std::cout << "[HOST] Added host to player list: " << hostName << " (" << hostIDStr << ")\n";
    
    // Initialize force fields for the host player
    playerManager->InitializeForceFields();
    
    
    
    BroadcastFullPlayerList();
}

HostNetwork::~HostNetwork() {}

void HostNetwork::ProcessMessage(const std::string& msg, CSteamID sender) {
    ParsedMessage parsed = MessageHandler::ParseMessage(msg);
    const auto* descriptor = MessageHandler::GetDescriptorByType(parsed.type);
    if (descriptor && descriptor->hostHandler) {
        descriptor->hostHandler(*game, *this, parsed, sender);
    } else {
        std::cout << "[HOST] Unhandled message type received: " << msg << "\n";
        ProcessUnknownMessage(*game, *this, parsed, sender);
    }
}

void HostNetwork::ProcessConnectionMessage(Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender) {
    auto& players = playerManager->GetPlayers();
    auto it = players.find(parsed.steamID);
    if (it == players.end()) {
        RemotePlayer rp;
        rp.playerID = parsed.steamID;
        rp.isHost = false;
        rp.player = Player(sf::Vector2f(PLAYER_DEFAULT_START_X * 2, PLAYER_DEFAULT_START_Y * 2), parsed.color);
        rp.cubeColor = parsed.color;
        rp.nameText.setFont(game.GetFont());
        rp.nameText.setString(parsed.steamName);
        rp.baseName = parsed.steamName;
        rp.nameText.setCharacterSize(PLAYER_NAME_FONT_SIZE);
        rp.nameText.setFillColor(PLAYER_NAME_COLOR);
        playerManager->AddOrUpdatePlayer(parsed.steamID, std::move(rp));
        
        // Initialize force field for the newly connected client
        auto& playerRef = players.find(parsed.steamID)->second;
        if (!playerRef.player.HasForceField()) {
            playerRef.player.InitializeForceField();
            std::cout << "[HOST] Initialized force field for new player: " << parsed.steamName << " (" << parsed.steamID << ")\n";
        }
        
        std::cout << "[HOST] New player connected: " << parsed.steamName << " (" << parsed.steamID << ")\n";
    } else {
        // Update existing player's position
        it->second.player.SetPosition(sf::Vector2f(PLAYER_DEFAULT_START_X * 2, PLAYER_DEFAULT_START_Y * 2));
        it->second.cubeColor = parsed.color;
        if (it->second.baseName != parsed.steamName) {
            it->second.baseName = parsed.steamName;
            it->second.nameText.setString(parsed.steamName);
            std::cout << "[HOST] Updated name for " << parsed.steamID << " to " << parsed.steamName << "\n";
        }
        
        // Initialize force field if it doesn't exist
        if (!it->second.player.HasForceField()) {
            it->second.player.InitializeForceField();
            std::cout << "[HOST] Initialized force field for existing player: " << parsed.steamName << " (" << parsed.steamID << ")\n";
        }
    }
    playerManager->SetReadyStatus(parsed.steamID, parsed.isReady);
    BroadcastFullPlayerList();
}

void HostNetwork::ProcessMovementMessage(Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender) {
    if (parsed.steamID.empty()) {
        std::cout << "[HOST] Invalid movement message from " << sender.ConvertToUint64() << "\n";
        return;
    }
    
    // Look up the player
    auto& playersMap = playerManager->GetPlayers();
    auto playerIt = playersMap.find(parsed.steamID);
    
    if (playerIt != playersMap.end()) {
        // Update existing player's position
        playerIt->second.previousPosition = playerIt->second.player.GetPosition();
        playerIt->second.targetPosition = parsed.position;
        playerIt->second.lastUpdateTime = std::chrono::steady_clock::now();
    } else {
        // Create a new player if needed
        RemotePlayer rp;
        rp.playerID = parsed.steamID;
        rp.player = Player(parsed.position, PLAYER_DEFAULT_COLOR);
        rp.nameText.setFont(game.GetFont());
        rp.nameText.setCharacterSize(PLAYER_NAME_FONT_SIZE);
        rp.nameText.setFillColor(PLAYER_NAME_COLOR);
        playerManager->AddOrUpdatePlayer(parsed.steamID, std::move(rp));
    }
    
    // Broadcast the movement to all clients
    std::string broadcastMsg = PlayerMessageHandler::FormatMovementMessage(parsed.steamID, parsed.position);
    game.GetNetworkManager().BroadcastMessage(broadcastMsg);
}

void HostNetwork::ProcessChatMessageParsed(Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender) {
    ProcessChatMessage(parsed.chatMessage, sender);
}

void HostNetwork::ProcessChatMessage(const std::string& message, CSteamID sender) {
    std::string msg = SystemMessageHandler::FormatChatMessage(std::to_string(sender.ConvertToUint64()), message);
    game->GetNetworkManager().BroadcastMessage(msg);
}
void HostNetwork::ProcessForceFieldUpdateMessage(Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender) {
    // Get the player ID from the message
    std::string playerID = parsed.steamID;
    
    // Normalize player ID for consistent comparison
    std::string normalizedPlayerID;
    try {
        uint64_t idNum = std::stoull(playerID);
        normalizedPlayerID = std::to_string(idNum);
    } catch (const std::exception& e) {
        std::cout << "[HOST] Error normalizing player ID in ProcessForceFieldUpdateMessage: " << e.what() << "\n";
        normalizedPlayerID = playerID;
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
            
            std::cout << "[HOST] Updated force field for player " << normalizedPlayerID 
                      << " - Radius: " << parsed.ffRadius
                      << ", Damage: " << parsed.ffDamage
                      << ", Type: " << parsed.ffType << "\n";
        }
    }
    
    // Broadcast the force field update to all clients
    std::string updateMsg = PlayerMessageHandler::FormatForceFieldUpdateMessage(
        normalizedPlayerID,
        parsed.ffRadius,
        parsed.ffDamage,
        parsed.ffCooldown,
        parsed.ffChainTargets,
        parsed.ffType,
        parsed.ffPowerLevel,
        parsed.ffChainEnabled
    );
    
    game.GetNetworkManager().BroadcastMessage(updateMsg);
}

void HostNetwork::ProcessKillMessage(Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender) {
    std::string killerID = parsed.steamID;
    int enemyId = parsed.enemyId;
    
    // Normalize killer ID for consistent comparison
    std::string normalizedKillerID;
    try {
        uint64_t killerIdNum = std::stoull(killerID);
        normalizedKillerID = std::to_string(killerIdNum);
    } catch (const std::exception& e) {
        std::cout << "[HOST] Error normalizing killer ID: " << e.what() << "\n";
        normalizedKillerID = killerID;
    }
    
    // Validate the kill (check if the enemy exists and is alive)
    bool validKill = false;
    PlayingState* playingState = GetPlayingState(&game);
    if (playingState && playingState->GetEnemyManager()) {
        Enemy* enemy = playingState->GetEnemyManager()->FindEnemy(enemyId);
        validKill = (enemy != nullptr);
    }
    
    if (validKill) {
        // This is where the host acts as authority - increment kill count
        playerManager->IncrementPlayerKills(normalizedKillerID);
        
        // Broadcast the validated kill to all clients
        std::string killMsg = PlayerMessageHandler::FormatKillMessage(normalizedKillerID, enemyId);
        game.GetNetworkManager().BroadcastMessage(killMsg);
        
        std::cout << "[HOST] Validated and broadcast kill for player " << normalizedKillerID << "\n";
    } else {
        std::cout << "[HOST] Rejected invalid kill claim for player " << normalizedKillerID << "\n";
    }
}
void HostNetwork::ProcessReadyStatusMessage(Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender) {
    std::string localSteamIDStr = std::to_string(game.GetLocalSteamID().ConvertToUint64());
    if (localSteamIDStr != parsed.steamID) {
        auto& players = playerManager->GetPlayers();
        if (players.find(parsed.steamID) != players.end() && players[parsed.steamID].isReady != parsed.isReady) {
            playerManager->SetReadyStatus(parsed.steamID, parsed.isReady);
        }
    }
    std::string broadcastMsg = StateMessageHandler::FormatReadyStatusMessage(parsed.steamID, parsed.isReady);
    game.GetNetworkManager().BroadcastMessage(broadcastMsg);
}

void HostNetwork::ProcessBulletMessage(Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender) {
    std::string localSteamIDStr = std::to_string(game.GetLocalSteamID().ConvertToUint64());
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
    if (parsed.direction.x == 0.f && parsed.direction.y == 0.f) {
        std::cout << "[HOST] Received invalid bullet direction, ignoring\n";
        return;
    }
    std::string broadcastMsg = PlayerMessageHandler::FormatBulletMessage(normalizedShooterID, parsed.position, parsed.direction, parsed.velocity);
    bool sent = game.GetNetworkManager().BroadcastMessage(broadcastMsg);
    
    if (normalizedShooterID == normalizedLocalID) {
        std::cout << "[HOST] Ignoring own bullet that was received as a message\n";
        return;
    }
    playerManager->AddBullet(normalizedShooterID, parsed.position, parsed.direction, parsed.velocity);
}

void HostNetwork::BroadcastFullPlayerList() {
    auto& players = playerManager->GetPlayers();
    for (const auto& pair : players) {
        const RemotePlayer& rp = pair.second;
        std::string msg = PlayerMessageHandler::FormatConnectionMessage(rp.playerID, rp.baseName, rp.cubeColor, rp.isReady, rp.isHost);
        game->GetNetworkManager().BroadcastMessage(msg);
    }
}

void HostNetwork::BroadcastPlayersList() {
    auto& players = playerManager->GetPlayers();
    for (auto& pair : players) {
        std::string msg = PlayerMessageHandler::FormatMovementMessage(pair.first, pair.second.player.GetPosition());
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

void HostNetwork::ProcessPlayerDeathMessage(Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender) {
    std::string playerID = parsed.steamID;
    std::string killerID = parsed.killerID;
    auto& players = playerManager->GetPlayers();
    if (players.find(playerID) != players.end()) {
        RemotePlayer& player = players[playerID];
        player.player.TakeDamage(player.player.GetHealth()); // Take full damage to ensure death
        player.respawnTimer = RESPAWN_TIME;
    }
    if (players.find(killerID) != players.end()) {
        playerManager->IncrementPlayerKills(killerID);
    }
    std::string deathMsg = PlayerMessageHandler::FormatPlayerDeathMessage(playerID, killerID);
    game.GetNetworkManager().BroadcastMessage(deathMsg);
}

void HostNetwork::ProcessPlayerRespawnMessage(Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender) {
    std::string playerID = parsed.steamID;
    sf::Vector2f respawnPos = parsed.position;
    auto& players = playerManager->GetPlayers();
    if (players.find(playerID) != players.end()) {
        RemotePlayer& player = players[playerID];
        player.player.SetRespawnPosition(respawnPos);
        player.player.Respawn();
    }
    std::string respawnMsg = PlayerMessageHandler::FormatPlayerRespawnMessage(playerID, respawnPos);
    game.GetNetworkManager().BroadcastMessage(respawnMsg);
}

void HostNetwork::ProcessStartGameMessage(Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender) {
    std::cout << "[HOST] Received start game message, changing to Playing state\n";
    if (game.GetCurrentState() != GameState::Playing) {
        game.SetCurrentState(GameState::Playing);
    }
}

void HostNetwork::ProcessPlayerDamageMessage(Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender) {
    std::cout << "[HOST] Received player damage message for player " << parsed.steamID << "\n";
}

void HostNetwork::ProcessUnknownMessage(Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender) {
    std::cout << "[HOST] Unknown message type received\n";
}

void HostNetwork::ProcessForceFieldZapMessage(Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender) {
    std::string zapperID = parsed.steamID;
    int enemyId = parsed.enemyId;
    float damage = parsed.damage;
    
    // Normalize player ID for consistent comparison
    std::string normalizedZapperID;
    try {
        uint64_t zapperIdNum = std::stoull(zapperID);
        normalizedZapperID = std::to_string(zapperIdNum);
    } catch (const std::exception& e) {
        std::cout << "[HOST] Error normalizing zapper ID: " << e.what() << "\n";
        normalizedZapperID = zapperID;
    }
    
    // Get the local player ID
    std::string localID = std::to_string(game.GetLocalSteamID().ConvertToUint64());
    std::string normalizedLocalID;
    try {
        uint64_t localIdNum = std::stoull(localID);
        normalizedLocalID = std::to_string(localIdNum);
    } catch (const std::exception& e) {
        std::cout << "[HOST] Error normalizing local ID: " << e.what() << "\n";
        normalizedLocalID = localID;
    }
    
    // Get the playing state to access the enemy manager
    PlayingState* playingState = GetPlayingState(&game);
    if (playingState && playingState->GetEnemyManager()) {
        EnemyManager* enemyManager = playingState->GetEnemyManager();
        
        // Apply damage to the enemy
        bool killed = false;
        Enemy* enemy = enemyManager->FindEnemy(enemyId);
        if (enemy) {
            killed = enemyManager->InflictDamage(enemyId, damage);
            
            // If the enemy was killed, use centralized kill handling
            if (killed) {
                playerManager->HandleKill(normalizedZapperID, enemyId);
            }
            
            // Apply visual effect if this isn't our own zap
            if (normalizedZapperID != normalizedLocalID) {
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
            }
        }
        
        // Broadcast this zap to all clients (even if we don't find the enemy)
        std::string zapMsg = PlayerMessageHandler::FormatForceFieldZapMessage(normalizedZapperID, enemyId, damage);
        game.GetNetworkManager().BroadcastMessage(zapMsg);
    }
}