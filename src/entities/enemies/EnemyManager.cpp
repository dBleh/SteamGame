#include "EnemyManager.h"
#include "../../core/Game.h"
#include "../PlayerManager.h"
#include "../../network/Host.h"
#include "../../network/Client.h"
#include "../../network/NetworkManager.h"
#include "../../network/messages/MessageHandler.h"
#include <algorithm>
#include <random>
#include <iostream>
#include <cmath>
#include <ctime>
#include <sstream>
#include <thread>
#include <chrono>

EnemyManager::EnemyManager(Game* game, PlayerManager* playerManager)
    : game(game),
      playerManager(playerManager),
      nextEnemyId(1),
      syncTimer(0.0f),
      fullSyncTimer(0.0f),
      currentWave(0),
      remainingEnemiesInWave(0),
      batchSpawnTimer(0.0f),
      currentWaveEnemyType(EnemyType::Triangle),
      enemiesRemainingToSpawn(0),
      lastEnemyStateUpdate(std::chrono::steady_clock::now()) {
    // Initialize random seed
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
}

void EnemyManager::Update(float dt) {
    // Skip updates if paused or not in gameplay
    if (game->GetCurrentState() != GameState::Playing) return;
    
    // Update batch spawning if needed
    if (remainingEnemiesInWave > 0) {
        UpdateSpawning(dt);
    }
    
    // Determine if we're the host
    CSteamID myID = SteamUser()->GetSteamID();
    CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
    bool isHost = (myID == hostID);
    
    // On the client, if we haven't received a full state update in a while, request one
    if (!isHost) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastEnemyStateUpdate).count();
        
        // If it's been more than 5 seconds since the last full update, request one
        if (elapsed > 5) {
            // Send a request for a full state update
            std::ostringstream oss;
            oss << "ESR"; // Enemy State Request
            game->GetNetworkManager().SendMessage(hostID, oss.str());
            lastEnemyStateUpdate = now; // Reset the timer to prevent spamming requests
        }
        
        // Apply client-side prediction for smoother movement
        UpdateEnemyClientPrediction(dt);
    } else {
        // Host updates enemies normally
        for (auto& pair : enemies) {
            pair.second->Update(dt, *playerManager);
        }
        
        // Sync enemy positions periodically
        syncTimer += dt;
        if (syncTimer >= ENEMY_SYNC_INTERVAL) {
            SyncEnemyPositions();
            syncTimer = 0.0f;
        }
        
        // Send full state less frequently
        fullSyncTimer += dt;
        if (fullSyncTimer >= FULL_SYNC_INTERVAL) {
            SyncFullState();
            fullSyncTimer = 0.0f;
        }
    }
    
    // These checks are done by both host and client
    CheckPlayerCollisions();
    CheckBulletEnemyCollisions();
    
    // Optimize enemy list if we have a large number
    if (enemies.size() > 200) {
        OptimizeEnemyList();
    }
}

void EnemyManager::UpdateEnemyClientPrediction(float dt) {
    // Update enemy positions on the client based on their current velocity and AI
    for (auto& pair : enemies) {
        Enemy* enemy = pair.second.get();

            enemy->Update(dt, *playerManager);
    }
}

void EnemyManager::Render(sf::RenderWindow& window) {
    for (auto& pair : enemies) {
        pair.second->Render(window);
    }
}

int EnemyManager::AddEnemy(EnemyType type, const sf::Vector2f& position, float health) {
    int id = nextEnemyId++;
    auto enemy = CreateEnemy(type, id, position);
    enemy->SetHealth(health);
    enemies[id] = std::move(enemy);
    
    // If we're the host, broadcast this new enemy to all clients
    CSteamID myID = SteamUser()->GetSteamID();
    CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
    
    if (myID == hostID) {
        std::ostringstream oss;
        oss << "EA|" << id << "|" << static_cast<int>(type) << "|" 
            << position.x << "," << position.y << "|" << health;
        
        game->GetNetworkManager().BroadcastMessage(oss.str());
    }
    
    return id;
}

void EnemyManager::RemoveEnemy(int id) {
    auto it = enemies.find(id);
    if (it != enemies.end()) {
        // If we're the host, broadcast this removal to all clients
        CSteamID myID = SteamUser()->GetSteamID();
        CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
        
        if (myID == hostID) {
            std::ostringstream oss;
            oss << "ER|" << id;
            game->GetNetworkManager().BroadcastMessage(oss.str());
        }
        
        enemies.erase(it);
    }
}

