#include "TriangleManager.h"
#include "../Game.h" // Your Game class
#include "PlayerManager.h" // Your PlayerManager class
#include "Bullet.h" // Your Bullet class
#include "MessageHandler.h" // Your MessageHandler class
#include <algorithm>
#include <cmath>
#include <random>

TriangleEnemyManager::TriangleEnemyManager(Game* game, PlayerManager* playerManager)
    : game(game), playerManager(playerManager), spatialGrid(100.f, sf::Vector2f(3000.f, 3000.f)),
      nextEnemyId(1), enemySyncTimer(0.f), fullSyncTimer(0.f)
{
    InitializeUpdateGroups();
}

TriangleEnemyManager::~TriangleEnemyManager()
{
    // Clear all data structures
    enemies.clear();
    enemyMap.clear();
    lastSyncedPositions.clear();
    visibleEnemies.clear();
    updateGroups.clear();
    spatialGrid.Clear();
}

void TriangleEnemyManager::InitializeUpdateGroups()
{
    // Clear existing groups
    updateGroups.clear();
    
    // Group 1: Close enemies - update every frame
    EnemyUpdateGroup closeGroup;
    closeGroup.updateInterval = 0.f; // Every frame
    closeGroup.timeSinceLastUpdate = 0.f;
    
    // Group 2: Medium distance enemies - update every 2 frames
    EnemyUpdateGroup mediumGroup;
    mediumGroup.updateInterval = 0.033f; // ~2 frames at 60fps
    mediumGroup.timeSinceLastUpdate = 0.f;
    
    // Group 3: Far enemies - update every 4 frames
    EnemyUpdateGroup farGroup;
    farGroup.updateInterval = 0.066f; // ~4 frames at 60fps
    farGroup.timeSinceLastUpdate = 0.f;
    
    updateGroups.push_back(closeGroup);
    updateGroups.push_back(mediumGroup);
    updateGroups.push_back(farGroup);
}

void TriangleEnemyManager::Update(float dt)
{
    // Update network sync timers
    enemySyncTimer += dt;
    fullSyncTimer += dt;
    
    // Gradual enemy spawning
    if (isSpawningWave && remainingEnemiestoSpawn > 0) {
        spawnTimer += dt;
        if (spawnTimer >= SPAWN_INTERVAL) {
            SpawnEnemyBatch(5); // Spawn 5 enemies at a time
            spawnTimer = 0.f;
        }
    }
    
    // Update each group based on its interval
    for (auto& group : updateGroups) {
        group.timeSinceLastUpdate += dt;
        
        if (group.timeSinceLastUpdate >= group.updateInterval) {
            // Update enemies in this group
            for (TriangleEnemy* enemy : group.enemies) {
                if (enemy && enemy->IsAlive()) {
                    sf::Vector2f targetPos = FindClosestPlayerPosition(enemy->GetPosition());
                    enemy->Update(dt, targetPos);
                    
                    // Update position in spatial grid
                    spatialGrid.UpdateEnemyPosition(enemy);
                }
            }
            
            group.timeSinceLastUpdate = 0.f;
        }
    }
    
    // Sync enemy positions over network if interval elapsed
    if (enemySyncTimer >= ENEMY_SYNC_INTERVAL) {
        SyncEnemyPositions();
        enemySyncTimer = 0.f;
    }
    
    // Perform full sync at longer intervals
    if (fullSyncTimer >= FULL_SYNC_INTERVAL) {
        SyncFullEnemyList();
        fullSyncTimer = 0.f;
    }
}

void TriangleEnemyManager::Render(sf::RenderWindow& window)
{
    // Update visible enemies based on current view
    sf::FloatRect viewBounds = sf::FloatRect(window.getView().getCenter() - window.getView().getSize() / 2.f, window.getView().getSize());
    UpdateVisibleEnemies(viewBounds);
    
    // Only render visible enemies
    for (TriangleEnemy* enemy : visibleEnemies) {
        if (enemy && enemy->IsAlive()) {
            window.draw(enemy->GetShape());
        }
    }
}

