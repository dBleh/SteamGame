#include "EnemyManager.h"
#include "../../core/Game.h"
#include "../player/PlayerManager.h"
#include "../../network/Host.h"
#include "../../network/Client.h"
#include "../../network/NetworkManager.h"
#include "../../network/messages/MessageHandler.h"
#include "../../network/messages/PlayerMessageHandler.h"
#include "../../network/messages/EnemyMessageHandler.h"
#include "../../network/messages/StateMessageHandler.h"
#include "../../network/messages/SystemMessageHandler.h"
#include <algorithm>
#include <random>
#include <iostream>
#include <cmath>
#include <ctime>
#include <sstream>

EnemyManager::EnemyManager(Game* game, PlayerManager* playerManager)
    : game(game),
      playerManager(playerManager),
      nextEnemyId(1),
      syncTimer(0.0f),
      fullSyncTimer(0.0f),
      lastFullSyncTime(std::chrono::steady_clock::now()),
      currentWave(0) {
    // Initialize random seed
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
}

void EnemyManager::Update(float dt) {
    if (game->GetCurrentState() != GameState::Playing) return;

    if (!queuedEnemies.empty()) {
        UpdateSpawning(dt);
    }

    for (auto& pair : enemies) {
        pair.second->Update(dt, *playerManager);
    }

    CheckPlayerCollisions();

    CSteamID myID = SteamUser()->GetSteamID();
    CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
    if (myID == hostID) {
        // Sync only critical updates (e.g., target changes) every POSITION_SYNC_INTERVAL
        syncTimer += dt;
        if (syncTimer >= POSITION_SYNC_INTERVAL) {
            SyncCriticalUpdates();
            syncTimer = 0.0f;
        }

        // Full sync less frequently to correct drift
        fullSyncTimer += dt;
        if (fullSyncTimer >= FULL_SYNC_INTERVAL) {
            SyncFullState();
            fullSyncTimer = 0.0f;
        }
    }
}

void EnemyManager::Render(sf::RenderWindow& window) {
    for (auto& pair : enemies) {
        pair.second->Render(window);
    }
}

void EnemyManager::InitializeEnemyCallbacks(Enemy* enemy) {
    if (!enemy) return;
    
    // Setup death callback
    enemy->SetDeathCallback([this](int enemyId, const sf::Vector2f& position, const std::string& killerID) {
        HandleEnemyDeath(enemyId, position, killerID);
    });
    
    // Setup damage callback
    enemy->SetDamageCallback([this](int enemyId, float amount, float actualDamage) {
        HandleEnemyDamage(enemyId, amount, actualDamage);
    });
    
    // Setup player collision callback
    enemy->SetPlayerCollisionCallback([this](int enemyId, const std::string& playerID) {
        HandlePlayerCollision(enemyId, playerID);
    });
}


int EnemyManager::AddEnemy(EnemyType type, const sf::Vector2f& position, float health) {
    int id = nextEnemyId++;
    auto enemy = CreateEnemy(type, id, position);
    enemy->SetHealth(health);
    
    // Initialize callbacks for this enemy
    InitializeEnemyCallbacks(enemy.get());
    
    enemies[id] = std::move(enemy);
    
    // Keep track of recently added enemies for sync purposes
    recentlyAddedIds.insert(id);
    
    // If we're the host, broadcast this new enemy to all clients
    CSteamID myID = SteamUser()->GetSteamID();
    CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
    
    if (myID == hostID) {
        std::string addMsg = EnemyMessageHandler::FormatEnemyAddMessage(
            id, type, position, health);
        
        game->GetNetworkManager().BroadcastMessage(addMsg);
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
            std::string removeMsg = EnemyMessageHandler::FormatEnemyRemoveMessage(id);
            game->GetNetworkManager().BroadcastMessage(removeMsg);
        }
        
        // Track recently removed enemies for sync purposes
        recentlyRemovedIds.insert(id);
        
        // Remove from other tracking sets
        recentlyAddedIds.erase(id);
        syncedEnemyIds.erase(id);
        
        // Actually remove the enemy
        enemies.erase(it);
    }
}

void EnemyManager::ClearEnemies() {
    enemies.clear();
    recentlyAddedIds.clear();
    recentlyRemovedIds.clear();
    syncedEnemyIds.clear();
    
    // If we're the host, broadcast a clear command
    CSteamID myID = SteamUser()->GetSteamID();
    CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
    
    if (myID == hostID) {
        game->GetNetworkManager().BroadcastMessage("EC");
    }
}

