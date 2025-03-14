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

void TriangleEnemyManager::SpawnWave(int enemyCount)
{
    // Generate random positions for new enemies
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> xDist(100.f, 2900.f);
    std::uniform_real_distribution<float> yDist(100.f, 2900.f);
    
    // Track newly created enemies for batch spawn message
    std::vector<std::tuple<int, sf::Vector2f, int>> spawnData;
    spawnData.reserve(enemyCount);
    
    // Create enemies in batches for better performance
    constexpr int BATCH_SIZE = 100;
    for (int i = 0; i < enemyCount; i += BATCH_SIZE) {
        int batchCount = std::min(BATCH_SIZE, enemyCount - i);
        
        for (int j = 0; j < batchCount; ++j) {
            sf::Vector2f position(xDist(gen), yDist(gen));
            
            // Create new enemy with unique ID
            auto enemy = std::make_unique<TriangleEnemy>(nextEnemyId++, position);
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
            
            // Add to spawn data for network message
            spawnData.emplace_back(
                enemyPtr->GetID(),
                position,
                enemyPtr->GetHealth()
            );
        }
    }
    
    // Only send network messages if we're the host
    CSteamID localSteamID = SteamUser()->GetSteamID();
    CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
    
    if (localSteamID == hostID && !spawnData.empty()) {
        // Send batch spawn message to all clients
        std::string batchMsg = MessageHandler::FormatTriangleEnemyBatchSpawnMessage(spawnData);
        game->GetNetworkManager().BroadcastMessage(batchMsg);
        
        // Also do a full sync to ensure all clients are updated
        SyncFullEnemyList();
        
        std::cout << "[HOST] Spawned and broadcast " << enemyCount 
                  << " triangle enemies" << std::endl;
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
void TriangleEnemyManager::CheckBulletCollisions(const std::vector<Bullet>& bullets) {
    // Check each bullet against nearby enemies using spatial partitioning
    for (const auto& bullet : bullets) {
        // Use IsExpired() instead of IsActive()
        if (bullet.IsExpired()) continue;
        
        sf::Vector2f bulletPos = bullet.GetPosition();
        
        // Calculate bullet radius from its shape
        float bulletRadius = bullet.GetShape().getSize().x / 2.0f;
        
        // Get enemies near the bullet
        auto nearbyEnemies = spatialGrid.GetEnemiesNearPosition(bulletPos, bulletRadius + 20.f);
        
        // Check collisions
        for (TriangleEnemy* enemy : nearbyEnemies) {
            if (enemy && enemy->IsAlive() && enemy->CheckBulletCollision(bulletPos, bulletRadius)) {
                // Apply damage with fixed damage amount (20 is the standard in your EnemyManager)
                bool killed = enemy->TakeDamage(20);
                
                // Handle kill
                if (killed) {
                    // Remove from spatial grid
                    spatialGrid.RemoveEnemy(enemy);
                }
                
                // Send hit message
                CSteamID localSteamID = SteamUser()->GetSteamID();
                CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
                
                if (localSteamID == hostID) {
                    // Host sends message to all clients
                    std::string hitMsg = MessageHandler::FormatTriangleEnemyHitMessage(
                        enemy->GetID(), 20, killed, bullet.GetShooterID());
                    game->GetNetworkManager().BroadcastMessage(hitMsg);
                }
                
                break; // Bullet can only hit one enemy
            }
        }
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
void TriangleEnemyManager::CheckPlayerCollisions() {
    // Get player shapes and positions from player manager
    // Use a non-const reference to players since we need to modify them
    auto& players = playerManager->GetPlayers();
    
    for (auto& playerPair : players) {  // Non-const iterating variable
        const std::string& playerID = playerPair.first;
        RemotePlayer& remotePlayer = playerPair.second;  // Now we can get a non-const reference
        
        // Skip dead players
        if (remotePlayer.player.IsDead()) continue;
        
        sf::Vector2f playerPos = remotePlayer.player.GetPosition();
        
        // Get enemies near the player
        auto nearbyEnemies = spatialGrid.GetEnemiesNearPosition(playerPos, 50.f);
        
        // Check collisions
        for (TriangleEnemy* enemy : nearbyEnemies) {
            if (enemy && enemy->IsAlive() && enemy->CheckCollision(remotePlayer.player.GetShape())) {
                // Apply damage to player
                remotePlayer.player.TakeDamage(10); // Example damage value
                
                // Kill enemy on collision
                enemy->TakeDamage(enemy->GetHealth());
                
                // Remove from spatial grid
                spatialGrid.RemoveEnemy(enemy);
                
                // Send death message if we're the host
                CSteamID localSteamID = SteamUser()->GetSteamID();
                CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
                
                if (localSteamID == hostID) {
                    std::string deathMsg = MessageHandler::FormatTriangleEnemyDeathMessage(
                        enemy->GetID(), playerID, false);
                    game->GetNetworkManager().BroadcastMessage(deathMsg);
                    
                    // Also send player damage message
                    std::string damageMsg = MessageHandler::FormatPlayerDamageMessage(
                        playerID, 10, enemy->GetID());
                    game->GetNetworkManager().BroadcastMessage(damageMsg);
                }
            }
        }
    }
}


void TriangleEnemyManager::SyncEnemyPositions()
{
    // Use delta compression to reduce bandwidth
    auto compressedData = GetDeltaCompressedPositions();
    
    // Send only if there's data to send
    if (!compressedData.empty()) {
        // Send message with compressed data
        std::string message = MessageHandler::FormatTriangleEnemyPositionsMessage(compressedData);
        game->GetNetworkManager().BroadcastMessage(message);
        
        // Update last synced positions
        for (const auto& tuple : compressedData) {
            int id = std::get<0>(tuple);
            sf::Vector2f pos = std::get<1>(tuple);
            lastSyncedPositions[id] = pos;
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

void TriangleEnemyManager::SyncFullEnemyList()
{
    std::vector<int> validIds;
    validIds.reserve(enemies.size());
    
    for (const auto& enemy : enemies) {
        if (enemy && enemy->IsAlive()) {
            validIds.push_back(enemy->GetID());
        }
    }
    
    // Send full list message
    std::string message = MessageHandler::FormatTriangleEnemyFullListMessage(validIds);
    game->GetNetworkManager().BroadcastMessage(message);
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
    return spatialGrid.GetEnemyCount();
}

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