void EnemyManager::ClearEnemies() {
    enemies.clear();
    
    // If we're the host, broadcast a clear command
    CSteamID myID = SteamUser()->GetSteamID();
    CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
    
    if (myID == hostID) {
        game->GetNetworkManager().BroadcastMessage("EC");
    }
}

bool EnemyManager::InflictDamage(int enemyId, float damage) {
    auto it = enemies.find(enemyId);
    if (it != enemies.end()) {
        bool killed = it->second->TakeDamage(damage);
        
        // If we're the host, broadcast this damage to all clients
        CSteamID myID = SteamUser()->GetSteamID();
        CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
        
        if (myID == hostID) {
            std::ostringstream oss;
            oss << "ED|" << enemyId << "|" << damage << "|" << it->second->GetHealth();
            game->GetNetworkManager().BroadcastMessage(oss.str());
            
            if (killed) {
                RemoveEnemy(enemyId);
            }
        }
        
        return killed;
    }
    
    return false;
}

void EnemyManager::CheckPlayerCollisions() {
    auto& players = playerManager->GetPlayers();
    
    for (auto enemyIt = enemies.begin(); enemyIt != enemies.end();) {
        bool enemyRemoved = false;
        
        for (auto& playerPair : players) {
            if (playerPair.second.player.IsDead()) continue;
            
            if (enemyIt->second->CheckPlayerCollision(playerPair.second.player.GetShape())) {
                // Apply damage to player
                playerPair.second.player.TakeDamage(TRIANGLE_DAMAGE);
                
                if (playerPair.second.player.IsDead()) {
                    playerManager->PlayerDied(playerPair.first, "");
                }
                
                // Remove enemy after collision
                int enemyId = enemyIt->first;
                enemyIt = enemies.erase(enemyIt);
                enemyRemoved = true;
                
                // If we're the host, broadcast this removal
                CSteamID myID = SteamUser()->GetSteamID();
                CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
                
                if (myID == hostID) {
                    std::ostringstream oss;
                    oss << "ER|" << enemyId;
                    game->GetNetworkManager().BroadcastMessage(oss.str());
                    
                    // Also inform about player damage
                    oss.str("");
                    oss << "PD|" << playerPair.first << "|" << TRIANGLE_DAMAGE << "|" << enemyId;
                    game->GetNetworkManager().BroadcastMessage(oss.str());
                }
                
                break;
            }
        }
        
        if (!enemyRemoved) {
            ++enemyIt;
        }
    }
}

void EnemyManager::CheckBulletEnemyCollisions() {
    if (!playerManager) return;
    
    // Get all bullets
    const auto& bullets = playerManager->GetAllBullets();
    std::vector<size_t> bulletsToRemove;
    
    // Check each bullet for collisions with enemies
    for (size_t bulletIdx = 0; bulletIdx < bullets.size(); bulletIdx++) {
        const Bullet& bullet = bullets[bulletIdx];
        
        // Skip already expired bullets
        if (bullet.IsExpired()) {
            bulletsToRemove.push_back(bulletIdx);
            continue;
        }
        
        sf::Vector2f bulletPos = bullet.GetPosition();
        int enemyId = -1;
        
        // Check if bullet hits any enemy
        if (CheckBulletCollision(bulletPos, 4.0f, enemyId)) {
            // If we hit an enemy, add this bullet to remove list
            bulletsToRemove.push_back(bulletIdx);
            
            // Apply damage to the enemy
            std::string shooterId = bullet.GetShooterID();
            
            // Get local player ID
            CSteamID myID = SteamUser()->GetSteamID();
            CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
            std::string localPlayerID = std::to_string(myID.ConvertToUint64());
            
            if (myID == hostID) {
                // Host handles damage directly
                bool killed = InflictDamage(enemyId, 20.0f); // 20 damage per bullet
                
                // If player killed an enemy, reward them
                if (killed && !shooterId.empty()) {
                    playerManager->IncrementPlayerKills(shooterId);
                    
                    // Add money reward
                    auto& players = playerManager->GetPlayers();
                    auto it = players.find(shooterId);
                    if (it != players.end()) {
                        it->second.money += TRIANGLE_KILL_REWARD;
                    }
                }
            } else {
                // Client sends damage message to host
                std::string damageMsg = MessageHandler::FormatEnemyDamageMessage(enemyId, 20.0f, 0.0f);
                game->GetNetworkManager().SendMessage(hostID, damageMsg);
            }
        }
    }
    
    // Remove all collided bullets
    if (!bulletsToRemove.empty()) {
        playerManager->RemoveBullets(bulletsToRemove);
    }
}