bool EnemyManager::InflictDamage(int enemyId, float damage) {
    // Use the overloaded version with empty attacker ID
    return InflictDamage(enemyId, damage, "");
}

bool EnemyManager::InflictDamage(int enemyId, float damage, const std::string& attackerID) {
    auto it = enemies.find(enemyId);
    if (it == enemies.end()) {
        return false;
    }
    
    // Use the TakeDamage method that supports attacker tracking
    bool killed = it->second->TakeDamage(damage, attackerID);
    
    // If we're the host, broadcast this damage to all clients
    CSteamID myID = SteamUser()->GetSteamID();
    CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
    
    if (myID == hostID) {
        std::string damageMsg = EnemyMessageHandler::FormatEnemyDamageMessage(
            enemyId, damage, it->second->GetHealth());
        game->GetNetworkManager().BroadcastMessage(damageMsg);
    }
    
    return killed;
}

void EnemyManager::HandleEnemyDeath(int enemyId, const sf::Vector2f& position, const std::string& killerID) {
    std::cout << "[EnemyManager] Enemy " << enemyId << " died at position (" 
              << position.x << "," << position.y << "), killed by " << killerID << std::endl;
    
    // If the enemy is killed by a player, give the player credit
    if (!killerID.empty()) {
        // Give credit to the killer
        playerManager->IncrementPlayerKills(killerID);
        
        // If we're the host, broadcast this kill
        CSteamID myID = SteamUser()->GetSteamID();
        CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
        
        if (myID == hostID) {
            // Create kill message
            std::string killMsg = PlayerMessageHandler::FormatKillMessage(killerID, enemyId);
            game->GetNetworkManager().BroadcastMessage(killMsg);
        }
    }
    
    // Remove the enemy
    RemoveEnemy(enemyId);
}

void EnemyManager::HandleEnemyDamage(int enemyId, float amount, float actualDamage) {
    // Optional: Do something when an enemy takes damage
    // This could include sound effects, visual effects, etc.
}

void EnemyManager::HandlePlayerCollision(int enemyId, const std::string& playerID) {
    // Find the player
    auto& players = playerManager->GetPlayers();
    auto playerIt = players.find(playerID);
    
    if (playerIt != players.end() && !playerIt->second.player.IsDead()) {
        // Apply damage to the player
        playerIt->second.player.TakeDamage(TRIANGLE_DAMAGE);
        
        // If we're the host, broadcast this collision
        CSteamID myID = SteamUser()->GetSteamID();
        CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
        
        if (myID == hostID) {
            // Create player damage message
            std::string damageMsg = PlayerMessageHandler::FormatPlayerDamageMessage(
                playerID, TRIANGLE_DAMAGE, enemyId);
            game->GetNetworkManager().BroadcastMessage(damageMsg);
        }
        
        // Remove the enemy after collision
        RemoveEnemy(enemyId);
    }
}

