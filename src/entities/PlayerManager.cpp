#include "PlayerManager.h"
#include "../Game.h"
#include <iostream>

PlayerManager::PlayerManager(Game* game, const std::string& localID)
    : game(game), localPlayerID(localID), lastFrameTime(std::chrono::steady_clock::now()) {
}

PlayerManager::~PlayerManager() {
}

void PlayerManager::Update() {
    auto now = std::chrono::steady_clock::now();
    float dt = std::chrono::duration<float>(now - lastFrameTime).count();
    lastFrameTime = now;

    for (auto& pair : players) {
        RemotePlayer& rp = pair.second;

        if (rp.player.IsDead() && rp.respawnTimer > 0.0f) {
            rp.respawnTimer -= dt;
            if (rp.respawnTimer <= 0.0f) {
                // Set respawn position to 0,0
                rp.player.SetRespawnPosition(sf::Vector2f(0.f, 0.f));
                RespawnPlayer(pair.first);
            }
        }

        
        sf::Vector2f pos;
        if (pair.first == localPlayerID) {
            rp.player.Update(dt);  // Local player updates with input
            pos = rp.player.GetPosition();
        } else {
            float elapsed = std::chrono::duration<float>(now - rp.lastUpdateTime).count();
            float t = elapsed / rp.interpDuration;
            if (t > 1.0f) t = 1.0f;
            pos = rp.previousPosition + (rp.targetPosition - rp.previousPosition) * t;
            rp.player.SetPosition(pos);
        }
        std::string status = rp.isReady ? " ✓" : " X";
        rp.nameText.setString(rp.baseName + status);
        rp.nameText.setPosition(pos.x, pos.y - 20.f);
    }

    std::vector<size_t> bulletsToRemove;
    
    for (size_t i = 0; i < bullets.size(); i++) {
        bullets[i].Update(dt);
        if (bullets[i].IsExpired()) {
            bulletsToRemove.push_back(i);
        }
    }
    
    // Remove expired bullets safely (from back to front)
    for (int i = bulletsToRemove.size() - 1; i >= 0; i--) {
        size_t idx = bulletsToRemove[i];
        if (idx < bullets.size()) {
            bullets.erase(bullets.begin() + idx);
        }
    }
    CheckBulletCollisions();
}

void PlayerManager::AddOrUpdatePlayer(const std::string& id, const RemotePlayer& player) {
    if (id.empty()) {
        std::cout << "[ERROR] Attempted to add player with empty ID!\n";
        return;
    }
    auto now = std::chrono::steady_clock::now();
    if (players.find(id) == players.end()) {
        players[id] = player;
        players[id].playerID = id;
        players[id].previousPosition = player.player.GetPosition();
        players[id].targetPosition = player.player.GetPosition();
        players[id].lastUpdateTime = now;
        players[id].baseName = player.nameText.getString().toAnsiString();
    } else if (id != localPlayerID) {
        players[id].previousPosition = players[id].player.GetPosition();
        players[id].targetPosition = player.player.GetPosition();
        players[id].lastUpdateTime = now;
        players[id].player = player.player;
        players[id].cubeColor = player.cubeColor;
        players[id].isHost = player.isHost;
        players[id].nameText = player.nameText;
    }
}

void PlayerManager::AddLocalPlayer(const std::string& id, const std::string& name, const sf::Vector2f& position, const sf::Color& color) {
    RemotePlayer rp;
    rp.player = Player(position, color);
    rp.nameText.setFont(game->GetFont());
    rp.nameText.setString(name);
    rp.baseName = name;
    rp.nameText.setCharacterSize(16);
    rp.nameText.setFillColor(sf::Color::Black);
    rp.previousPosition = position;
    rp.targetPosition = position;
    rp.lastUpdateTime = std::chrono::steady_clock::now();
    rp.interpDuration = 0.1f;
    players[id] = rp;
    localPlayerID = id;
}