bool EnemyManager::CheckBulletCollision(const sf::Vector2f& bulletPos, float bulletRadius, int& outEnemyId) {
    for (auto& pair : enemies) {
        if (pair.second->CheckBulletCollision(bulletPos, bulletRadius)) {
            outEnemyId = pair.first;
            return true;
        }
    }
    
    outEnemyId = -1;
    return false;
}

void EnemyManager::SyncEnemyPositions() {
    if (enemies.empty()) return;
    
    // Get priority list of enemies to sync
    std::vector<int> priorities = GetEnemyUpdatePriorities();
    
    // Increase to 20 enemies per update for better coverage
    size_t updateCount = std::min(priorities.size(), static_cast<size_t>(20));
    
    // Create batch position update message
    std::ostringstream oss;
    oss << "EP";
    
    for (size_t i = 0; i < updateCount; ++i) {
        int enemyId = priorities[i];
        auto it = enemies.find(enemyId);
        if (it != enemies.end()) {
            const sf::Vector2f& pos = it->second->GetPosition();
            sf::Vector2f vel = it->second->GetVelocity();
            oss << "|" << enemyId << "," << pos.x << "," << pos.y << "," << vel.x << "," << vel.y;
        }
    }
    
    // Broadcast the message
    game->GetNetworkManager().BroadcastMessage(oss.str());
}