void TriangleEnemyManager::SpawnWave(int enemyCount) {
    // Get the current time for a seed - this will be shared with clients
    uint32_t seed = static_cast<uint32_t>(std::time(nullptr));
    
    // Check if we're the host
    CSteamID localSteamID = SteamUser()->GetSteamID();
    CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
    bool isHost = (localSteamID == hostID);
    
    if (isHost) {
        // Send a wave start message with the seed and count using MessageHandler
        std::string message = MessageHandler::FormatTriangleWaveStartMessage(seed, enemyCount);
        game->GetNetworkManager().BroadcastMessage(message);
    }
    
    // Set up for gradual spawning rather than all at once
    isSpawningWave = true;
    remainingEnemiestoSpawn = enemyCount;
    spawnTimer = 0.f;
    
    // Seed the RNG for the wave
    GenerateEnemiesWithSeed(seed, enemyCount);
}
// Modified SpawnEnemyBatch method using the config constants

void TriangleEnemyManager::SpawnEnemyBatch(int count) {
    // Get average position of all players to use as spawn center
    sf::Vector2f centerPos(0.f, 0.f);
    int playerCount = 0;
    
    auto& players = playerManager->GetPlayers();
    for (const auto& pair : players) {
        if (!pair.second.player.IsDead()) {
            centerPos += pair.second.player.GetPosition();
            playerCount++;
        }
    }
    
    if (playerCount > 0) {
        centerPos /= static_cast<float>(playerCount);
    } else {
        // If no players alive, use origin as center
        centerPos = sf::Vector2f(0.f, 0.f);
    }
    
    // Use the constants from Config.h
    float minDistance = TRIANGLE_MIN_SPAWN_DISTANCE;
    float maxDistance = TRIANGLE_MAX_SPAWN_DISTANCE;
    
    // Setup random distributions for spawn position
    std::uniform_real_distribution<float> distanceDist(minDistance, maxDistance);
    std::uniform_real_distribution<float> angleDist(0.f, 2.f * 3.14159f); // Full 360 degrees in radians
    
    // Don't spawn more than we need
    int batchCount = std::min(count, remainingEnemiestoSpawn);
    remainingEnemiestoSpawn -= batchCount;
    
    // If done spawning, update flag
    if (remainingEnemiestoSpawn <= 0) {
        isSpawningWave = false;
    }
    
    // Prepare a batch of enemy data for efficient network transmission
    std::vector<std::tuple<int, sf::Vector2f, int>> batchData;
    batchData.reserve(batchCount);
    
    // Spawn the batch
    for (int i = 0; i < batchCount; i++) {
        // Generate spawn position using distance/angle from center (polar coordinates)
        float distance = distanceDist(seedGenerator);
        float angle = angleDist(seedGenerator);
        
        // Convert polar to cartesian coordinates
        float x = centerPos.x + distance * std::cos(angle);
        float y = centerPos.y + distance * std::sin(angle);
        
        sf::Vector2f position(x, y);
        
        // Create new enemy with health from config
        auto enemy = std::make_unique<TriangleEnemy>(nextEnemyId, position, ENEMY_SPEED, TRIANGLE_HEALTH);
        TriangleEnemy* enemyPtr = enemy.get();
        
        // Add to containers
        enemyMap[enemyPtr->GetID()] = enemyPtr;
        enemies.push_back(std::move(enemy));
        
        // Add to spatial grid
        spatialGrid.AddEnemy(enemyPtr);
        
        // Add to appropriate update group
        AssignEnemyToUpdateGroup(enemyPtr);
        
        // Initialize last synced position
        lastSyncedPositions[enemyPtr->GetID()] = position;
        
        // Add to batch data
        batchData.emplace_back(nextEnemyId, position, TRIANGLE_HEALTH);
        
        // Increment ID for next enemy
        nextEnemyId++;
    }
    
    // Send batch spawn message to clients
    if (!batchData.empty()) {
        CSteamID localSteamID = SteamUser()->GetSteamID();
        CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
        
        if (localSteamID == hostID) {
            std::string batchMsg = MessageHandler::FormatTriangleEnemyBatchSpawnMessage(batchData);
            game->GetNetworkManager().BroadcastMessage(batchMsg);
        }
    }
}
void TriangleEnemyManager::AddEnemy(int id, const sf::Vector2f& position) {
    // Create a new enemy with default health (40)
    AddEnemy(id, position, 40);
}

