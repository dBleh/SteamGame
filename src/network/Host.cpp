#include "Host.h"
#include "../core/Game.h"
#include "NetworkManager.h"
#include "messages/MessageHandler.h"
#include "../states/PlayingState.h"
#include <iostream>

HostNetwork::HostNetwork(Game* game, PlayerManager* manager)
    : game(game), playerManager(manager), lastBroadcastTime(std::chrono::steady_clock::now()) {
    CSteamID hostID = game->GetLocalSteamID();
    std::string hostIDStr = std::to_string(hostID.ConvertToUint64());
    std::string hostName = SteamFriends()->GetPersonaName();
    RemotePlayer hostPlayer;
    hostPlayer.playerID = hostIDStr;
    hostPlayer.isHost = true;
    hostPlayer.player = Player(sf::Vector2f(200.f, 200.f), sf::Color::Blue);
    hostPlayer.cubeColor = sf::Color::Blue;
    hostPlayer.nameText.setFont(game->GetFont());
    hostPlayer.nameText.setString(hostName);
    hostPlayer.baseName = hostName;
    hostPlayer.nameText.setCharacterSize(16);
    hostPlayer.nameText.setFillColor(sf::Color::Black);
    hostPlayer.isReady = false;
    playerManager->AddOrUpdatePlayer(hostIDStr, hostPlayer);
    std::cout << "[HOST] Added host to player list: " << hostName << " (" << hostIDStr << ")\n";
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
        rp.player = Player(sf::Vector2f(200.f, 200.f), parsed.color);
        rp.cubeColor = parsed.color;
        rp.nameText.setFont(game.GetFont());
        rp.nameText.setString(parsed.steamName);
        rp.baseName = parsed.steamName;
        rp.nameText.setCharacterSize(16);
        rp.nameText.setFillColor(sf::Color::Black);
        playerManager->AddOrUpdatePlayer(parsed.steamID, std::move(rp));
        std::cout << "[HOST] New player connected: " << parsed.steamName << " (" << parsed.steamID << ")\n";
    } else {
        // Update existing player's position
        it->second.player.SetPosition(sf::Vector2f(200.f, 200.f));
        it->second.cubeColor = parsed.color;
        if (it->second.baseName != parsed.steamName) {
            it->second.baseName = parsed.steamName;
            it->second.nameText.setString(parsed.steamName);
            std::cout << "[HOST] Updated name for " << parsed.steamID << " to " << parsed.steamName << "\n";
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
        rp.player = Player(parsed.position, sf::Color::Blue);
        rp.nameText.setFont(game.GetFont());
        rp.nameText.setCharacterSize(16);
        rp.nameText.setFillColor(sf::Color::Black);
        playerManager->AddOrUpdatePlayer(parsed.steamID, std::move(rp));
    }
    
    // Broadcast the movement to all clients
    std::string broadcastMsg = MessageHandler::FormatMovementMessage(parsed.steamID, parsed.position);
    game.GetNetworkManager().BroadcastMessage(broadcastMsg);
}

void HostNetwork::ProcessChatMessageParsed(Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender) {
    ProcessChatMessage(parsed.chatMessage, sender);
}

void HostNetwork::ProcessChatMessage(const std::string& message, CSteamID sender) {
    std::string msg = MessageHandler::FormatChatMessage(std::to_string(sender.ConvertToUint64()), message);
    game->GetNetworkManager().BroadcastMessage(msg);
}

void HostNetwork::ProcessReadyStatusMessage(Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender) {
    std::string localSteamIDStr = std::to_string(game.GetLocalSteamID().ConvertToUint64());
    if (localSteamIDStr != parsed.steamID) {
        auto& players = playerManager->GetPlayers();
        if (players.find(parsed.steamID) != players.end() && players[parsed.steamID].isReady != parsed.isReady) {
            playerManager->SetReadyStatus(parsed.steamID, parsed.isReady);
        }
    }
    std::string broadcastMsg = MessageHandler::FormatReadyStatusMessage(parsed.steamID, parsed.isReady);
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
    std::string broadcastMsg = MessageHandler::FormatBulletMessage(normalizedShooterID, parsed.position, parsed.direction, parsed.velocity);
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
        std::string msg = MessageHandler::FormatConnectionMessage(rp.playerID, rp.baseName, rp.cubeColor, rp.isReady, rp.isHost);
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

void HostNetwork::ProcessPlayerDeathMessage(Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender) {
    std::string playerID = parsed.steamID;
    std::string killerID = parsed.killerID;
    auto& players = playerManager->GetPlayers();
    if (players.find(playerID) != players.end()) {
        RemotePlayer& player = players[playerID];
        player.player.TakeDamage(100);
        player.respawnTimer = 3.0f;
    }
    if (players.find(killerID) != players.end()) {
        playerManager->IncrementPlayerKills(killerID);
    }
    std::string deathMsg = MessageHandler::FormatPlayerDeathMessage(playerID, killerID);
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
    std::string respawnMsg = MessageHandler::FormatPlayerRespawnMessage(playerID, respawnPos);
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
            
            // If the enemy was killed, update player kills
            if (killed) {
                playerManager->IncrementPlayerKills(normalizedZapperID);
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
                    sf::Vector2f playerPos = rp.player.GetPosition() + sf::Vector2f(25.0f, 25.0f); // Assuming 50x50 player
                    sf::Vector2f enemyPos = enemy->GetPosition();
                    
                    // Create the zap effect
                    rp.player.GetForceField()->CreateZapEffect(playerPos, enemyPos);
                    rp.player.GetForceField()->SetIsZapping(true);
                    rp.player.GetForceField()->SetZapEffectTimer(0.3f); // Show effect for 0.3 seconds
                }
            }
        }
        
        // Broadcast this zap to all clients (even if we don't find the enemy)
        std::string zapMsg = MessageHandler::FormatForceFieldZapMessage(normalizedZapperID, enemyId, damage);
        game.GetNetworkManager().BroadcastMessage(zapMsg);
    }
}