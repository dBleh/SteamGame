#include "PlayerManager.h"
#include "../core/Game.h"
#include "../network/messages/MessageHandler.h"
#include "../network/messages/PlayerMessageHandler.h"
#include "../network/messages/EnemyMessageHandler.h"
#include "../network/messages/StateMessageHandler.h"
#include "../network/messages/SystemMessageHandler.h"
#include "ForceField.h"

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

    UpdatePlayers(dt, game);
    UpdateBullets(dt);
    CheckBulletCollisions();
}

void PlayerManager::Update() {
    // Legacy method for backward compatibility
    Update(game);
}

void PlayerManager::UpdatePlayers(float dt, Game* game) {
    for (auto& pair : players) {
        std::string playerID = pair.first;
        RemotePlayer& rp = pair.second;

        // Handle respawning for dead players
        HandlePlayerRespawn(dt, playerID, rp);
        
        // Update player position based on whether it's local or remote
        UpdatePlayerPosition(dt, playerID, rp, game);
        
        // Update player name display
        UpdatePlayerNameDisplay(playerID, rp, game);
    }
}

void PlayerManager::HandlePlayerRespawn(float dt, const std::string& playerID, RemotePlayer& rp) {
    if (rp.player.IsDead()) {
        // Debug output (once per second)
        static std::chrono::steady_clock::time_point lastDebugOutput = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();
        float debugElapsed = std::chrono::duration<float>(now - lastDebugOutput).count();
        
        if (debugElapsed >= 1.0f) {
            std::cout << "[PM] Player " << playerID << " is dead. Respawn timer: " 
                      << rp.respawnTimer << ", Health: " << rp.player.GetHealth() << "\n";
            lastDebugOutput = now;
        }
        
        if (rp.respawnTimer > 0.0f) {
            rp.respawnTimer -= dt;
            if (rp.respawnTimer <= 0.0f) {
                std::cout << "[PM] Respawn timer expired for " << playerID << "\n";
                
                // Call respawn
                RespawnPlayer(playerID);
                
                // Double-check health after respawn
                if (rp.player.GetHealth() < rp.player.GetMaxHealth()) {
                    std::cout << "[PM] WARNING: Player health not fully restored after respawn: " 
                              << rp.player.GetHealth() << "/" << rp.player.GetMaxHealth() << "\n";
                    
                    // Force health to full as a fallback
                    rp.player.Respawn();
                }
            }
        } else if (playerID == localPlayerID) {
            // If local player is dead but has no respawn timer, set one using the setting
            if (rp.respawnTimer <= 0.0f) {
                std::cout << "[PM] Setting respawn timer for local player\n";
                rp.respawnTimer = respawnTime; // Use setting-based respawn time
            }
        }
    }
}


void PlayerManager::UpdatePlayerPosition(float dt, const std::string& playerID, RemotePlayer& rp, Game* game) {
    sf::Vector2f pos;
    
    if (playerID == localPlayerID) {
        // For local player, use the InputManager for movement
        rp.player.Update(dt, game->GetInputManager());
        pos = rp.player.GetPosition();
    } else {
        // For remote players, use interpolation
        auto now = std::chrono::steady_clock::now();
        float elapsed = std::chrono::duration<float>(now - rp.lastUpdateTime).count();
        float t = elapsed / rp.interpDuration;
        if (t > 1.0f) t = 1.0f;
        pos = rp.previousPosition + (rp.targetPosition - rp.previousPosition) * t;
        rp.player.SetPosition(pos);
    }
    
    // Update player name position to follow the player
    rp.nameText.setPosition(pos.x, pos.y - 20.f);
}

void PlayerManager::UpdatePlayerNameDisplay(const std::string& playerID, RemotePlayer& rp, Game* game) {
    // Only show ready status in Lobby state
    if (game->GetCurrentState() == GameState::Lobby) {
        std::string status = rp.isReady ? " [R]" : " [X]";
        rp.nameText.setString(rp.baseName + status);
    } else {
        rp.nameText.setString(rp.baseName);
    }
}