void TriangleEnemyManager::AddEnemy(int id, const sf::Vector2f& position, int health) {
    // Check if an enemy with this ID already exists
    if (enemyMap.find(id) != enemyMap.end()) {
        return; // Don't add duplicate IDs
    }
    
    // Create a new triangle enemy
    auto enemy = std::make_unique<TriangleEnemy>(id, position, 60.f, health);
    TriangleEnemy* enemyPtr = enemy.get();
    
    // Add to containers
    enemyMap[id] = enemyPtr;
    enemies.push_back(std::move(enemy));
    
    // Add to spatial grid
    spatialGrid.AddEnemy(enemyPtr);
    
    // Add to appropriate update group
    AssignEnemyToUpdateGroup(enemyPtr);
    
    // Initialize last synced position
    lastSyncedPositions[id] = position;
    
    // Update next enemy ID if necessary
    if (id >= nextEnemyId) {
        nextEnemyId = id + 1;
    }
}

int TriangleEnemyManager::GetNextEnemyId() const {
    return nextEnemyId;
}
// Update the CheckBulletCollisions method to use the config constant for rewards

void TriangleEnemyManager::CheckBulletCollisions(const std::vector<Bullet>& bullets) {
    // Get pointers to bullets that collided for later removal
    std::vector<size_t> bulletsToRemove;
    bulletsToRemove.reserve(bullets.size());
    
    // Check each bullet against nearby enemies using spatial partitioning
    for (size_t bulletIndex = 0; bulletIndex < bullets.size(); bulletIndex++) {
        const Bullet& bullet = bullets[bulletIndex];
        
        // Skip expired bullets
        if (bullet.IsExpired()) continue;
        
        sf::Vector2f bulletPos = bullet.GetPosition();
        
        // Calculate bullet radius from its shape
        float bulletRadius = bullet.GetShape().getSize().x / 2.0f;
        
        // Get enemies near the bullet
        auto nearbyEnemies = spatialGrid.GetEnemiesNearPosition(bulletPos, bulletRadius + 20.f);
        
        bool bulletHit = false;
        
        // Check collisions
        for (TriangleEnemy* enemy : nearbyEnemies) {
            if (enemy && enemy->IsAlive() && enemy->CheckBulletCollision(bulletPos, bulletRadius)) {
                // Apply damage with fixed damage amount (20 is the standard)
                bool killed = enemy->TakeDamage(20);
                
                // Mark the bullet for removal
                bulletsToRemove.push_back(bulletIndex);
                bulletHit = true;
                
                // Handle kill
                if (killed) {
                    // Remove from spatial grid
                    spatialGrid.RemoveEnemy(enemy);
                    
                    // Award money to the shooter using the config constant
                    std::string shooterID = bullet.GetShooterID();
                    auto& players = playerManager->GetPlayers();
                    
                    // Find the shooter in the player list
                    auto shooterIt = players.find(shooterID);
                    if (shooterIt != players.end()) {
                        // Add money from config and increment kill count
                        shooterIt->second.money += TRIANGLE_KILL_REWARD;
                        playerManager->IncrementPlayerKills(shooterID);
                        
                        std::cout << "[REWARD] Player " << shooterID 
                                  << " earned " << TRIANGLE_KILL_REWARD 
                                  << " money for killing triangle enemy" << std::endl;
                    }
                }
                
                // Send hit message
                CSteamID localSteamID = SteamUser()->GetSteamID();
                CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
                
                if (localSteamID == hostID) {
                    // Host sends message to all clients
                    std::string hitMsg = MessageHandler::FormatTriangleEnemyHitMessage(
                        enemy->GetID(), 20, killed, bullet.GetShooterID());
                    game->GetNetworkManager().BroadcastMessage(hitMsg);
                    
                    // If killed, also send death message with reward
                    if (killed) {
                        std::string deathMsg = MessageHandler::FormatTriangleEnemyDeathMessage(
                            enemy->GetID(), bullet.GetShooterID(), true);  // With reward
                        game->GetNetworkManager().BroadcastMessage(deathMsg);
                    }
                }
                
                break; // Bullet can only hit one enemy
            }
        }
        
        // If bullet hit something, no need to check further enemies
        if (bulletHit) continue;
    }
    
    // Remove bullets that hit enemies
    if (!bulletsToRemove.empty()) {
        playerManager->RemoveBullets(bulletsToRemove);
    }
}