void PlayerManager::SetReadyStatus(const std::string& id, bool ready) {
    std::cout << "ready status set " << std::endl;
    if (players.find(id) != players.end()) {
        players[id].isReady = ready;
        std::cout << "[PM] Set ready status for " << id << " to " << (ready ? "true" : "false") << "\n";
        std::string status = ready ? " ✓" : " X";
        players[id].nameText.setString(players[id].baseName + status);
    } else {
        std::cout << "[PM] Player " << id << " not found for ready status update\n";
    }
}

void PlayerManager::AddBullet(const std::string& shooterID, const sf::Vector2f& position, const sf::Vector2f& direction, float velocity) {
    bullets.emplace_back(position, direction, velocity, shooterID);
    std::cout << "[PM] Added bullet from " << shooterID << " at " << position.x << "," << position.y << "\n";
}

RemotePlayer& PlayerManager::GetLocalPlayer() {
    if (players.find(localPlayerID) == players.end()) {
        std::cerr << "[ERROR] Local player not found!" << std::endl;
        // Create a temporary player to avoid crash
        static RemotePlayer defaultPlayer;
        return defaultPlayer;
    }
    return players[localPlayerID];
}

void PlayerManager::RemovePlayer(const std::string& id) {
    players.erase(id);
}

std::unordered_map<std::string, RemotePlayer>& PlayerManager::GetPlayers() {
    return players;
}

void PlayerManager::CheckBulletCollisions() {
    for (auto bulletIt = bullets.begin(); bulletIt != bullets.end();) {
        bool bulletHit = false;
        
        for (auto& playerPair : players) {
            std::string playerID = playerPair.first;
            RemotePlayer& remotePlayer = playerPair.second;
            
            // Skip dead players
            if (remotePlayer.player.IsDead()) continue;
            
            // Check if bullet belongs to this player (can't shoot yourself)
            if (bulletIt->BelongsToPlayer(playerID)) continue;
            
            if (bulletIt->CheckCollision(remotePlayer.player.GetShape(), playerID)) {
                remotePlayer.player.TakeDamage(25); // 4 hits to kill
                
                if (remotePlayer.player.IsDead()) {
                    PlayerDied(playerID, bulletIt->GetShooterID());
                }
                
                bulletHit = true;
                break;
            }
        }
        
        if (bulletHit) {
            bulletIt = bullets.erase(bulletIt);
        } else {
            ++bulletIt;
        }
    }
}

void PlayerManager::PlayerDied(const std::string& playerID, const std::string& killerID) {
    std::cout << "[PM] Player " << playerID << " was killed by " << killerID << "\n";
    
    // Schedule respawn after 3 seconds
    players[playerID].respawnTimer = 3.0f;
    
    // If this is the local player, notify the network
    if (playerID == localPlayerID) {
        // Check if we're the host by comparing with the lobby owner
        CSteamID localSteamID = SteamUser()->GetSteamID();
        CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
        
        if (localSteamID == hostID) {
            // We are the host, broadcast to all clients
            std::string deathMsg = MessageHandler::FormatPlayerDeathMessage(playerID, killerID);
            game->GetNetworkManager().BroadcastMessage(deathMsg);
        } else {
            // We are a client, send to host
            std::string deathMsg = MessageHandler::FormatPlayerDeathMessage(playerID, killerID);
            game->GetNetworkManager().SendMessage(hostID, deathMsg);
        }
    }
}

void PlayerManager::RespawnPlayer(const std::string& playerID) {
    if (players.find(playerID) != players.end()) {
        players[playerID].player.Respawn();
        
        // If this is the local player, notify the network of respawn
        if (playerID == localPlayerID) {
            sf::Vector2f respawnPos = players[playerID].player.GetPosition();
            
            // Check if we're the host by comparing with the lobby owner
            CSteamID localSteamID = SteamUser()->GetSteamID();
            CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
            
            if (localSteamID == hostID) {
                // We are the host, broadcast to all clients
                std::string respawnMsg = MessageHandler::FormatPlayerRespawnMessage(playerID, respawnPos);
                game->GetNetworkManager().BroadcastMessage(respawnMsg);
            } else {
                // We are a client, send to host
                std::string respawnMsg = MessageHandler::FormatPlayerRespawnMessage(playerID, respawnPos);
                game->GetNetworkManager().SendMessage(hostID, respawnMsg);
            }
        }
    }
}