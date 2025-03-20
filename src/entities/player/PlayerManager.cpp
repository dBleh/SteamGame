#include "PlayerManager.h"
#include "../../core/Game.h"
#include "../../network/messages/MessageHandler.h"
#include "../../network/messages/PlayerMessageHandler.h"
#include "../../network/messages/EnemyMessageHandler.h"
#include "../../network/messages/StateMessageHandler.h"
#include "../../network/messages/SystemMessageHandler.h"
#include "ForceField.h"
#include "../../network/NetworkManager.h"

#include <iostream>
#include <algorithm>

PlayerManager::PlayerManager(Game* game, const std::string& localID)
    : game(game), localPlayerID(localID), lastFrameTime(std::chrono::steady_clock::now()) {
}

PlayerManager::~PlayerManager() {
    // Clean up if needed
}

void PlayerManager::Update(Game* game) {
    auto now = std::chrono::steady_clock::now();
    float dt = std::chrono::duration<float>(now - lastFrameTime).count();
    lastFrameTime = now;

    Update(dt, game);
}

void PlayerManager::Update() {
    // Legacy method for backward compatibility
    Update(game);
}

void PlayerManager::Update(float dt, Game* game) {
    UpdatePlayers(dt, game);
    UpdateBullets(dt);
    CheckBulletCollisions();
}

void PlayerManager::UpdatePlayers(float dt, Game* game) {
    for (auto& pair : players) {
        std::string playerID = pair.first;
        RemotePlayer& rp = pair.second;

        // Let the player update itself (handles cooldowns and respawn timer)
        rp.player.Update(dt);
        
        // Update player position based on whether it's local or remote
        if (playerID == localPlayerID) {
            // For local player, use the InputManager for movement
            rp.player.Update(dt, game->GetInputManager());
            sf::Vector2f playerPos = rp.player.GetPosition();
            rp.nameText.setPosition(playerPos.x, playerPos.y - 20.f);
        } else {
            // For remote players, use interpolation
            UpdateRemotePlayerPosition(dt, playerID, rp);
        }
        
        // Update player name display
        UpdatePlayerNameDisplay(playerID, rp, game);
    }
}

void PlayerManager::UpdateRemotePlayerPosition(float dt, const std::string& playerID, RemotePlayer& rp) {
    auto now = std::chrono::steady_clock::now();
    float elapsed = std::chrono::duration<float>(now - rp.lastUpdateTime).count();
    float t = elapsed / rp.interpDuration;
    if (t > 1.0f) t = 1.0f;
    sf::Vector2f pos = rp.previousPosition + (rp.targetPosition - rp.previousPosition) * t;
    rp.player.SetPosition(pos);
    
    // Update player name position to follow the player
    rp.nameText.setPosition(pos.x, pos.y - 20.f);
}

void PlayerManager::UpdatePlayerNameDisplay(const std::string& playerID, RemotePlayer& rp, Game* game) {
    // Only show ready status in Lobby state
    if (game->GetCurrentState() == GameState::Lobby) {
        std::string status = rp.isReady ? " ✓" : " X";
        rp.nameText.setString(rp.baseName + status);
    } else {
        rp.nameText.setString(rp.baseName);
    }
}

void PlayerManager::UpdateBullets(float dt) {
    std::vector<size_t> bulletsToRemove;
    
    for (size_t i = 0; i < bullets.size(); i++) {
        bullets[i].Update(dt);
        
        // Mark expired bullets or those far away from all players
        if (bullets[i].IsExpired()) {
            bulletsToRemove.push_back(i);
        } else {
            // Also remove bullets that are far away from any player
            bool tooFarFromAllPlayers = true;
            sf::Vector2f bulletPos = bullets[i].GetPosition();
            
            for (const auto& pair : players) {
                if (pair.second.player.IsDead()) continue;
                
                sf::Vector2f playerPos = pair.second.player.GetPosition();
                float distSquared = (bulletPos.x - playerPos.x) * (bulletPos.x - playerPos.x) +
                                    (bulletPos.y - playerPos.y) * (bulletPos.y - playerPos.y);
                
                // If bullet is within reasonable distance of any player, keep it
                if (distSquared < 1000*1000) {  // 1000 pixels
                    tooFarFromAllPlayers = false;
                    break;
                }
            }
            
            if (tooFarFromAllPlayers) {
                bulletsToRemove.push_back(i);
            }
        }
    }
    
    // Remove expired bullets safely (from back to front)
    RemoveBullets(bulletsToRemove);
}