void EnemyManager::SyncFullState() {
    if (enemies.empty()) return;
    
    // Limit the number of enemies per state update to avoid network congestion
    
    // Get a list of all enemy IDs
    std::vector<int> allEnemyIds;
    for (const auto& pair : enemies) {
        allEnemyIds.push_back(pair.first);
    }
    
    // Process enemies in batches
    for (size_t startIdx = 0; startIdx < allEnemyIds.size(); startIdx += ENEMY_SPAWN_BATCH_SIZE) {
        // Calculate how many enemies to include in this batch
        size_t endIdx = std::min(startIdx + ENEMY_SPAWN_BATCH_SIZE, allEnemyIds.size());
        size_t count = endIdx - startIdx;
        
        // Create a batch state update message
        std::ostringstream oss;
        oss << "ES";
        
        for (size_t i = startIdx; i < endIdx; ++i) {
            int enemyId = allEnemyIds[i];
            const Enemy& enemy = *enemies[enemyId];
            sf::Vector2f velocity = enemy.GetVelocity();
            oss << "|" << enemy.GetID() << ","
                << static_cast<int>(enemy.GetType()) << ","
                << enemy.GetPosition().x << "," << enemy.GetPosition().y << ","
                << enemy.GetHealth() << "," << velocity.x << "," << velocity.y;
        }
        
        // Broadcast the message
        game->GetNetworkManager().BroadcastMessage(oss.str());
        
        // Add a small delay between batches to avoid overloading the network
        if (count < allEnemyIds.size()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

void EnemyManager::ApplyNetworkUpdate(int enemyId, const sf::Vector2f& position, float health) {
    auto it = enemies.find(enemyId);
    if (it != enemies.end()) {
        // Update existing enemy
        it->second->SetPosition(position);
        it->second->SetHealth(health);
    } else {
        // Enemy doesn't exist, create it with default type until a full state update arrives
        RemoteAddEnemy(enemyId, EnemyType::Triangle, position, health);
        
        std::cout << "[CLIENT] Created new enemy from position update: ID=" << enemyId 
                  << " at position (" << position.x << "," << position.y << ")\n";
    }
}

void EnemyManager::ApplyNetworkUpdateWithVelocity(int enemyId, const sf::Vector2f& position, 
    const sf::Vector2f& velocity, float health) {
        auto it = enemies.find(enemyId);
        if (it != enemies.end()) {
        // Update existing enemy
        Enemy* enemy = it->second.get();

        // Calculate position difference
        sf::Vector2f currentPos = enemy->GetPosition();
        sf::Vector2f posDiff = position - currentPos;

        // If position change is small, apply smoothly
        float distanceSquared = posDiff.x * posDiff.x + posDiff.y * posDiff.y;
        if (distanceSquared < 100.0f) { // Small movement (10 units squared)
        // Apply half the correction now for smoother movement
        enemy->SetPosition(currentPos + posDiff * 0.5f);
        } else {
        // Large jump detected, apply full update
        enemy->SetPosition(position);
        }

        enemy->SetVelocity(velocity);
        enemy->SetHealth(health);

        // Reset prediction timer since we got a fresh update
        lastEnemyStateUpdate = std::chrono::steady_clock::now();
        } else {
        // Enemy doesn't exist, create it and set velocity
        RemoteAddEnemyWithVelocity(enemyId, EnemyType::Triangle, position, velocity, health);

        std::cout << "[CLIENT] Created new enemy with velocity: ID=" << enemyId 
        << " at position (" << position.x << "," << position.y 
        << ") with velocity (" << velocity.x << "," << velocity.y << ")\n";
        }
}

void EnemyManager::RemoteAddEnemy(int enemyId, EnemyType type, const sf::Vector2f& position, float health) {
    // Create the enemy with the given ID
    auto enemy = CreateEnemy(type, enemyId, position);
    enemy->SetHealth(health);
    
    // Apply a small random offset to the initial position for smoother appearance
    sf::Vector2f spawnPos = position;
    
    // Only apply offset if this isn't the first creation of the enemy
    if (enemies.find(enemyId) == enemies.end()) {
        // Get unit vector in a random direction
        float angle = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 2.0f * 3.14159f;
        sf::Vector2f offset(std::cos(angle), std::sin(angle));
        
        // Scale the offset to a small distance (20-40 pixels)
        float distance = 20.0f + static_cast<float>(rand() % 20);
        spawnPos += offset * distance;
        
        // Set the actual spawn position
        enemy->SetPosition(spawnPos);
    }
    
    enemies[enemyId] = std::move(enemy);
    
    // Update nextEnemyId if necessary
    if (enemyId >= nextEnemyId) {
        nextEnemyId = enemyId + 1;
    }
}
void EnemyManager::RemoteAddEnemyWithVelocity(int enemyId, EnemyType type, const sf::Vector2f& position, 
                                          const sf::Vector2f& velocity, float health) {
    // Create the enemy with the given ID
    auto enemy = CreateEnemy(type, enemyId, position);
    enemy->SetHealth(health);
    enemy->SetVelocity(velocity);
    enemies[enemyId] = std::move(enemy);
    
    // Update nextEnemyId if necessary
    if (enemyId >= nextEnemyId) {
        nextEnemyId = enemyId + 1;
    }
}

void EnemyManager::RemoteRemoveEnemy(int enemyId) {
    enemies.erase(enemyId);
}

void EnemyManager::StartNewWave(int enemyCount, EnemyType type) {
    // Clear any remaining enemies
    ClearEnemies();
    
    // Increment wave counter
    currentWave++;
    
    // Set up enemy spawning
    currentWaveEnemyType = type;
    remainingEnemiesInWave = enemyCount;
    batchSpawnTimer = 0.0f;
    
    // Cache player positions for spawning
    UpdatePlayerPositionsCache();
    
    // Broadcast wave start if we're the host
    CSteamID myID = SteamUser()->GetSteamID();
    CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
    
    if (myID == hostID) {
        std::ostringstream oss;
        oss << "WS|" << currentWave << "|" << enemyCount;
        game->GetNetworkManager().BroadcastMessage(oss.str());
    }
}

void EnemyManager::UpdatePlayerPositionsCache() {
    playerPositionsCache.clear();
    auto& players = playerManager->GetPlayers();
    
    for (const auto& pair : players) {
        if (!pair.second.player.IsDead()) {
            playerPositionsCache.push_back(pair.second.player.GetPosition());
        }
    }
    
    // If no players, use origin
    if (playerPositionsCache.empty()) {
        playerPositionsCache.push_back(sf::Vector2f(0.0f, 0.0f));
    }
}

void EnemyManager::OptimizeEnemyList() {
    // Remove enemies that are very far from all players
    auto& players = playerManager->GetPlayers();
    std::vector<int> enemiesToRemove;
    
    // If no players, don't remove any enemies
    if (players.empty()) return;
    
    for (const auto& pair : enemies) {
        const sf::Vector2f& enemyPos = pair.second->GetPosition();
        bool tooFar = true;
        
        for (const auto& playerPair : players) {
            if (playerPair.second.player.IsDead()) continue;
            
            sf::Vector2f playerPos = playerPair.second.player.GetPosition();
            float distSquared = std::pow(enemyPos.x - playerPos.x, 2) + 
                                std::pow(enemyPos.y - playerPos.y, 2);
            
            // Keep enemies within ENEMY_CULLING_DISTANCE units of any player
            if (distSquared < ENEMY_CULLING_DISTANCE * ENEMY_CULLING_DISTANCE) {
                tooFar = false;
                break;
            }
        }
        
        if (tooFar) {
            enemiesToRemove.push_back(pair.first);
        }
    }
    
    // Remove the enemies that are too far
    for (int id : enemiesToRemove) {
        RemoveEnemy(id);
    }
}

bool EnemyManager::IsValidSpawnPosition(const sf::Vector2f& position) {
    // Check if the position is too close to any player
    auto& players = playerManager->GetPlayers();
    
    for (const auto& pair : players) {
        if (pair.second.player.IsDead()) continue;
        
        sf::Vector2f playerPos = pair.second.player.GetPosition();
        float distSquared = std::pow(position.x - playerPos.x, 2) + 
                            std::pow(position.y - playerPos.y, 2);
        
        if (distSquared < TRIANGLE_MIN_SPAWN_DISTANCE * TRIANGLE_MIN_SPAWN_DISTANCE) {
            return false;
        }
    }
    
    return true;
}

std::vector<int> EnemyManager::GetEnemyUpdatePriorities() {
    std::vector<int> priorityList;
    auto& players = playerManager->GetPlayers();
    
    // If no players, prioritize based on ID
    if (players.empty()) {
        for (const auto& pair : enemies) {
            priorityList.push_back(pair.first);
        }
        return priorityList;
    }
    
    // Create a vector of pairs (enemy ID, priority score)
    std::vector<std::pair<int, float>> enemyPriorities;
    
    for (const auto& pair : enemies) {
        const sf::Vector2f& enemyPos = pair.second->GetPosition();
        float minDistSquared = std::numeric_limits<float>::max();
        
        // Find the closest player
        for (const auto& playerPair : players) {
            if (playerPair.second.player.IsDead()) continue;
            
            sf::Vector2f playerPos = playerPair.second.player.GetPosition();
            float distSquared = std::pow(enemyPos.x - playerPos.x, 2) + 
                                std::pow(enemyPos.y - playerPos.y, 2);
            
            minDistSquared = std::min(minDistSquared, distSquared);
        }
        
        // Priority is inverse of distance (closer = higher priority)
        float priority = 1.0f / (1.0f + std::sqrt(minDistSquared));
        enemyPriorities.push_back({pair.first, priority});
    }
    
    // Sort by priority (higher first)
    std::sort(enemyPriorities.begin(), enemyPriorities.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });
    
    // Extract only the IDs
    for (const auto& pair : enemyPriorities) {
        priorityList.push_back(pair.first);
    }
    
    return priorityList;
}

sf::Vector2f EnemyManager::GetRandomSpawnPosition(const sf::Vector2f& targetPosition, 
                                                float minDistance, float maxDistance) {
    // Create random number generator
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * 3.14159f);
    std::uniform_real_distribution<float> distDist(minDistance, maxDistance);
    
    // Generate a random angle and distance
    float angle = angleDist(gen);
    float distance = distDist(gen);
    
    // Convert to Cartesian coordinates
    float x = targetPosition.x + std::cos(angle) * distance;
    float y = targetPosition.y + std::sin(angle) * distance;
    
    // Return the spawn position
    return sf::Vector2f(x, y);
}

void EnemyManager::RemoveEnemiesNotInList(const std::vector<int>& validIds) {
    std::vector<int> enemyIdsToRemove;
    
    for (const auto& pair : enemies) {
        bool found = false;
        for (int id : validIds) {
            if (pair.first == id) {
                found = true;
                break;
            }
        }
        
        if (!found) {
            enemyIdsToRemove.push_back(pair.first);
        }
    }
    
    for (int id : enemyIdsToRemove) {
        RemoteRemoveEnemy(id);
    }
}

Enemy* EnemyManager::FindEnemy(int id) {
    auto it = enemies.find(id);
    return (it != enemies.end()) ? it->second.get() : nullptr;
}

void EnemyManager::HandleStateRequest(CSteamID requesterId) {
    if (enemies.empty()) return;
    
    CSteamID myID = SteamUser()->GetSteamID();
    CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
    
    // Only the host should respond to state requests
    if (myID != hostID) return;
    
    std::cout << "[HOST] Received state request from " << requesterId.ConvertToUint64() << "\n";
    
    // Send a full state update directly to the requester
    SyncFullStateForClient(requesterId);
}

void EnemyManager::SyncFullStateForClient(CSteamID clientId) {
    if (enemies.empty()) return;
    
    // Limit the number of enemies per state update
   
    
    // Get a list of all enemy IDs
    std::vector<int> allEnemyIds;
    for (const auto& pair : enemies) {
        allEnemyIds.push_back(pair.first);
    }
    
    // Process enemies in batches
    for (size_t startIdx = 0; startIdx < allEnemyIds.size(); startIdx += ENEMY_SPAWN_BATCH_SIZE) {
        // Calculate how many enemies to include in this batch
        size_t endIdx = std::min(startIdx + ENEMY_SPAWN_BATCH_SIZE, allEnemyIds.size());
        size_t count = endIdx - startIdx;
        
        // Create a batch state update message
        std::ostringstream oss;
        oss << "ES";
        
        for (size_t i = startIdx; i < endIdx; ++i) {
            int enemyId = allEnemyIds[i];
            const Enemy& enemy = *enemies[enemyId];
            sf::Vector2f velocity = enemy.GetVelocity();
            oss << "|" << enemy.GetID() << ","
                << static_cast<int>(enemy.GetType()) << ","
                << enemy.GetPosition().x << "," << enemy.GetPosition().y << ","
                << enemy.GetHealth() << "," << velocity.x << "," << velocity.y;
        }
        
        // Send the message directly to the requesting client
        game->GetNetworkManager().SendMessage(clientId, oss.str());
        
        // Add a small delay between batches
        if (count < allEnemyIds.size()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

void EnemyManager::UpdateSpawning(float dt) {
    // Update batch timer
    batchSpawnTimer += dt;
    
    // Check if it's time to spawn a batch
    if (batchSpawnTimer >= ENEMY_SPAWN_BATCH_INTERVAL) {
        // Reset timer
        batchSpawnTimer = 0.0f;
        
        // Make sure we have up-to-date player positions for spawning
        UpdatePlayerPositionsCache();
        
        // Calculate how many enemies to spawn in this batch
        int spawnCount = std::min(ENEMY_SPAWN_BATCH_SIZE, remainingEnemiesInWave);
        
        if (spawnCount > 0) {
            // Debug log
            std::cout << "[WAVE] Spawning batch of " << spawnCount 
                      << " enemies. Remaining after batch: " 
                      << (remainingEnemiesInWave - spawnCount) << std::endl;
            
            // Spawn the batch
            for (int i = 0; i < spawnCount; ++i) {
                // Randomly select a player position to spawn relative to
                sf::Vector2f targetPos = playerPositionsCache[rand() % playerPositionsCache.size()];
                
                // Get a spawn position away from the player
                sf::Vector2f spawnPos = GetRandomSpawnPosition(targetPos, 
                                                             TRIANGLE_MIN_SPAWN_DISTANCE, 
                                                             TRIANGLE_MAX_SPAWN_DISTANCE);
                
                // Add the enemy
                AddEnemy(currentWaveEnemyType, spawnPos);
            }
            
            // Update remaining count
            remainingEnemiesInWave -= spawnCount;
        }
    }
}

bool EnemyManager::IsWaveComplete() const {
    // A wave is complete when all enemies have been spawned and eliminated
    return enemies.empty() && remainingEnemiesInWave == 0;
}