sf::Vector2f TriangleEnemyManager::FindClosestPlayerPosition(const sf::Vector2f& enemyPos) {
    sf::Vector2f closestPos = enemyPos;
    float minDistance = std::numeric_limits<float>::max();
    
    // Get player positions from player manager
    const auto& players = playerManager->GetPlayers();
    
    for (const auto& playerPair : players) {
        const std::string& playerID = playerPair.first;
        const RemotePlayer& remotePlayer = playerPair.second;
        
        // Skip dead players using the Player's IsDead method
        if (remotePlayer.player.IsDead()) continue;
        
        // Use Player's GetPosition method
        sf::Vector2f playerPos = remotePlayer.player.GetPosition();
        float dx = playerPos.x - enemyPos.x;
        float dy = playerPos.y - enemyPos.y;
        float distSquared = dx * dx + dy * dy;
        
        if (distSquared < minDistance) {
            minDistance = distSquared;
            closestPos = playerPos;
        }
    }
    
    return closestPos;
}
// Update the CheckPlayerCollisions method to use the config constants

void TriangleEnemyManager::CheckPlayerCollisions() {
    // Get player shapes and positions from player manager
    auto& players = playerManager->GetPlayers();
    
    for (auto& playerPair : players) {
        const std::string& playerID = playerPair.first;
        RemotePlayer& remotePlayer = playerPair.second;
        
        // Skip dead players
        if (remotePlayer.player.IsDead()) continue;
        
        sf::Vector2f playerPos = remotePlayer.player.GetPosition();
        
        // Get enemies near the player
        auto nearbyEnemies = spatialGrid.GetEnemiesNearPosition(playerPos, 50.f);
        
        // Check collisions
        for (TriangleEnemy* enemy : nearbyEnemies) {
            if (enemy && enemy->IsAlive() && enemy->CheckCollision(remotePlayer.player.GetShape())) {
                // Apply damage to player using the config constant
                remotePlayer.player.TakeDamage(TRIANGLE_DAMAGE);
                
                std::cout << "[COLLISION] Player " << playerID << " took " << TRIANGLE_DAMAGE 
                          << " damage from triangle enemy " << enemy->GetID() << std::endl;
                
                // Kill enemy on collision
                enemy->TakeDamage(enemy->GetHealth());
                
                // Remove from spatial grid
                spatialGrid.RemoveEnemy(enemy);
                
                // Send death message if we're the host
                CSteamID localSteamID = SteamUser()->GetSteamID();
                CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
                
                if (localSteamID == hostID) {
                    // Send triangle enemy death message
                    std::string deathMsg = MessageHandler::FormatTriangleEnemyDeathMessage(
                        enemy->GetID(), "", false);  // No killer, no reward
                    game->GetNetworkManager().BroadcastMessage(deathMsg);
                    
                    // Also send player damage message
                    std::string damageMsg = MessageHandler::FormatPlayerDamageMessage(
                        playerID, TRIANGLE_DAMAGE, enemy->GetID());
                    game->GetNetworkManager().BroadcastMessage(damageMsg);
                }
                
                // If this is the local player, check if they died
                std::string localSteamIDStr = std::to_string(SteamUser()->GetSteamID().ConvertToUint64());
                if (playerID == localSteamIDStr && remotePlayer.player.IsDead()) {
                    // Set respawn timer
                    remotePlayer.respawnTimer = 3.0f;
                    
                    std::cout << "[DEATH] Local player died from triangle enemy collision" << std::endl;
                }
            }
        }
    }
}