void PlayerManager::AddOrUpdatePlayer(const std::string& id, const RemotePlayer& player) {
    if (id.empty()) {
        std::cout << "[ERROR] Attempted to add player with empty ID!\n";
        return;
    }
    
    auto now = std::chrono::steady_clock::now();
    
    if (players.find(id) == players.end()) {
        // New player - create using constructor, not copy
        RemotePlayer newPlayer;
        newPlayer.playerID = id;
        newPlayer.isHost = player.isHost;
        // Create a new player with the same parameters, don't copy
        newPlayer.player = Player(player.player.GetPosition(), player.cubeColor);
        newPlayer.nameText = player.nameText;
        newPlayer.cubeColor = player.cubeColor;
        newPlayer.previousPosition = player.player.GetPosition();
        newPlayer.targetPosition = player.player.GetPosition();
        newPlayer.lastUpdateTime = now;
        newPlayer.baseName = player.nameText.getString().toAnsiString();
        newPlayer.kills = player.kills;
        newPlayer.money = player.money;
        
        // Initialize player callbacks
        InitializePlayerCallbacks(newPlayer, id);
        
        // Insert the new player
        players[id] = std::move(newPlayer);
    } else if (id != localPlayerID) {
        // Update existing remote player - update fields individually, don't copy the Player object
        players[id].previousPosition = players[id].player.GetPosition();
        players[id].targetPosition = player.player.GetPosition();
        players[id].lastUpdateTime = now;
        players[id].player.SetPosition(player.player.GetPosition());
        players[id].cubeColor = player.cubeColor;
        players[id].isHost = player.isHost;
        players[id].nameText = player.nameText;
        // Don't override stats when updating position
    }
}

void PlayerManager::AddOrUpdatePlayer(const std::string& id, RemotePlayer&& player) {
    if (id.empty()) {
        std::cout << "[ERROR] Attempted to add player with empty ID!\n";
        return;
    }
    
    auto now = std::chrono::steady_clock::now();
    
    if (players.find(id) == players.end()) {
        // New player - move it directly into the map
        player.previousPosition = player.player.GetPosition();
        player.targetPosition = player.player.GetPosition();
        player.lastUpdateTime = now;
        
        // Initialize player callbacks
        InitializePlayerCallbacks(player, id);
        
        // Move the player into the map
        players[id] = std::move(player);
    } else if (id != localPlayerID) {
        // Update existing remote player
        // We can't move the whole player since it exists, so update fields
        players[id].previousPosition = players[id].player.GetPosition();
        players[id].targetPosition = player.player.GetPosition();
        players[id].lastUpdateTime = now;
        players[id].player.SetPosition(player.player.GetPosition());
        players[id].cubeColor = player.cubeColor;
        players[id].isHost = player.isHost;
        players[id].nameText = player.nameText;
        // Don't override stats when updating position
    }
}

void PlayerManager::AddLocalPlayer(const std::string& id, const std::string& name, const sf::Vector2f& position, const sf::Color& color) {
    RemotePlayer rp;
    // Create a new Player directly, don't copy
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
    rp.kills = 0;
    rp.money = 0;
    rp.cubeColor = color;
    rp.playerID = id;
    
    // Initialize player callbacks
    InitializePlayerCallbacks(rp, id);
    
    // Move to the map (don't copy)
    players[id] = std::move(rp);
    localPlayerID = id;
}

void PlayerManager::InitializePlayerCallbacks(RemotePlayer& rp, const std::string& playerID) {
    // Set the player ID
    rp.player.SetPlayerID(playerID);
    
    // Set up death callback
    rp.player.SetDeathCallback([this](const std::string& id, const sf::Vector2f& pos, const std::string& killerID) {
        HandlePlayerDeath(id, pos, killerID);
    });
    
    // Set up respawn callback
    rp.player.SetRespawnCallback([this](const std::string& id, const sf::Vector2f& pos) {
        HandlePlayerRespawn(id, pos);
    });
    
    // Set up damage callback
    rp.player.SetDamageCallback([this](const std::string& id, int amount, int actualDamage) {
        HandlePlayerDamage(id, amount, actualDamage);
    });
}