void EnemyManager::CheckPlayerCollisions() {
    auto& players = playerManager->GetPlayers();
    
    for (auto enemyIt = enemies.begin(); enemyIt != enemies.end();) {
        bool enemyRemoved = false;
        
        for (auto& playerPair : players) {
            if (playerPair.second.player.IsDead()) continue;
            
            if (enemyIt->second->CheckPlayerCollision(playerPair.second.player.GetShape())) {
                // Trigger the player collision callback
                int enemyId = enemyIt->first;
                
                // Call the collision handler directly
                HandlePlayerCollision(enemyId, playerPair.first);
                
                // Enemy has been removed in the handler
                enemyRemoved = true;
                break;
            }
        }
        
        if (!enemyRemoved) {
            ++enemyIt;
        } else {
            // Enemy was removed, get a fresh iterator
            enemyIt = enemies.begin();
            
            // If no more enemies, break out
            if (enemies.empty()) {
                break;
            }
        }
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
    
    // Limit to MAX_ENEMIES_PER_UPDATE enemies per update
    size_t updateCount = std::min(priorities.size(), static_cast<size_t>(MAX_ENEMIES_PER_UPDATE));
    
    // Create vectors for batch update
    std::vector<int> enemyIds;
    std::vector<sf::Vector2f> positions;
    std::vector<sf::Vector2f> velocities;
    
    // Update the synced enemies set
    syncedEnemyIds.clear();
    
    for (size_t i = 0; i < updateCount; ++i) {
        int enemyId = priorities[i];
        auto it = enemies.find(enemyId);
        if (it != enemies.end()) {
            enemyIds.push_back(enemyId);
            positions.push_back(it->second->GetPosition());
            velocities.push_back(it->second->GetVelocity());
            
            // Track which enemies were included in this sync
            syncedEnemyIds.insert(enemyId);
        }
    }
    
    std::string epMessage = EnemyMessageHandler::FormatEnemyPositionUpdateMessage(enemyIds, positions, velocities);
    
    // Check if message is too large
    if (epMessage.length() > MAX_PACKET_SIZE) {
        std::cout << "[EnemyManager] Chunk too large: " << epMessage.length() << std::endl;
        // Either chunk the message or reduce the number of updates
        int maxEnemiesToUpdate = std::max(5, MAX_ENEMIES_PER_UPDATE / 2);
        // Recreate with fewer enemies
        enemyIds.resize(maxEnemiesToUpdate);
        positions.resize(maxEnemiesToUpdate);
        velocities.resize(maxEnemiesToUpdate);
        
        epMessage = EnemyMessageHandler::FormatEnemyPositionUpdateMessage(enemyIds, positions, velocities);
    }
    
    game->GetNetworkManager().BroadcastMessage(epMessage);
}

void EnemyManager::HandleSyncFullState(bool forceSend) {
    auto now = std::chrono::steady_clock::now();
    float timeSinceLastSync = std::chrono::duration<float>(now - lastFullSyncTime).count();
    
    // Don't send too many full syncs in a short time, unless forced
    if (!forceSend && timeSinceLastSync < URGENT_SYNC_THRESHOLD) {
        return;
    }
    
    lastFullSyncTime = now;
    
    // Check if we need to send a full sync
    if (forceSend || !recentlyAddedIds.empty() || !recentlyRemovedIds.empty()) {
        SyncFullState();
        
        // Clear the tracking sets after sync
        recentlyAddedIds.clear();
        recentlyRemovedIds.clear();
    }
}

void EnemyManager::SyncFullState() {
    if (enemies.empty() && recentlyRemovedIds.empty()) return;
    
    // Create vectors for batch update
    std::vector<int> enemyIds;
    std::vector<EnemyType> types;
    std::vector<sf::Vector2f> positions;
    std::vector<float> healths;
    
    // Include all enemies in the full state update
    for (const auto& pair : enemies) {
        enemyIds.push_back(pair.first);
        types.push_back(pair.second->GetType());
        positions.push_back(pair.second->GetPosition());
        healths.push_back(pair.second->GetHealth());
    }
    
    // Use the Complete Enemy State message format for clear differentiation
    std::string fullStateMsg = EnemyMessageHandler::FormatCompleteEnemyStateMessage(
        enemyIds, types, positions, healths);
    
    // Handle large messages with chunking
    if (fullStateMsg.length() > MAX_PACKET_SIZE) {
        std::vector<std::string> chunks = SystemMessageHandler::ChunkMessage(fullStateMsg, "ECS");
        for (const auto& chunk : chunks) {
            game->GetNetworkManager().BroadcastMessage(chunk);
        }
    } else {
        game->GetNetworkManager().BroadcastMessage(fullStateMsg);
    }
    
    std::cout << "[HOST] Sent complete enemy state with " << enemyIds.size() 
              << " enemies" << std::endl;
}

void EnemyManager::ApplyNetworkUpdate(int enemyId, const sf::Vector2f& position, float health) {
    auto it = enemies.find(enemyId);
    if (it != enemies.end()) {
        it->second->SetPosition(position);
        it->second->SetHealth(health);
    } else {
        // Enemy doesn't exist, create it
        RemoteAddEnemy(enemyId, EnemyType::Triangle, position, health);
    }
}

void EnemyManager::RemoteAddEnemy(int enemyId, EnemyType type, const sf::Vector2f& position, float health) {
    // Skip if enemy already exists
    if (enemies.find(enemyId) != enemies.end()) {
        return;
    }
    
    // Create the enemy with the given ID
    auto enemy = CreateEnemy(type, enemyId, position);
    enemy->SetHealth(health);
    
    // Initialize callbacks for this enemy
    InitializeEnemyCallbacks(enemy.get());
    
    // Store the enemy
    enemies[enemyId] = std::move(enemy);
    
    // Update nextEnemyId if necessary
    if (enemyId >= nextEnemyId) {
        nextEnemyId = enemyId + 1;
    }
    
    std::cout << "[CLIENT] Added enemy " << enemyId << " at position ("
              << position.x << "," << position.y << ")" << std::endl;
}

void EnemyManager::RemoteRemoveEnemy(int enemyId) {
    auto it = enemies.find(enemyId);
    if (it != enemies.end()) {
        enemies.erase(it);
        std::cout << "[CLIENT] Removed enemy " << enemyId << std::endl;
    }
}

void EnemyManager::StartNewWave(int enemyCount, EnemyType type) {
    ClearEnemies();
    currentWave++;

    auto& players = playerManager->GetPlayers();
    std::vector<sf::Vector2f> playerPositions;
    for (const auto& pair : players) {
        if (!pair.second.player.IsDead()) {
            playerPositions.push_back(pair.second.player.GetPosition());
        }
    }
    if (playerPositions.empty()) {
        playerPositions.push_back(sf::Vector2f(0.0f, 0.0f));
    }
    playerPositionsCache = playerPositions;

    remainingEnemiesInWave = enemyCount;
    currentWaveEnemyType = type;
    batchSpawnTimer = 0.0f;

    // Queue enemies for spawning instead of adding them immediately
    queuedEnemies.clear();
    for (int i = 0; i < enemyCount; ++i) {
        sf::Vector2f targetPos = playerPositionsCache[rand() % playerPositionsCache.size()];
        sf::Vector2f spawnPos = GetRandomSpawnPosition(targetPos, TRIANGLE_MIN_SPAWN_DISTANCE, TRIANGLE_MAX_SPAWN_DISTANCE);
        queuedEnemies.push_back({nextEnemyId++, type, spawnPos, TRIANGLE_HEALTH});
    }

    // If host, start broadcasting spawn chunks
    CSteamID myID = SteamUser()->GetSteamID();
    CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
    if (myID == hostID) {
        std::string waveMsg = StateMessageHandler::FormatWaveStartMessage(currentWave, enemyCount);
        game->GetNetworkManager().BroadcastMessage(waveMsg);
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
            
            // Keep enemies within culling distance of any player
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
    
    if (!enemiesToRemove.empty()) {
        std::cout << "[OPTIMIZATION] Removed " << enemiesToRemove.size() 
                  << " enemies that were too far from players" << std::endl;
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
        
        // Newly added enemies get a priority boost
        if (recentlyAddedIds.find(pair.first) != recentlyAddedIds.end()) {
            priority *= 1.5f;
        }
        
        // Enemies that weren't synced recently get a boost too
        if (syncedEnemyIds.find(pair.first) == syncedEnemyIds.end()) {
            priority *= 1.2f;
        }
        
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

void EnemyManager::UpdateSpawning(float dt) {
    if (queuedEnemies.empty()) {
        remainingEnemiesInWave = 0;
        return;
    }

    batchSpawnTimer += dt;
    if (batchSpawnTimer < ENEMY_SPAWN_BATCH_INTERVAL) return;

    batchSpawnTimer = 0.0f;
    int spawnCount = std::min(ENEMY_SPAWN_BATCH_SIZE, static_cast<int>(queuedEnemies.size()));
    std::vector<QueuedEnemy> batch(queuedEnemies.begin(), queuedEnemies.begin() + spawnCount);
    queuedEnemies.erase(queuedEnemies.begin(), queuedEnemies.begin() + spawnCount);

    CSteamID myID = SteamUser()->GetSteamID();
    CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
    bool isHost = (myID == hostID);

    // Spawn locally
    for (const auto& queued : batch) {
        auto enemy = CreateEnemy(queued.type, queued.id, queued.position);
        enemy->SetHealth(queued.health);
        InitializeEnemyCallbacks(enemy.get());
        enemies[queued.id] = std::move(enemy);
        recentlyAddedIds.insert(queued.id);
    }

    // If host, send this batch to clients
    if (isHost) {
        std::vector<int> enemyIds;
        std::vector<EnemyType> types;
        std::vector<sf::Vector2f> positions;
        std::vector<float> healths;

        for (const auto& queued : batch) {
            enemyIds.push_back(queued.id);
            types.push_back(queued.type);
            positions.push_back(queued.position);
            healths.push_back(queued.health);
        }

        std::string spawnMsg = EnemyMessageHandler::FormatEnemyStateMessage(enemyIds, types, positions, healths);
        if (spawnMsg.length() > MAX_PACKET_SIZE) {
            std::vector<std::string> chunks = SystemMessageHandler::ChunkMessage(spawnMsg, "ES");
            for (const auto& chunk : chunks) {
                game->GetNetworkManager().BroadcastMessage(chunk);
            }
        } else {
            game->GetNetworkManager().BroadcastMessage(spawnMsg);
        }
    }

    remainingEnemiesInWave = queuedEnemies.size();
}

bool EnemyManager::IsWaveComplete() const {
    // A wave is complete when all enemies have been spawned and eliminated
    return enemies.empty() && remainingEnemiesInWave == 0;
}

Enemy* EnemyManager::FindEnemy(int id) {
    auto it = enemies.find(id);
    return (it != enemies.end()) ? it->second.get() : nullptr;
}

void EnemyManager::RemoveEnemiesNotInList(const std::vector<int>& validIds) {
    // Create a set for more efficient lookups
    std::unordered_set<int> validIdSet(validIds.begin(), validIds.end());
    std::vector<int> enemyIdsToRemove;
    
    // Identify all enemies that aren't in the valid list
    for (const auto& pair : enemies) {
        int currentId = pair.first;
        if (validIdSet.find(currentId) == validIdSet.end()) {
            enemyIdsToRemove.push_back(currentId);
        }
    }
    
    // Then remove them one by one
    for (int id : enemyIdsToRemove) {
        std::cout << "[CLIENT] Removing enemy " << id << " not present in sync message" << std::endl;
        RemoteRemoveEnemy(id);
    }
    
    if (!enemyIdsToRemove.empty()) {
        std::cout << "[CLIENT] Removed " << enemyIdsToRemove.size() 
                  << " enemies to stay in sync with host" << std::endl;
    }
}

void EnemyManager::PrintEnemyStats() const {
    std::cout << "======== ENEMY STATS ========" << std::endl;
    std::cout << "Total enemies: " << enemies.size() << std::endl;
    std::cout << "Current wave: " << currentWave << std::endl;
    std::cout << "Remaining enemies to spawn: " << remainingEnemiesInWave << std::endl;
    std::cout << "Recently added enemies: " << recentlyAddedIds.size() << std::endl;
    std::cout << "Recently removed enemies: " << recentlyRemovedIds.size() << std::endl;
    
    // Print health stats
    if (!enemies.empty()) {
        float totalHealth = 0.0f;
        float minHealth = std::numeric_limits<float>::max();
        float maxHealth = 0.0f;
        
        for (const auto& pair : enemies) {
            float health = pair.second->GetHealth();
            totalHealth += health;
            minHealth = std::min(minHealth, health);
            maxHealth = std::max(maxHealth, health);
        }
        
        std::cout << "Average enemy health: " << (totalHealth / enemies.size()) << std::endl;
        std::cout << "Min health: " << minHealth << ", Max health: " << maxHealth << std::endl;
    }
    
    // Print a few enemy IDs for debugging
    std::cout << "Enemy IDs (first 10): ";
    int count = 0;
    for (const auto& pair : enemies) {
        if (count++ >= 10) break;
        std::cout << pair.first << " ";
    }
    std::cout << std::endl;
    std::cout << "============================" << std::endl;
}

void EnemyManager::SyncCriticalUpdates() {
    std::vector<int> enemyIds;
    std::vector<sf::Vector2f> positions;
    std::vector<sf::Vector2f> velocities;

    for (const auto& pair : enemies) {
        int id = pair.first;
        Enemy* enemy = pair.second.get();

        // Sync if target changed recently or enemy is near a player
        bool shouldSync = recentlyAddedIds.count(id) > 0 || IsNearPlayer(enemy);
        if (shouldSync) {
            enemyIds.push_back(id);
            positions.push_back(enemy->GetPosition());
            velocities.push_back(enemy->GetVelocity());
        }

        if (enemyIds.size() >= MAX_ENEMIES_PER_UPDATE) break;
    }

    if (!enemyIds.empty()) {
        std::string epMessage = EnemyMessageHandler::FormatEnemyPositionUpdateMessage(enemyIds, positions, velocities);
        game->GetNetworkManager().BroadcastMessage(epMessage);
    }
}

bool EnemyManager::IsNearPlayer(Enemy* enemy) {
    auto& players = playerManager->GetPlayers();
    for (const auto& pair : players) {
        if (pair.second.player.IsDead()) continue;
        float distSquared = std::pow(enemy->GetPosition().x - pair.second.player.GetPosition().x, 2) +
                            std::pow(enemy->GetPosition().y - pair.second.player.GetPosition().y, 2);
        if (distSquared < 500.0f * 500.0f) return true; // Within 500 units
    }
    return false;
}