void PlayerManager::AddBullet(const std::string& shooterID, const sf::Vector2f& position, const sf::Vector2f& direction, float velocity) {
    // Validate input parameters
    if (direction.x == 0.f && direction.y == 0.f) {
        return;
    }
    
    if (shooterID.empty()) {
        return;
    }
    
    // For debugging: print the ID being added and local ID for comparison
    std::string localSteamIDStr = std::to_string(SteamUser()->GetSteamID().ConvertToUint64());

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
    
    // If game settings manager exists, apply settings to the new bullet
    if (game->GetGameSettingsManager()) {
        bullets.back().ApplySettings(game->GetGameSettingsManager());
    }
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
    
    // Move to the map (don't copy)
    players[id] = std::move(rp);
    localPlayerID = id;
}
void PlayerManager::SetReadyStatus(const std::string& id, bool ready) {
    if (players.find(id) != players.end()) {
        players[id].isReady = ready;        
        // Only update the visible name with ready status if in Lobby state
        if (game->GetCurrentState() == GameState::Lobby) {
            std::string status = ready ? " [R]" : " [X]";
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

bool PlayerManager::PlayerShoot(const sf::Vector2f& mouseWorldPos) {
    // Skip if the local player is dead
    auto& localPlayer = GetLocalPlayer();
    if (localPlayer.player.IsDead()) return false;
    
    // Call the player's AttemptShoot method
    Player::BulletParams bulletParams = localPlayer.player.AttemptShoot(mouseWorldPos);
    
    // If shooting was successful, create a bullet and send network message
    if (bulletParams.success) {
        // Get bullet speed from settings and apply multiplier
        float bulletSpeed = this->bulletSpeed * localPlayer.player.GetBulletSpeedMultiplier();
        
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

// Add these debug logging calls to PlayerManager::IncrementPlayerKills

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
            
            if (bulletIt->CheckCollision(remotePlayer.player.GetShape(), playerID)) {
                // Get the shooter
                std::string shooterID = bulletIt->GetShooterID();
                float damageAmount = bulletIt->GetDamage(); // Use bullet's damage value
                
                // Apply damage to player
                remotePlayer.player.TakeDamage(damageAmount);
                
                if (remotePlayer.player.IsDead()) {
                    // Report the death to the network
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
    // Lookup player in map
    auto it = players.find(playerID);
    if (it == players.end()) {
        std::cout << "[ERROR] PlayerDied called for non-existent player ID: " << playerID << std::endl;
        return;
    }
    
    // Make sure player isn't already dead to avoid double-processing
    if (it->second.player.IsDead()) {
        std::cout << "[INFO] PlayerDied called for already dead player: " << playerID << std::endl;
        return;
    }
    
    // Save current position as respawn position before death
    sf::Vector2f currentPos = it->second.player.GetPosition();
    it->second.player.SetRespawnPosition(currentPos);
    
    // Apply the damage to kill the player and set their state to dead
    it->second.player.TakeDamage(it->second.player.GetHealth()); // Kill player with exact damage needed
    
    // Schedule respawn after respawnTime seconds (from settings)
    it->second.respawnTimer = respawnTime;
    
    // If this is the local player, notify the network
    if (playerID == localPlayerID) {
        std::cout << "[DEATH] Local player died at position (" 
                  << currentPos.x << "," << currentPos.y 
                  << "), respawn position set to same location" << std::endl;
        
        // Check if we're the host by comparing with the lobby owner
        CSteamID localSteamID = SteamUser()->GetSteamID();
        CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
        
        if (localSteamID == hostID) {
            // We are the host, broadcast to all clients
            std::string deathMsg = PlayerMessageHandler::FormatPlayerDeathMessage(playerID, killerID);
            game->GetNetworkManager().BroadcastMessage(deathMsg);
        } else {
            // We are a client, send to host
            std::string deathMsg = PlayerMessageHandler::FormatPlayerDeathMessage(playerID, killerID);
            game->GetNetworkManager().SendMessage(hostID, deathMsg);
        }
    }
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
    
    
    // Check if we're the host
    CSteamID localSteamID = SteamUser()->GetSteamID();
    CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
    bool isHost = (localSteamID == hostID);
    
    if (isHost) {
        // Host logic - authoritative source of kill tracking
        if (players.find(normalizedKillerID) != players.end()) {
            // CHANGE: Use a static map to track recently processed kills
            static std::unordered_map<std::string, std::chrono::steady_clock::time_point> recentKills;
            
            // Create a unique key for this kill
            std::string killKey = normalizedKillerID + "_" + std::to_string(enemyId);
            
            // Check if we've recently processed this kill (within last 1 second)
            auto now = std::chrono::steady_clock::now();
            auto it = recentKills.find(killKey);
            
            if (it != recentKills.end()) {
                float elapsed = std::chrono::duration<float>(now - it->second).count();
                if (elapsed < 1.0f) {
                    // We've recently processed this exact kill, skip it
                    std::cout << "[HOST] Ignoring duplicate kill claim for " << normalizedKillerID 
                              << " and enemy " << enemyId << " (processed " << elapsed << "s ago)\n";
                    return;
                }
            }
            
            // Record this kill as processed
            recentKills[killKey] = now;
            
            // Clean up old entries from recentKills map (older than 10 seconds)
            for (auto killIt = recentKills.begin(); killIt != recentKills.end();) {
                float elapsed = std::chrono::duration<float>(now - killIt->second).count();
                if (elapsed > 10.0f) {
                    killIt = recentKills.erase(killIt);
                } else {
                    ++killIt;
                }
            }
            
            // CHANGE: Create a proper kill sequence using the current time for uniqueness
            static uint32_t killSequence = 0;
            killSequence++;  // Increment on each new kill
            
            // Add the sequence number to the kill message to ensure proper ordering
            std::string killMsg;
            if (killSequence > 0) {
                killMsg = PlayerMessageHandler::FormatKillMessage(
                    normalizedKillerID, 
                    enemyId, 
                    killSequence  // Use the incrementing sequence number
                );
            } else {
                killMsg = PlayerMessageHandler::FormatKillMessage(
                    normalizedKillerID, 
                    enemyId
                );
            }
            
            // IMPORTANT: Apply the kill locally first before broadcasting
            // This ensures the host's state matches what it broadcasts
            players[normalizedKillerID].kills++;
            players[normalizedKillerID].money += 50;

            
            // Broadcast the kill information with sequence number to all clients
            game->GetNetworkManager().BroadcastMessage(killMsg);
        }
    } else {
        // Client logic - send kill message to host for validation
        // Note: Clients don't increment their own kills or award money here
        // They wait for the host to broadcast the kill message back
        std::string killMsg = PlayerMessageHandler::FormatKillMessage(normalizedKillerID, enemyId);
        game->GetNetworkManager().SendMessage(hostID, killMsg);

    }
}

void PlayerManager::RespawnPlayer(const std::string& playerID) {
    // Normalize the player ID for consistent comparison
    std::string normalizedID = playerID;
    try {
        uint64_t idNum = std::stoull(playerID);
        normalizedID = std::to_string(idNum);
    } catch (const std::exception& e) {
        std::cout << "[PM] Error normalizing player ID in RespawnPlayer: " << e.what() << "\n";
    }
    
    // Normalize the local player ID too
    std::string normalizedLocalID = localPlayerID;
    try {
        uint64_t idNum = std::stoull(localPlayerID);
        normalizedLocalID = std::to_string(idNum);
    } catch (const std::exception& e) {
        std::cout << "[PM] Error normalizing local ID in RespawnPlayer: " << e.what() << "\n";
    }
    
    auto it = players.find(normalizedID);
    if (it != players.end()) {
        // Save health before respawn for debugging
        int oldHealth = it->second.player.GetHealth();
        bool wasDead = it->second.player.IsDead();
        
        // Get the current saved respawn position before calling respawn
        sf::Vector2f respawnPos = it->second.player.GetRespawnPosition();
        
        // Call respawn
        it->second.player.Respawn();
        
        // Check health after respawn
        int newHealth = it->second.player.GetHealth();
        bool isDeadNow = it->second.player.IsDead();
        
        // If this is the local player, notify the network of respawn
        if (normalizedID == normalizedLocalID) {
            // Use the actual respawn position that was used
            sf::Vector2f usedRespawnPos = it->second.player.GetPosition();
            
            // Check if we're the host
            CSteamID localSteamID = SteamUser()->GetSteamID();
            CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
            
            if (localSteamID == hostID) {
                // We are the host, broadcast to all clients
                std::string respawnMsg = PlayerMessageHandler::FormatPlayerRespawnMessage(normalizedID, usedRespawnPos);
                if (game->GetNetworkManager().BroadcastMessage(respawnMsg)) {
                } else {
                    std::cout << "[PM] Failed to broadcast respawn message\n";
                }
            } else {
                // We are a client, send to host
                std::string respawnMsg = PlayerMessageHandler::FormatPlayerRespawnMessage(normalizedID, usedRespawnPos);
                if (game->GetNetworkManager().SendMessage(hostID, respawnMsg)) {
                } else {
                    std::cout << "[PM] Failed to send respawn message to host\n";
                }
            }
        }
    } else {
        std::cout << "[PM] Could not find player " << normalizedID << " to respawn\n";
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

// Updated implementation for PlayerManager::InitializeForceFields()
void PlayerManager::InitializeForceFields() {
    
    for (auto& pair : players) {
        std::string playerID = pair.first;
        RemotePlayer& rp = pair.second;
        
        // Initialize force field if not already done
        if (!rp.player.HasForceField()) {
            rp.player.InitializeForceField();
            
            // Always make sure the force field is enabled after initialization
            rp.player.EnableForceField(true);
            
            // Set up callback for zap events
            // We set this for all players, not just local, to ensure kill tracking works for all
            rp.player.GetForceField()->SetZapCallback([this, playerID](int enemyId, float damage, bool killed) {
                // Handle zap event
                HandleForceFieldZap(playerID, enemyId, damage, killed);
            });
            
        } else {
            
            // Make sure callback is set even for existing force fields
            rp.player.GetForceField()->SetZapCallback([this, playerID](int enemyId, float damage, bool killed) {
                // Handle zap event
                HandleForceFieldZap(playerID, enemyId, damage, killed);
            });
        }
    }
}
void PlayerManager::HandleForceFieldZap(const std::string& playerID, int enemyId, float damage, bool killed) {
    // Validate inputs
    if (playerID.empty() || enemyId < 0) {
        std::cerr << "[PM] Invalid parameters in HandleForceFieldZap: playerID=" 
                  << playerID << ", enemyId=" << enemyId << std::endl;
        return;
    }
    
    // Only update rewards for hits, not kills (as those are handled by HandleKill)
    if (!killed) {
        try {
            // Reward for hits (not kills) - with safe lookup
            auto it = players.find(playerID);
            if (it != players.end()) {
                it->second.money += 10; // 10 money for hitting an enemy with force field
            }
        } catch (const std::exception& e) {
            std::cerr << "[PM] Exception updating player money: " << e.what() << std::endl;
        }
    } else {
        // If the enemy was killed, use the centralized kill handling
        // This ensures consistent kill tracking for all kill types
        try {
            HandleKill(playerID, enemyId);
        } catch (const std::exception& e) {
            std::cerr << "[PM] Exception in HandleKill: " << e.what() << std::endl;
        }
    }
    
    // If this is the local player, send a network message for the zap effect
    if (playerID == localPlayerID) {
        try {
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
        } catch (const std::exception& e) {
            std::cerr << "[PM] Exception sending network message: " << e.what() << std::endl;
        }
    }
}

void PlayerManager::ApplySettings() {
    GameSettingsManager* settingsManager = game->GetGameSettingsManager();
    if (!settingsManager) return;
    
    // Retrieve and cache common settings
    GameSetting* respawnTimeSetting = settingsManager->GetSetting("respawn_time");
    if (respawnTimeSetting) {
        respawnTime = respawnTimeSetting->GetFloatValue();
    }
    
    GameSetting* bulletDamageSetting = settingsManager->GetSetting("bullet_damage");
    if (bulletDamageSetting) {
        bulletDamage = bulletDamageSetting->GetFloatValue();
    }
    
    GameSetting* bulletSpeedSetting = settingsManager->GetSetting("bullet_speed");
    if (bulletSpeedSetting) {
        bulletSpeed = bulletSpeedSetting->GetFloatValue();
    }
    
    // Apply settings to all players and bullets
    ApplySettingsToAllPlayers();
    ApplySettingsToAllBullets();
}

void PlayerManager::ApplySettingsToAllPlayers() {
    GameSettingsManager* settingsManager = game->GetGameSettingsManager();
    if (!settingsManager) return;
    
    // Apply settings to each player
    for (auto& pair : players) {
        pair.second.player.ApplySettings(settingsManager);
    }
}

void PlayerManager::ApplySettingsToAllBullets() {
    GameSettingsManager* settingsManager = game->GetGameSettingsManager();
    if (!settingsManager) return;
    
    // Apply settings to each bullet
    for (auto& bullet : bullets) {
        bullet.ApplySettings(settingsManager);
    }
}