void PlayerManager::SetReadyStatus(const std::string& id, bool ready) {
    if (players.find(id) != players.end()) {
        players[id].isReady = ready;        
        // Only update the visible name with ready status if in Lobby state
        if (game->GetCurrentState() == GameState::Lobby) {
            std::string status = ready ? " ✓" : " X";
            players[id].nameText.setString(players[id].baseName + status);
        }
    }
}

bool PlayerManager::AreAllPlayersReady() const {
    for (const auto& pair : players) {
        if (!pair.second.isReady) {
            return false;
        }
    }
    return !players.empty(); // Only true if there are players and all are ready
}

void PlayerManager::AddBullet(const std::string& shooterID, const sf::Vector2f& position, const sf::Vector2f& direction, float velocity) {
    // Validate input parameters
    if (direction.x == 0.f && direction.y == 0.f) {
        return;
    }
    
    if (shooterID.empty()) {
        return;
    }
    
    // Ensure we use the exact same string format for IDs
    std::string normalizedID = shooterID;
    try {
        // Convert to uint64, then back to string to normalize format
        uint64_t idNum = std::stoull(shooterID);
        normalizedID = std::to_string(idNum);
    } catch (const std::exception& e) {
        std::cout << "[PM] Error normalizing shooter ID: " << e.what() << "\n";
    }
    
    // Apply bullet speed multiplier from player if available
    float adjustedVelocity = velocity;
    auto it = players.find(normalizedID);
    if (it != players.end()) {
        adjustedVelocity *= it->second.player.GetBulletSpeedMultiplier();
    }
    
    // Add the bullet with the normalized ID
    bullets.emplace_back(position, direction, adjustedVelocity, normalizedID);
}

bool PlayerManager::PlayerShoot(const sf::Vector2f& mouseWorldPos) {
    // Skip if the local player is dead
    auto& localPlayer = GetLocalPlayer();
    if (localPlayer.player.IsDead()) return false;
    
    // Call the player's AttemptShoot method
    Player::BulletParams bulletParams = localPlayer.player.AttemptShoot(mouseWorldPos);
    
    // If shooting was successful, create a bullet and send network message
    if (bulletParams.success) {
        // Apply bullet speed multiplier
        float bulletSpeed = BULLET_SPEED * localPlayer.player.GetBulletSpeedMultiplier();
        
        // Add the bullet locally
        AddBullet(localPlayerID, bulletParams.position, bulletParams.direction, bulletSpeed);
        
        // Send the bullet creation to other players via network
        SendBulletMessageToNetwork(bulletParams.position, bulletParams.direction, bulletSpeed);
        
        return true;
    }
    
    return false;
}

