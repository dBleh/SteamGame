#include "Client.h"
#include "../Game.h"
#include "../network/NetworkManager.h"
#include "../utils/MessageHandler.h"
#include "../states/PlayingState.h"
#include <iostream>

ClientNetwork::ClientNetwork(Game* game, PlayerManager* manager)
    : game(game), playerManager(manager), lastSendTime(std::chrono::steady_clock::now()) {
    hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
}

ClientNetwork::~ClientNetwork() {}

void ClientNetwork::ProcessMessage(const std::string& msg, CSteamID sender) {
    ParsedMessage parsed = MessageHandler::ParseMessage(msg);
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
void ClientNetwork::ProcessBulletMessage(const ParsedMessage& parsed) {
    // Get local player ID
    std::string localSteamIDStr = std::to_string(SteamUser()->GetSteamID().ConvertToUint64());
    
    // Skip adding bullets that were fired by the local player
    // (we already added them when we shot)
    if (parsed.steamID == localSteamIDStr) {
        return;
    }
    
    playerManager->AddBullet(parsed.steamID, parsed.position, parsed.direction, parsed.velocity);
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
    auto& players = playerManager->GetPlayers();
    if (players.find(parsed.steamID) != players.end()) {
        RemotePlayer& player = players[parsed.steamID];
        player.player.SetRespawnPosition(parsed.position);
        player.player.Respawn();
    }
}

void ClientNetwork::ProcessEnemySpawnMessage(const ParsedMessage& parsed) {
    // Access the PlayingState's EnemyManager through the Game
    PlayingState* playingState = GetPlayingState(game);
    if (playingState) {
        EnemyManager* enemyManager = playingState->GetEnemyManager();
        if (enemyManager) {
            // Spawn the enemy
            enemyManager->AddEnemy(parsed.enemyId, parsed.position);
            std::cout << "[CLIENT] Spawned enemy " << parsed.enemyId << " at position (" 
                      << parsed.position.x << ", " << parsed.position.y << ")\n";
        }
    }
}

void ClientNetwork::ProcessEnemyHitMessage(const ParsedMessage& parsed) {
    // Access the PlayingState's EnemyManager
    PlayingState* playingState = GetPlayingState(game);
    if (playingState) {
        EnemyManager* enemyManager = playingState->GetEnemyManager();
        if (enemyManager) {
            // Process the enemy hit
            enemyManager->HandleEnemyHit(parsed.enemyId, parsed.damage, parsed.killed);
            std::cout << "[CLIENT] Enemy " << parsed.enemyId << " took " << parsed.damage 
                      << " damage from " << parsed.steamID << (parsed.killed ? " and was killed" : "") << "\n";
        }
    }
}

void ClientNetwork::ProcessEnemyDeathMessage(const ParsedMessage& parsed) {
    // Get the PlayingState and its EnemyManager
    if (game->GetCurrentState() == GameState::Playing) {
        PlayingState* playingState = dynamic_cast<PlayingState*>(game->GetState());
        if (playingState) {
            EnemyManager* enemyManager = playingState->GetEnemyManager();
            if (enemyManager) {
                // Handle enemy death
                enemyManager->RemoveEnemy(parsed.enemyId);
                
                // If this was a rewarded kill for the local player, update UI
                std::string localSteamIDStr = std::to_string(SteamUser()->GetSteamID().ConvertToUint64());
                if (parsed.rewardKill && parsed.killerID == localSteamIDStr) {
                    std::cout << "[CLIENT] Local player killed enemy " << parsed.enemyId << "\n";
                    // UI updates will happen through PlayerManager
                }
            }
        }
    }
}

void ClientNetwork::ProcessWaveStartMessage(const ParsedMessage& parsed) {
    // Get the PlayingState and its EnemyManager
    if (game->GetCurrentState() == GameState::Playing) {
        PlayingState* playingState = dynamic_cast<PlayingState*>(game->GetState());
        if (playingState) {
            EnemyManager* enemyManager = playingState->GetEnemyManager();
            if (enemyManager) {
                // As a client, we don't spawn enemies, but we update our wave number
                if (parsed.waveNumber > enemyManager->GetCurrentWave()) {
                    std::cout << "[CLIENT] Starting wave " << parsed.waveNumber << "\n";
                    // The enemies will be spawned via EnemySpawn messages from the host
                }
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