void TriangleEnemyManager::SyncEnemyPositions() {
    // Use delta compression to reduce bandwidth
    auto compressedData = GetDeltaCompressedPositions();
    
    // Send only if there's data to send
    if (!compressedData.empty()) {
        // Split into batches if there are too many position updates
        const size_t BATCH_SIZE = 80; // Maximum position updates per message
        
        if (compressedData.size() <= BATCH_SIZE) {
            // Small enough for a single message
            std::string message = MessageHandler::FormatTriangleEnemyPositionsMessage(compressedData);
            game->GetNetworkManager().BroadcastMessage(message);
            
            // Update last synced positions
            for (const auto& tuple : compressedData) {
                int id = std::get<0>(tuple);
                sf::Vector2f pos = std::get<1>(tuple);
                lastSyncedPositions[id] = pos;
            }
        } else {
            // Split into batches
            for (size_t i = 0; i < compressedData.size(); i += BATCH_SIZE) {
                std::vector<std::tuple<int, sf::Vector2f, int>> batch;
                size_t end = std::min(i + BATCH_SIZE, compressedData.size());
                
                batch.insert(batch.end(), compressedData.begin() + i, compressedData.begin() + end);
                
                std::string message = MessageHandler::FormatTriangleEnemyPositionsMessage(batch);
                game->GetNetworkManager().BroadcastMessage(message);
                
                // Update last synced positions for this batch
                for (const auto& tuple : batch) {
                    int id = std::get<0>(tuple);
                    sf::Vector2f pos = std::get<1>(tuple);
                    lastSyncedPositions[id] = pos;
                }
                
                // Small delay to prevent network congestion
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
        }
    }
}


std::vector<std::tuple<int, sf::Vector2f, int>> TriangleEnemyManager::GetEnemyDataForSync() const
{
    std::vector<std::tuple<int, sf::Vector2f, int>> result;
    result.reserve(enemies.size());
    
    for (const auto& enemy : enemies) {
        if (enemy && enemy->IsAlive()) {
            result.emplace_back(
                enemy->GetID(),
                enemy->GetPosition(),
                enemy->GetHealth()
            );
        }
    }
    
    return result;
}

std::vector<std::tuple<int, sf::Vector2f, int>> TriangleEnemyManager::GetDeltaCompressedPositions() const
{
    std::vector<std::tuple<int, sf::Vector2f, int>> result;
    result.reserve(enemies.size());
    
    // Only send positions that have changed significantly
    constexpr float MIN_DELTA = 1.0f; // Minimum movement to consider a position changed
    
    for (const auto& enemy : enemies) {
        if (enemy && enemy->IsAlive()) {
            int id = enemy->GetID();
            sf::Vector2f currentPos = enemy->GetPosition();
            
            // Check if we have last synced position
            auto it = lastSyncedPositions.find(id);
            if (it != lastSyncedPositions.end()) {
                sf::Vector2f lastPos = it->second;
                sf::Vector2f delta = enemy->GetPositionDelta(lastPos);
                
                // Only sync if position changed significantly
                float deltaLength = std::sqrt(delta.x * delta.x + delta.y * delta.y);
                if (deltaLength >= MIN_DELTA) {
                    result.emplace_back(id, currentPos, enemy->GetHealth());
                }
            } else {
                // No last position, always sync
                result.emplace_back(id, currentPos, enemy->GetHealth());
            }
        }
    }
    
    return result;
}

void TriangleEnemyManager::SyncFullEnemyList() {
    std::vector<int> validIds;
    validIds.reserve(enemies.size());
    
    for (const auto& enemy : enemies) {
        if (enemy && enemy->IsAlive()) {
            validIds.push_back(enemy->GetID());
        }
    }
    
    // Split into batches if there are too many enemies
    const size_t BATCH_SIZE = 100; // Maximum enemies per message
    
    if (validIds.size() <= BATCH_SIZE) {
        // Small enough for a single message
        std::string message = MessageHandler::FormatTriangleEnemyFullListMessage(validIds);
        game->GetNetworkManager().BroadcastMessage(message);
    } else {
        // Split into batches
        for (size_t i = 0; i < validIds.size(); i += BATCH_SIZE) {
            std::vector<int> batch;
            size_t end = std::min(i + BATCH_SIZE, validIds.size());
            
            batch.insert(batch.end(), validIds.begin() + i, validIds.begin() + end);
            
            std::string message = MessageHandler::FormatTriangleEnemyFullListMessage(batch);
            game->GetNetworkManager().BroadcastMessage(message);
            
            // Small delay to prevent network congestion
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }
}


void TriangleEnemyManager::ValidateEnemyList(const std::vector<int>& validIds) {
    // Create set of valid IDs for faster lookup - using proper include
    std::unordered_set<int> validIdSet(validIds.begin(), validIds.end());
    
    // Remove enemies not in the valid list
    for (auto it = enemies.begin(); it != enemies.end();) {
        int id = (*it)->GetID();
        if (validIdSet.find(id) == validIdSet.end()) {
            // Enemy not valid, remove it
            spatialGrid.RemoveEnemy(it->get());
            enemyMap.erase(id);
            lastSyncedPositions.erase(id);
            it = enemies.erase(it);
        } else {
            ++it;
        }
    }
    
    // Reassign all remaining enemies to update groups
    ReassignAllEnemies();
}

void TriangleEnemyManager::HandleEnemyHit(int enemyId, int damage, bool killed)
{
    TriangleEnemy* enemy = GetEnemy(enemyId);
    if (enemy && enemy->IsAlive()) {
        // Apply damage
        bool actuallyKilled = enemy->TakeDamage(damage);
        
        // If server says it died but client doesn't think so, force kill
        if (killed && !actuallyKilled) {
            enemy->TakeDamage(enemy->GetHealth());
        }
        
        // If enemy died, remove from spatial grid
        if (killed) {
            spatialGrid.RemoveEnemy(enemy);
        }
    }
}

void TriangleEnemyManager::UpdateEnemyPositions(const std::vector<std::tuple<int, sf::Vector2f, int>>& enemyPositions)
{
    for (const auto& tuple : enemyPositions) {
        int id = std::get<0>(tuple);
        sf::Vector2f position = std::get<1>(tuple);
        int health = std::get<2>(tuple);
        
        TriangleEnemy* enemy = GetEnemy(id);
        if (enemy) {
            // Update position with interpolation for smooth movement
            enemy->UpdatePosition(position, true);
            
            // Update health
            if (enemy->GetHealth() != health) {
                // Set health directly to match server
                if (health <= 0) {
                    enemy->TakeDamage(enemy->GetHealth()); // Kill
                    spatialGrid.RemoveEnemy(enemy);
                } else {
                    // Force set health by applying damage
                    int currentHealth = enemy->GetHealth();
                    if (currentHealth > health) {
                        enemy->TakeDamage(currentHealth - health);
                    }
                }
            }
            
            // Update spatial grid
            spatialGrid.UpdateEnemyPosition(enemy);
            
            // Update last synced position
            lastSyncedPositions[id] = position;
        } else {
            // Enemy doesn't exist, create it
            auto newEnemy = std::make_unique<TriangleEnemy>(id, position, 60.f, health);
            TriangleEnemy* enemyPtr = newEnemy.get();
            
            // Add to containers
            enemyMap[id] = enemyPtr;
            enemies.push_back(std::move(newEnemy));
            
            // Add to spatial grid
            spatialGrid.AddEnemy(enemyPtr);
            
            // Add to appropriate update group
            AssignEnemyToUpdateGroup(enemyPtr);
            
            // Initialize last synced position
            lastSyncedPositions[id] = position;
        }
    }
}

void TriangleEnemyManager::UpdateEnemyHealth(int enemyId, int health)
{
    TriangleEnemy* enemy = GetEnemy(enemyId);
    if (enemy) {
        // Set health directly to match server
        if (health <= 0) {
            enemy->TakeDamage(enemy->GetHealth()); // Kill
            spatialGrid.RemoveEnemy(enemy);
        } else {
            // Force set health by applying damage
            int currentHealth = enemy->GetHealth();
            if (currentHealth > health) {
                enemy->TakeDamage(currentHealth - health);
            }
        }
    }
}

size_t TriangleEnemyManager::GetEnemyCount() const
{
    // Count only living enemies directly from the enemies vector
    size_t liveCount = 0;
    
    for (const auto& enemy : enemies) {
        if (enemy && enemy->IsAlive()) {
            liveCount++;
        }
    }
    
    return liveCount;
}

// 

TriangleEnemy* TriangleEnemyManager::GetEnemy(int enemyId)
{
    auto it = enemyMap.find(enemyId);
    if (it != enemyMap.end()) {
        return it->second;
    }
    return nullptr;
}

void TriangleEnemyManager::AssignEnemyToUpdateGroup(TriangleEnemy* enemy)
{
    if (!enemy) return;
    
    // Find closest player
    sf::Vector2f playerPos = FindClosestPlayerPosition(enemy->GetPosition());
    sf::Vector2f enemyPos = enemy->GetPosition();
    
    // Calculate distance
    float dx = playerPos.x - enemyPos.x;
    float dy = playerPos.y - enemyPos.y;
    float distance = std::sqrt(dx * dx + dy * dy);
    
    // Assign to group based on distance
    if (distance <= CLOSE_DISTANCE) {
        // Close group (group index 0)
        updateGroups[0].enemies.push_back(enemy);
    } else if (distance <= MEDIUM_DISTANCE) {
        // Medium group (group index 1)
        updateGroups[1].enemies.push_back(enemy);
    } else {
        // Far group (group index 2)
        updateGroups[2].enemies.push_back(enemy);
    }
}

void TriangleEnemyManager::ReassignAllEnemies()
{
    // Clear all groups
    for (auto& group : updateGroups) {
        group.enemies.clear();
    }
    
    // Reassign all enemies
    for (const auto& enemy : enemies) {
        if (enemy && enemy->IsAlive()) {
            AssignEnemyToUpdateGroup(enemy.get());
        }
    }
}

void TriangleEnemyManager::RemoveEnemy(int enemyId)
{
    auto it = enemyMap.find(enemyId);
    if (it != enemyMap.end()) {
        TriangleEnemy* enemy = it->second;
        
        // Remove from spatial grid
        spatialGrid.RemoveEnemy(enemy);
        
        // Remove from other containers
        enemyMap.erase(it);
        lastSyncedPositions.erase(enemyId);
        
        // Find and remove from enemies vector
        for (auto vecIt = enemies.begin(); vecIt != enemies.end(); ++vecIt) {
            if (vecIt->get() == enemy) {
                enemies.erase(vecIt);
                break;
            }
        }
    }
}

void TriangleEnemyManager::UpdateVisibleEnemies(const sf::FloatRect& viewBounds)
{
    // Clear previous list
    visibleEnemies.clear();
    
    // Get enemies in the view rect
    visibleEnemies = spatialGrid.GetEnemiesInRect(viewBounds);
}

void TriangleEnemyManager::GenerateEnemiesWithSeed(uint32_t seed, int enemyCount) {
    // Use the seed for deterministic generation on both host and client
    std::mt19937 gen(seed);
    
    // Clear existing enemies
    enemies.clear();
    enemyMap.clear();
    lastSyncedPositions.clear();
    for (auto& group : updateGroups) {
        group.enemies.clear();
    }
    spatialGrid.Clear();
    
    // Reset next enemy ID
    nextEnemyId = 1;
    
    // Set up for gradual spawning
    isSpawningWave = true;
    remainingEnemiestoSpawn = enemyCount;
    spawnTimer = 0.f;
    
    // Store the generator for later use in batched spawning
    seedGenerator = gen;
    
    std::cout << "Prepared to generate " << enemyCount << " triangle enemies with seed " << seed << std::endl;
}