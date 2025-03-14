#include "Host.h"
#include "../Game.h"
#include "../network/NetworkManager.h"
#include "../utils/MessageHandler.h"
#include "../states/PlayingState.h"
#include <iostream>

HostNetwork::HostNetwork(Game* game, PlayerManager* manager)
    : game(game), playerManager(manager), lastBroadcastTime(std::chrono::steady_clock::now()) {
    // Add host to the player list
    CSteamID hostID = game->GetLocalSteamID();  // Host’s SteamID
    std::string hostIDStr = std::to_string(hostID.ConvertToUint64());
    std::string hostName = SteamFriends()->GetPersonaName();  // Host’s Steam name

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

    // Immediately broadcast the full player list to ensure clients get the host’s data
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
    
    // Validate the bullet data before broadcasting
    if (parsed.direction.x == 0.f && parsed.direction.y == 0.f) {
        std::cout << "[HOST] Received invalid bullet direction, ignoring\n";
        return;
    }
    
    // Broadcast this bullet to all clients (including the one who sent it)
    // This ensures all clients see all bullets
    std::string broadcastMsg = MessageHandler::FormatBulletMessage(
        parsed.steamID, parsed.position, parsed.direction, parsed.velocity);
    
    bool sent = game->GetNetworkManager().BroadcastMessage(broadcastMsg);
    if (sent) {
        std::cout << "[HOST] Broadcast bullet from " << parsed.steamID << "\n";
    } else {
        std::cout << "[HOST] Failed to broadcast bullet message\n";
    }
    
    // Skip adding bullets that were fired by the host (local player)
    // since they've already been added when the host shot
    if (parsed.steamID == localSteamIDStr) {
        return;
    }
    
    // For bullets from other players, add them to the host's game
    playerManager->AddBullet(parsed.steamID, parsed.position, parsed.direction, parsed.velocity);
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
    // Removed "R" key logic, handled in LobbyState
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
    
    std::cout << "[HOST] Broadcasting player death: " << playerID << " killed by " << killerID << "\n";
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
    
    std::cout << "[HOST] Broadcasting player respawn: " << playerID << " at position " << respawnPos.x << "," << respawnPos.y << "\n";
}


void HostNetwork::ProcessEnemyHitMessage(const ParsedMessage& parsed) {
    // Access the PlayingState's EnemyManager
    PlayingState* playingState = GetPlayingState(game);
    if (playingState) {
        EnemyManager* enemyManager = playingState->GetEnemyManager();
        if (enemyManager) {
            // Process the enemy hit
            enemyManager->HandleEnemyHit(parsed.enemyId, parsed.damage, parsed.killed);
            
            // Broadcast the hit to all clients
            std::string hitMsg = MessageHandler::FormatEnemyHitMessage(
                parsed.enemyId, parsed.damage, parsed.killed, parsed.steamID);
            game->GetNetworkManager().BroadcastMessage(hitMsg);
            
            // If enemy was killed, handle rewards
            if (parsed.killed) {
                // Find the player that shot the enemy
                auto& players = playerManager->GetPlayers();
                auto it = players.find(parsed.steamID);
                if (it != players.end()) {
                    // Award kill and money
                    playerManager->IncrementPlayerKills(parsed.steamID);
                    it->second.money += 10; // Award money for kill
                    
                    // Broadcast enemy death message
                    std::string deathMsg = MessageHandler::FormatEnemyDeathMessage(
                        parsed.enemyId, parsed.steamID, true);
                    game->GetNetworkManager().BroadcastMessage(deathMsg);
                }
            }
        }
    }
}

void HostNetwork::ProcessEnemyDeathMessage(const ParsedMessage& parsed) {
    // Get the PlayingState and its EnemyManager
    if (game->GetCurrentState() == GameState::Playing) {
        PlayingState* playingState = dynamic_cast<PlayingState*>(game->GetState());
        if (playingState) {
            EnemyManager* enemyManager = playingState->GetEnemyManager();
            if (enemyManager) {
                // Handle enemy death
                enemyManager->RemoveEnemy(parsed.enemyId);
                
                // If this was a rewarded kill, increment player stats
                if (parsed.rewardKill && !parsed.killerID.empty()) {
                    auto& players = playerManager->GetPlayers();
                    auto it = players.find(parsed.killerID);
                    if (it != players.end()) {
                        playerManager->IncrementPlayerKills(parsed.killerID);
                        it->second.money += 10; // Award money for kill
                    }
                }
                
                // Broadcast to ensure all clients know
                std::string deathMsg = MessageHandler::FormatEnemyDeathMessage(
                    parsed.enemyId, parsed.killerID, parsed.rewardKill);
                game->GetNetworkManager().BroadcastMessage(deathMsg);
            }
        }
    }
}

void HostNetwork::ProcessWaveStartMessage(const ParsedMessage& parsed) {
    // Get the PlayingState and its EnemyManager
    if (game->GetCurrentState() == GameState::Playing) {
        PlayingState* playingState = dynamic_cast<PlayingState*>(game->GetState());
        if (playingState) {
            EnemyManager* enemyManager = playingState->GetEnemyManager();
            if (enemyManager) {
                // Start the wave if we're the host
                if (SteamUser()->GetSteamID() == SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID())) {
                    enemyManager->StartNextWave();
                    
                    // Broadcast wave start to ensure all clients know
                    std::string waveMsg = MessageHandler::FormatWaveStartMessage(
                        enemyManager->GetCurrentWave());
                    game->GetNetworkManager().BroadcastMessage(waveMsg);
                }
            }
        }
    }
}

void HostNetwork::ProcessWaveCompleteMessage(const ParsedMessage& parsed) {
    // Wave complete messages are primarily for clients
    // But we'll process it anyway for consistency
    std::cout << "[HOST] Wave " << parsed.waveNumber << " complete\n";
    
    // Broadcast to ensure all clients know
    std::string waveMsg = MessageHandler::FormatWaveCompleteMessage(parsed.waveNumber);
    game->GetNetworkManager().BroadcastMessage(waveMsg);
}