void PlayerManager::SendBulletMessageToNetwork(const sf::Vector2f& position, const sf::Vector2f& direction, float bulletSpeed) {
    // Create the bullet message
    std::string bulletMsg = PlayerMessageHandler::FormatBulletMessage(
        localPlayerID, position, direction, bulletSpeed);
    
    // Check if we're the host by comparing with the lobby owner
    CSteamID localSteamID = SteamUser()->GetSteamID();
    CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
    
    if (localSteamID == hostID) {
        // We are the host, broadcast to all clients
        game->GetNetworkManager().BroadcastMessage(bulletMsg);
    } else {
        // We are a client, send to host
        game->GetNetworkManager().SendMessage(hostID, bulletMsg);
    }
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

void PlayerManager::IncrementPlayerKills(const std::string& playerID) {
    // Normalize player ID for consistent comparison
    std::string normalizedPlayerID;
    try {
        uint64_t idNum = std::stoull(playerID);
        normalizedPlayerID = std::to_string(idNum);
    } catch (const std::exception& e) {
        std::cout << "[PM] Error normalizing player ID in IncrementPlayerKills: " << e.what() << "\n";
        normalizedPlayerID = playerID;
    }
    
    if (players.find(normalizedPlayerID) != players.end()) {
        int oldKills = players[normalizedPlayerID].kills;
        players[normalizedPlayerID].kills++;
        
        // Also reward the player with some money
        players[normalizedPlayerID].money += 50;
        
        // Enhanced logging
        std::cout << "[KILL TRACKING] Incremented kills for " << normalizedPlayerID 
                  << " from " << oldKills << " to " << players[normalizedPlayerID].kills 
                  << " - Player name: " << players[normalizedPlayerID].baseName 
                  << " - Is local: " << (normalizedPlayerID == localPlayerID ? "YES" : "NO")
                  << " - Is host: " << (players[normalizedPlayerID].isHost ? "YES" : "NO") << "\n";
    } else {
        std::cout << "[PM] Could not find player " << normalizedPlayerID << " to increment kills\n";
        
        // Dump all player IDs for debugging
        std::cout << "[PM] Current players: ";
        for (const auto& pair : players) {
            std::cout << pair.first << " (" << pair.second.baseName << "), ";
        }
        std::cout << "\n";
    }
}

const std::vector<Bullet>& PlayerManager::GetAllBullets() const {
    return bullets;
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
            
            // Use the Player's collision detection method
            if (remotePlayer.player.CheckBulletCollision(bulletIt->GetPosition(), BULLET_RADIUS)) {
                // Apply damage to player, passing shooter ID for kill credit
                remotePlayer.player.TakeDamage(BULLET_DAMAGE, bulletIt->GetShooterID());
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

void PlayerManager::HandlePlayerDeath(const std::string& playerID, const sf::Vector2f& position, const std::string& killerID) {
    std::cout << "[PlayerManager] Player " << playerID << " died at position (" 
              << position.x << "," << position.y 
              << "), killed by " << killerID << std::endl;
    
    // If this is the local player, notify the network
    if (playerID == localPlayerID) {
        BroadcastPlayerDeath(playerID, position, killerID);
    }
    
    // If a killer was specified, increment their kill count
    if (!killerID.empty()) {
        IncrementPlayerKills(killerID);
    }
}

void PlayerManager::BroadcastPlayerDeath(const std::string& playerID, const sf::Vector2f& position, const std::string& killerID) {
    // Check if we're the host by comparing with the lobby owner
    CSteamID localSteamID = SteamUser()->GetSteamID();
    CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
    
    std::string deathMsg = PlayerMessageHandler::FormatPlayerDeathMessage(playerID, killerID);
    
    if (localSteamID == hostID) {
        // We are the host, broadcast to all clients
        game->GetNetworkManager().BroadcastMessage(deathMsg);
    } else {
        // We are a client, send to host
        game->GetNetworkManager().SendMessage(hostID, deathMsg);
    }
}

void PlayerManager::HandlePlayerRespawn(const std::string& playerID, const sf::Vector2f& position) {
    std::cout << "[PlayerManager] Player " << playerID << " respawned at position (" 
              << position.x << "," << position.y << ")" << std::endl;
    
    // If this is the local player, notify the network
    if (playerID == localPlayerID) {
        BroadcastPlayerRespawn(playerID, position);
    }
}

void PlayerManager::BroadcastPlayerRespawn(const std::string& playerID, const sf::Vector2f& position) {
    // Check if we're the host by comparing with the lobby owner
    CSteamID localSteamID = SteamUser()->GetSteamID();
    CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
    
    std::string respawnMsg = PlayerMessageHandler::FormatPlayerRespawnMessage(playerID, position);
    
    if (localSteamID == hostID) {
        // We are the host, broadcast to all clients
        game->GetNetworkManager().BroadcastMessage(respawnMsg);
    } else {
        // We are a client, send to host
        game->GetNetworkManager().SendMessage(hostID, respawnMsg);
    }
}

void PlayerManager::HandlePlayerDamage(const std::string& playerID, int amount, int actualDamage) {
    // Optional: Do anything needed when a player takes damage
    // This could include playing sounds, showing visual effects, etc.
}

void PlayerManager::HandleKill(const std::string& killerID, int enemyId) {
    // Normalize player ID to ensure consistent string representation
    std::string normalizedKillerID;
    try {
        uint64_t idNum = std::stoull(killerID);
        normalizedKillerID = std::to_string(idNum);
    } catch (const std::exception& e) {
        std::cout << "[KILL] Error normalizing killer ID: " << e.what() << "\n";
        normalizedKillerID = killerID;
    }
    
    std::cout << "[PM::HandleKill] Player " << normalizedKillerID << " got kill for enemy " << enemyId << "\n";
    
    // Check if we're the host
    CSteamID localSteamID = SteamUser()->GetSteamID();
    CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
    bool isHost = (localSteamID == hostID);
    
    if (isHost) {
        // Host logic - authoritative source of kill tracking
        if (players.find(normalizedKillerID) != players.end()) {
            // Increment kill counter
            players[normalizedKillerID].kills++;
            
            // Award money for kill
            players[normalizedKillerID].money += 50;
            
            // Broadcast kill information to all clients
            std::string killMsg = PlayerMessageHandler::FormatKillMessage(normalizedKillerID, enemyId);
            game->GetNetworkManager().BroadcastMessage(killMsg);
            
            std::cout << "[HOST] Player " << normalizedKillerID << " awarded kill for enemy " << enemyId << "\n";
        }
    } else {
        // Client logic - send kill message to host for validation
        // Note: Clients don't increment their own kills or award money here
        // They wait for the host to broadcast the kill message back
        std::string killMsg = PlayerMessageHandler::FormatKillMessage(normalizedKillerID, enemyId);
        game->GetNetworkManager().SendMessage(hostID, killMsg);
        
        std::cout << "[CLIENT] Sent kill claim to host for player " << normalizedKillerID 
                  << " and enemy " << enemyId << "\n";
    }
}

void PlayerManager::RemoveBullets(const std::vector<size_t>& indicesToRemove) {
    if (indicesToRemove.empty()) {
        return;
    }
    
    // Create a sorted copy of indices (sorted in descending order)
    std::vector<size_t> sortedIndices = indicesToRemove;
    std::sort(sortedIndices.begin(), sortedIndices.end(), std::greater<size_t>());
    
    // Remove bullets from back to front to avoid invalidating indices
    for (size_t index : sortedIndices) {
        if (index < bullets.size()) {
            bullets.erase(bullets.begin() + index);
        }
    }
}

void PlayerManager::InitializeForceFields() {
    std::cout << "[PlayerManager] Initializing force fields for " << players.size() << " players" << std::endl;
    
    for (auto& pair : players) {
        std::string playerID = pair.first;
        RemotePlayer& rp = pair.second;
        
        // Initialize force field if not already done
        if (!rp.player.HasForceField()) {
            std::cout << "[PlayerManager] Creating force field for player " << rp.baseName << std::endl;
            rp.player.InitializeForceField();
            
            // Set up callback for zap events
            // Use lambda to capture playerID for the callback
            std::string capturedID = playerID; // Make a copy for the lambda capture
            rp.player.SetForceFieldZapCallback([this, capturedID](int enemyId, float damage, bool killed) {
                // Handle zap event
                HandleForceFieldZap(capturedID, enemyId, damage, killed);
            });
            
            std::cout << "[PlayerManager] Force field initialized for player " << rp.baseName << std::endl;
        } else {
            std::cout << "[PlayerManager] Player " << rp.baseName << " already has a force field" << std::endl;
        }
    }
}

void PlayerManager::HandleForceFieldZap(const std::string& playerID, int enemyId, float damage, bool killed) {
    // Only update rewards for hits, not kills (as those are handled by HandleKill)
    if (!killed) {
        // Reward for hits (not kills)
        auto it = players.find(playerID);
        if (it != players.end()) {
            it->second.money += 10; // 10 money for hitting an enemy with force field
        }
    } else {
        // If the enemy was killed, use the centralized kill handling
        HandleKill(playerID, enemyId);
    }
    
    // If this is the local player, send a network message
    if (playerID == localPlayerID) {
        // Format and send force field zap message
        std::string zapMsg = PlayerMessageHandler::FormatForceFieldZapMessage(playerID, enemyId, damage);
        
        // Check if we're the host
        CSteamID localSteamID = SteamUser()->GetSteamID();
        CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
        
        if (localSteamID == hostID) {
            // We are the host, broadcast to all clients
            game->GetNetworkManager().BroadcastMessage(zapMsg);
        } else {
            // We are a client, send to host
            game->GetNetworkManager().SendMessage(hostID, zapMsg);
        }
    }
}

