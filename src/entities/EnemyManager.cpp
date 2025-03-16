#include "EnemyManager.h"
#include "../Game.h"
#include "../utils/MessageHandler.h"
#include "../entities/PlayerManager.h"
#include <iostream>
#include <cmath>
#include <sstream>
#include <algorithm>

EnemyManager::EnemyManager(Game* game, PlayerManager* playerManager)
    : game(game), 
      playerManager(playerManager),
      currentWave(0),
      waveTimer(3.0f),
      waveActive(false),
      nextEnemyId(0),
      triangleNextEnemyId(10000) { // Start triangle enemy IDs at 10000 to avoid conflicts
    // Initialize random number generator with time-based seed
    rng.seed(static_cast<unsigned int>(std::chrono::system_clock::now().time_since_epoch().count()));
}

EnemyManager::~EnemyManager() {
    // Vectors will auto-clean their contents
}

// Modify the Update method in EnemyManager.cpp to improve target switching
void EnemyManager::Update(float dt) {
    // Static variable to track the last time the spatial grid was updated
    static float timeSinceLastGridUpdate = 0.0f;
    timeSinceLastGridUpdate += dt;
    
    // Start by handling enemy removal more efficiently
    // Only rebuild maps and update spatial grid when needed
    bool removedEnemies = false;
    
    // Remove dead regular enemies
    size_t beforeSize = enemies.size();
    enemies.erase(std::remove_if(enemies.begin(), enemies.end(), 
        [](const EnemyEntry& e) { return !e.IsAlive(); }), enemies.end());
    size_t afterSize = enemies.size();
    
    if (beforeSize != afterSize) {
        std::cout << "Removed " << (beforeSize - afterSize) << " dead regular enemies" << std::endl;
        removedEnemies = true;
    }
    
    // Remove dead triangle enemies
    size_t triangleBeforeSize = triangleEnemies.size();
    triangleEnemies.erase(std::remove_if(triangleEnemies.begin(), triangleEnemies.end(), 
        [](const TriangleEnemy& e) { return !e.IsAlive(); }), triangleEnemies.end());
    size_t triangleAfterSize = triangleEnemies.size();
    
    if (triangleBeforeSize != triangleAfterSize) {
        std::cout << "Removed " << (triangleBeforeSize - triangleAfterSize) << " dead triangle enemies" << std::endl;
        removedEnemies = true;
    }
    
    // If we removed enemies, rebuild the ID to index map and update spatial grid
    if (removedEnemies) {
        enemyIdToIndex.clear();
        for (size_t i = 0; i < enemies.size(); i++) {
            if (enemies[i].enemy) {
                enemyIdToIndex[enemies[i].GetID()] = i;
            }
        }
        
        // Force update spatial grid
        UpdateSpatialGrid();
        timeSinceLastGridUpdate = 0.0f;
    }
    
    // Update the spatial grid periodically to handle moving enemies
    // (but not every frame - only when needed)
    if (timeSinceLastGridUpdate > 0.5f) { // Update every half second
        UpdateSpatialGrid();
        timeSinceLastGridUpdate = 0.0f;
    }
    
    // Handle wave state
    if (!waveActive) {
        // If no wave is active, count down to next wave
        waveTimer -= dt;
        if (waveTimer <= 0.0f) {
            StartNextWave();
        }
        return;
    }
    
    // Update position sync timer and sync if needed
    enemySyncTimer -= dt;
    if (enemySyncTimer <= 0.0f) {
        SyncEnemyPositions();
        enemySyncTimer = ENEMY_SYNC_INTERVAL;
    }
    
    // Update full sync timer for both enemy types
    fullSyncTimer -= dt;
    if (fullSyncTimer <= 0.0f) {
        //SyncFullEnemyList();
        fullSyncTimer = FULL_SYNC_INTERVAL;
    }
    
    // NEW: Check if any live players exist to avoid excessive retargeting
    auto& players = playerManager->GetPlayers();
    bool anyPlayersAlive = std::any_of(players.begin(), players.end(), 
        [](const auto& pair) { return !pair.second.player.IsDead(); });
    
    // If no players are alive, pause enemy movement to avoid visual jitter
    if (!anyPlayersAlive) {
        return;
    }
    
    // Build a target position cache to avoid recalculating for each enemy
    // This significantly reduces CPU usage during retargeting events
    std::unordered_map<int, sf::Vector2f> targetPositionCache;
    
    // Get player center for general targeting
    sf::Vector2f playerCenter = GetPlayerCenterPosition();
    
    // Process in batches for better cache locality and to allow frame splitting
    const size_t BATCH_SIZE = 50; // Process up to 50 enemies per frame to avoid stutter
    static size_t lastProcessedRegular = 0;
    static size_t lastProcessedTriangle = 0;
    
    // Regular enemy batch update
    size_t endIndex = std::min(lastProcessedRegular + BATCH_SIZE, enemies.size());
    for (size_t i = lastProcessedRegular; i < endIndex; i++) {
        auto& entry = enemies[i];
        if (entry.enemy && entry.enemy->IsAlive()) {
            // Generate a unique key for this enemy's position
            int enemyId = entry.GetID();
            sf::Vector2f oldPos = entry.GetPosition();
            
            // Find or calculate target position
            sf::Vector2f targetPos;
            auto it = targetPositionCache.find(enemyId);
            if (it != targetPositionCache.end()) {
                targetPos = it->second;
            } else {
                targetPos = FindClosestPlayerPosition(oldPos);
                targetPositionCache[enemyId] = targetPos;
            }
            
            // Update the enemy with gradual interpolation to smooth movement
            // This helps reduce visual jittering when many enemies change targets
            entry.enemy->Update(dt, targetPos);
            
            // Get new position
            sf::Vector2f newPos = entry.GetPosition();
            
            // Always update the spatial grid if the enemy moved significantly
            if (std::abs(oldPos.x - newPos.x) > 0.5f || std::abs(oldPos.y - newPos.y) > 0.5f) {
                spatialGrid.UpdateEnemyPosition(entry.enemy.get(), oldPos);
            }
        }
    }
    
    // Update lastProcessedRegular for next frame
    lastProcessedRegular = endIndex;
    if (lastProcessedRegular >= enemies.size()) {
        lastProcessedRegular = 0;
    }
    
    // Triangle enemy batch update
    endIndex = std::min(lastProcessedTriangle + BATCH_SIZE, triangleEnemies.size());
    for (size_t i = lastProcessedTriangle; i < endIndex; i++) {
        auto& enemy = triangleEnemies[i];
        if (enemy.IsAlive()) {
            // Generate a unique key for this enemy's position
            int enemyId = enemy.GetID();
            sf::Vector2f oldPos = enemy.GetPosition();
            
            // Find or calculate target position
            sf::Vector2f targetPos;
            auto it = targetPositionCache.find(enemyId);
            if (it != targetPositionCache.end()) {
                targetPos = it->second;
            } else {
                targetPos = FindClosestPlayerPosition(oldPos);
                targetPositionCache[enemyId] = targetPos;
            }
            
            // Update the enemy with gradual interpolation to smooth movement
            enemy.Update(dt, targetPos);
            
            // Get new position
            sf::Vector2f newPos = enemy.GetPosition();
            
            // Always update spatial grid if the enemy moved significantly
            if (std::abs(oldPos.x - newPos.x) > 0.5f || std::abs(oldPos.y - newPos.y) > 0.5f) {
                spatialGrid.UpdateEnemyPosition(&enemy, oldPos);
            }
        }
    }
    
    // Update lastProcessedTriangle for next frame
    lastProcessedTriangle = endIndex;
    if (lastProcessedTriangle >= triangleEnemies.size()) {
        lastProcessedTriangle = 0;
    }
    
    // Check for collisions with players
    CheckPlayerCollisions();
    
    // Check if all enemies are dead
    if (enemies.empty() && triangleEnemies.empty()) {
        // All enemies are dead, start timer for next wave
        waveActive = false;
        waveTimer = waveCooldown;
        
        // Broadcast wave complete message (only host should do this)
        CSteamID localSteamID = SteamUser()->GetSteamID();
        CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
        
        if (localSteamID == hostID) {
            std::string msg = MessageHandler::FormatWaveCompleteMessage(currentWave);
            game->GetNetworkManager().BroadcastMessage(msg);
        }
        
        // Reset batch processing indices
        lastProcessedRegular = 0;
        lastProcessedTriangle = 0;
    }
}


void EnemyManager::Render(sf::RenderWindow& window) {
    // Only render enemies that are in view
    sf::FloatRect viewBounds = sf::FloatRect(
        window.getView().getCenter() - window.getView().getSize() / 2.f,
        window.getView().getSize()
    );
    
    // Render regular enemies
    for (auto& entry : enemies) {
        if (entry.enemy && entry.enemy->IsAlive()) {
            // Check if enemy is in view bounds
            sf::Vector2f pos = entry.GetPosition();
            if (viewBounds.contains(pos)) {
                // Draw enemy based on its type
                switch (entry.type) {
                    case EnemyType::Rectangle: {
                        Enemy* rectEnemy = static_cast<Enemy*>(entry.enemy.get());
                        window.draw(rectEnemy->GetShape());
                        break;
                    }
                    case EnemyType::Triangle: {
                        TriangleEnemy* triEnemy = static_cast<TriangleEnemy*>(entry.enemy.get());
                        window.draw(triEnemy->GetShape());
                        break;
                    }
                }
            }
        }
    }
    
    // Render triangle enemies (separate container during transition)
    for (auto& enemy : triangleEnemies) {
        if (enemy.IsAlive()) {
            sf::Vector2f pos = enemy.GetPosition();
            if (viewBounds.contains(pos)) {
                window.draw(enemy.GetShape());
            }
        }
    }
}

void EnemyManager::StartNextWave() {
    // Force clear all enemies before starting new wave
    if (!enemies.empty()) {
        std::cout << "[EM] Clearing " << enemies.size() << " regular enemies before starting new wave" << std::endl;
        enemies.clear();
        enemyIdToIndex.clear(); // Also clear the ID to index map
    }
    
    if (!triangleEnemies.empty()) {
        std::cout << "[EM] Clearing " << triangleEnemies.size() << " triangle enemies before starting new wave" << std::endl;
        triangleEnemies.clear();
    }
    
    currentWave++;
    std::cout << "Starting wave " << currentWave << std::endl;
    waveActive = true;
    
    // Add a short delay before spawning to ensure everything is reset
    waveCooldown = 0.5f;
    
    // Reset the full sync timer to trigger a sync very soon after wave start
    fullSyncTimer = 0.3f;  // Sync after 0.3 seconds
    
    // Check if we're the host
    CSteamID localSteamID = SteamUser()->GetSteamID();
    CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
    bool isHost = (localSteamID == hostID);
    
    if (isHost) {
        // Only host spawns enemies immediately
        SpawnRegularWave();
        SpawnTriangleWave();
    }
    // Clients will receive spawn messages from the host
}

void EnemyManager::SpawnRegularWave() {
    // Number of regular enemies increases with each wave
    int numEnemies = 4 + currentWave;
    
    // Spawn regular enemies
    for (int i = 0; i < numEnemies; i++) {
        sf::Vector2f pos = GetRandomSpawnPosition();
        int enemyId = nextEnemyId++;
        AddEnemy(enemyId, pos, EnemyType::Rectangle);
        
        // If we're the host, broadcast enemy spawn
        CSteamID localSteamID = SteamUser()->GetSteamID();
        CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
        
        if (localSteamID == hostID) {
            std::string msg = MessageHandler::FormatEnemySpawnMessage(
                enemyId, pos, ParsedMessage::EnemyType::Regular);

            game->GetNetworkManager().BroadcastMessage(msg);
        }
    }
}


// Replace the AddTriangleEnemy methods in EnemyManager.cpp
void EnemyManager::AddTriangleEnemy(int id, const sf::Vector2f& position) {
    // Strict ghost prevention at origin
    if (std::abs(position.x) < 1.0f && std::abs(position.y) < 1.0f) {
        std::cout << "[CLIENT] Prevented ghost triangle creation at origin: ID " << id << "\n";
        return;
    }
    
    // Additional check for very close to origin (likely ghosts)
    if (std::abs(position.x) < 10.0f && std::abs(position.y) < 10.0f) {
        std::cout << "[CLIENT] Prevented suspicious ghost triangle near origin: ID " << id << "\n";
        return;
    }
    
    // Check if triangle enemy already exists
    for (auto& enemy : triangleEnemies) {
        if (enemy.GetID() == id) {
            // Instead of returning, update the existing enemy's position
            enemy.SetTargetPosition(position);
            return;
        }
    }
    
    // Add new triangle enemy
    triangleEnemies.emplace_back(id, position);
    
    // Update nextEnemyId if necessary
    if (id >= triangleNextEnemyId) {
        triangleNextEnemyId = id + 1;
    }
}

void EnemyManager::AddTriangleEnemy(int id, const sf::Vector2f& position, int health) {
    // Strict ghost prevention at origin
    if (std::abs(position.x) < 1.0f && std::abs(position.y) < 1.0f) {
        std::cout << "[CLIENT] Prevented ghost triangle creation at origin: ID " << id << "\n";
        return;
    }
    
    // Additional check for very close to origin (likely ghosts)
    if (std::abs(position.x) < 10.0f && std::abs(position.y) < 10.0f) {
        // Only allow if health makes sense
        if (health <= 0 || health > TRIANGLE_HEALTH) {
            std::cout << "[CLIENT] Prevented suspicious ghost triangle near origin: ID " << id << "\n";
            return;
        }
    }
    
    // Check if triangle enemy already exists
    for (auto& enemy : triangleEnemies) {
        if (enemy.GetID() == id) {
            // Update existing enemy's health and position
            int currentHealth = enemy.GetHealth();
            
            // Only allow reducing health, not increasing it
            if (health < currentHealth) {
                enemy.TakeDamage(currentHealth - health);
            }
            
            // Update position with smooth transition
            enemy.SetTargetPosition(position);
            
            // Force update visuals
            enemy.UpdateVisuals();
            return;
        }
    }
    
    // Validate the health value
    if (health <= 0) {
        health = TRIANGLE_HEALTH; // Use default if invalid
    }
    
    // Add new triangle enemy with specified health
    triangleEnemies.emplace_back(id, position);
    
    // Set health by applying damage
    for (auto& enemy : triangleEnemies) {
        if (enemy.GetID() == id) {
            // Damage it to set correct health (reverse calculation)
            int damage = static_cast<int>(TRIANGLE_HEALTH - health);
            if (damage > 0) {
                enemy.TakeDamage(damage);
            }
            enemy.UpdateVisuals(); // Force update visuals
            break;
        }
    }
    
    // Update nextEnemyId if necessary
    if (id >= triangleNextEnemyId) {
        triangleNextEnemyId = id + 1;
    }
}
sf::Vector2f EnemyManager::GetPlayerCenterPosition() {
    sf::Vector2f centerPos(0.0f, 0.0f);
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
        // If no players are alive (shouldn't happen), use default position
        centerPos = sf::Vector2f(0.0f, 0.0f);
    }
    
    return centerPos;
}

void EnemyManager::SpawnTriangleWave(int count, uint32_t seed) {
    // Initialize random number generator with seed
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * 3.14159f);
    std::uniform_real_distribution<float> distDist(300.0f, 600.0f);
    
    // Get average position of all players for spawn reference
    sf::Vector2f centerPos = GetPlayerCenterPosition();
    
    // Batch data for efficient network transmission
    std::vector<std::tuple<int, sf::Vector2f, int>> batchData;
    
    // Spawn triangle enemies
    for (int i = 0; i < count; i++) {
        // Generate random position around players
        float angle = angleDist(rng);
        float distance = distDist(rng);
        float x = centerPos.x + distance * std::cos(angle);
        float y = centerPos.y + distance * std::sin(angle);
        sf::Vector2f pos(x, y);
        
        // Create new triangle enemy with unique ID
        int enemyId = triangleNextEnemyId++;
        triangleEnemies.emplace_back(enemyId, pos);
        
        // Add to batch data
        batchData.emplace_back(enemyId, pos, TRIANGLE_HEALTH); // Use config health value
        
        // If batch is full or this is the last enemy, send batch
        if (batchData.size() >= 20 || i == count - 1) {
            // If we're the host, broadcast using the consolidated format
            CSteamID localSteamID = SteamUser()->GetSteamID();
            CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
            
            if (localSteamID == hostID) {
                std::string msg = MessageHandler::FormatEnemyBatchSpawnMessage(
                    batchData, ParsedMessage::EnemyType::Triangle);
                game->GetNetworkManager().BroadcastMessage(msg);
            }
            
            // Clear batch for next group
            batchData.clear();
        }
    }
    
    std::cout << "[EM] Spawned " << count << " triangle enemies" << std::endl;
}

void EnemyManager::SpawnWave(int enemyCount, const std::vector<EnemyType>& types) {
    // Set up for gradual spawning
    isSpawningWave = true;
    remainingEnemiestoSpawn = enemyCount;
    spawnTimer = 0.0f;
    
    // Store the types for spawning
    if (types.empty()) {
        // Default to rectangle enemies if no types specified
        spawnTypes.assign(enemyCount, EnemyType::Rectangle);
    } else {
        // Use the provided types, repeating if necessary
        spawnTypes.clear();
        for (int i = 0; i < enemyCount; i++) {
            spawnTypes.push_back(types[i % types.size()]);
        }
    }
    
    // If we're the host, broadcast wave start
    CSteamID localSteamID = SteamUser()->GetSteamID();
    CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
    
    if (localSteamID == hostID) {
        // Create a seed for consistent random spawning across clients
        uint32_t seed = static_cast<uint32_t>(std::chrono::system_clock::now().time_since_epoch().count());
        
        // Convert EnemyType enum values to ints for message
        std::vector<int> typeInts;
        for (const auto& type : spawnTypes) {
            typeInts.push_back(static_cast<int>(type));
        }
        
        // Send wave start message with types and seed
        std::string msg = MessageHandler::FormatWaveStartWithTypesMessage(
            currentWave, seed, typeInts);
        game->GetNetworkManager().BroadcastMessage(msg);
        
        std::cout << "[HOST] Broadcast wave " << currentWave << " start with " 
                  << enemyCount << " enemies and " << types.size() << " type(s)" << std::endl;
    }
}

void EnemyManager::SpawnEnemyBatch(int count) {
    // Don't spawn more than we need
    int batchCount = std::min(count, remainingEnemiestoSpawn);
    remainingEnemiestoSpawn -= batchCount;
    
    // If done spawning, update flag
    if (remainingEnemiestoSpawn <= 0) {
        isSpawningWave = false;
    }
    
    // Prepare a batch of enemy data for efficient network transmission
    std::vector<std::tuple<int, sf::Vector2f, int>> batchData; // id, pos, health
    batchData.reserve(batchCount);
    
    // Spawn the batch
    for (int i = 0; i < batchCount; i++) {
        // Get the enemy type from our spawn types
        EnemyType type = EnemyType::Rectangle; // Default
        if (!spawnTypes.empty()) {
            type = spawnTypes[spawnTypes.size() - remainingEnemiestoSpawn - i - 1];
        }
        
        // Generate random spawn position
        sf::Vector2f position = GetRandomSpawnPosition();
        
        // Add enemy
        int enemyId = nextEnemyId++;
        AddEnemy(enemyId, position, type);
        
        // Add to batch data for network transmission
        batchData.emplace_back(
            enemyId, 
            position, 
            ENEMY_HEALTH // Use correct health from config
        );
    }
    
    // Send batch spawn message to clients if we're the host
    if (!batchData.empty()) {
        CSteamID localSteamID = SteamUser()->GetSteamID();
        CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
        
        if (localSteamID == hostID) {
            std::string batchMsg = MessageHandler::FormatEnemyBatchSpawnMessage(
                batchData, ParsedMessage::EnemyType::Regular); // 0 = Regular type
            game->GetNetworkManager().BroadcastMessage(batchMsg);
        }
    }
}

// Replace the AddEnemy method in EnemyManager.cpp to more strictly prevent ghost enemies
void EnemyManager::AddEnemy(int id, const sf::Vector2f& position, EnemyType type, int health) {
    // Strict ghost enemy prevention - reject any enemies with suspicious positions
    if (std::abs(position.x) < 1.0f && std::abs(position.y) < 1.0f) {
        std::cout << "[CLIENT] Prevented ghost enemy creation at origin: ID " << id << "\n";
        return;
    }
    
    // Additional check for very close to origin (likely ghosts)
    if (std::abs(position.x) < 10.0f && std::abs(position.y) < 10.0f) {
        // Only allow enemies near origin if they have specific health values that make sense
        // This helps distinguish between genuine spawns and network artifacts
        if (health <= 0 || health > TRIANGLE_HEALTH) {
            std::cout << "[CLIENT] Prevented suspicious ghost enemy near origin: ID " << id << "\n";
            return;
        }
    }
    
    // Check if this is a client and the enemy has already been registered
    CSteamID localSteamID = SteamUser()->GetSteamID();
    CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
    bool isClient = (localSteamID != hostID);
    
    if (isClient) {
        // First check if this enemy already exists
        bool enemyExists = false;
        if (id >= 10000) {
            enemyExists = HasTriangleEnemy(id);
        } else {
            enemyExists = HasEnemy(id);
        }
        
        if (enemyExists) {
            // Don't recreate existing enemies, just update their properties
            if (id >= 10000) {
                // Update triangle enemy health
                UpdateTriangleEnemyHealth(id, health);
                
                // Update position through the proper channels with interpolation
                TriangleEnemy* enemy = GetTriangleEnemy(id);
                if (enemy) {
                    enemy->SetTargetPosition(position);
                }
            } else {
                // Update regular enemy health
                UpdateEnemyHealth(id, health);
                
                // Update through proper channels
                auto it = enemyIdToIndex.find(id);
                if (it != enemyIdToIndex.end() && it->second < enemies.size() && enemies[it->second].enemy) {
                    enemies[it->second].enemy->SetPosition(position);
                }
            }
            return;
        }
    }
    
    // If health is not specified, use the appropriate default health value
    int actualHealth = (health <= 0) ? 
        ((type == EnemyType::Triangle) ? TRIANGLE_HEALTH : ENEMY_HEALTH) : 
        health;
    
    // Create the enemy with the proper health
    std::unique_ptr<EnemyBase> enemy = CreateEnemy(id, position, type, actualHealth);
    EnemyBase* enemyPtr = enemy.get();
    
    // Force update visuals to ensure correct color
    if (enemy) {
        enemy->UpdateVisuals();
    }
    
    // Create entry and store it
    EnemyEntry entry;
    entry.enemy = std::move(enemy);
    entry.type = type;
    entry.lastSyncedPosition = position;
    
    // Add to the vector
    enemies.push_back(std::move(entry));
    
    // Update the ID to index map
    enemyIdToIndex[id] = enemies.size() - 1;
    
    // Add to spatial grid for collision detection
    spatialGrid.AddEnemy(enemyPtr);
    
    // Update next enemy ID if necessary
    if (id >= nextEnemyId) {
        nextEnemyId = id + 1;
    }

    // Debug message to verify health is set correctly
    std::cout << "Added enemy " << id << " with health: " << actualHealth << "\n";
}
void EnemyManager::RemoveEnemy(int id) {
    auto it = enemyIdToIndex.find(id);
    if (it != enemyIdToIndex.end()) {
        size_t index = it->second;
        
        // Remove from spatial grid
        if (index < enemies.size() && enemies[index].enemy) {
            spatialGrid.RemoveEnemy(enemies[index].enemy.get());
        }
        
        // Mark the enemy as dead
        if (index < enemies.size() && enemies[index].enemy) {
            enemies[index].enemy->TakeDamage(1000); // Ensure it's dead
            std::cout << "Enemy " << id << " marked as dead" << std::endl;
        }
        
        // Remove from the ID to index map
        enemyIdToIndex.erase(it);
    }
}

void EnemyManager::CheckBulletCollisions(const std::vector<Bullet>& bullets) {
    // Early exit if no bullets or enemies
    if (bullets.empty() || (enemies.empty() && triangleEnemies.empty())) {
        return;
    }

    // Track which bullets hit something
    std::vector<size_t> bulletsToRemove;
    bulletsToRemove.reserve(bullets.size() / 4); // Reserve reasonable capacity
    
    // Check if we're the host
    CSteamID localSteamID = SteamUser()->GetSteamID();
    CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
    bool isHost = (localSteamID == hostID);
    
    // Simplified for better performance - check all enemies against each bullet
    for (size_t bulletIndex = 0; bulletIndex < bullets.size(); bulletIndex++) {
        const Bullet& bullet = bullets[bulletIndex];
        sf::Vector2f bulletPos = bullet.GetPosition();
        float bulletRadius = 4.0f; // Bullet radius
        
        // Get nearby enemies - use a smaller search radius for better performance
        std::vector<EnemyBase*> nearbyEnemies;
        float searchRadius = bulletRadius + ENEMY_SIZE;
        
        try {
            nearbyEnemies = spatialGrid.GetNearbyEnemies(bulletPos, searchRadius);
        } 
        catch (const std::exception& e) {
            // If spatial grid fails, fallback to direct checking as a last resort
            // This should rarely happen with our improved implementation
            std::cout << "Spatial grid failed, using fallback: " << e.what() << std::endl;
            
            // Simple proximity check for regular enemies
            for (auto& entry : enemies) {
                if (entry.enemy && entry.enemy->IsAlive()) {
                    sf::Vector2f enemyPos = entry.GetPosition();
                    float dx = enemyPos.x - bulletPos.x;
                    float dy = enemyPos.y - bulletPos.y;
                    float distSquared = dx * dx + dy * dy;
                    
                    if (distSquared <= (searchRadius * searchRadius)) {
                        nearbyEnemies.push_back(entry.enemy.get());
                    }
                }
            }
            
            // Simple proximity check for triangle enemies
            for (auto& enemy : triangleEnemies) {
                if (enemy.IsAlive()) {
                    sf::Vector2f enemyPos = enemy.GetPosition();
                    float dx = enemyPos.x - bulletPos.x;
                    float dy = enemyPos.y - bulletPos.y;
                    float distSquared = dx * dx + dy * dy;
                    
                    if (distSquared <= (searchRadius * searchRadius)) {
                        nearbyEnemies.push_back(&enemy);
                    }
                }
            }
        }
        
        // If no nearby enemies, continue to next bullet
        if (nearbyEnemies.empty()) {
            continue;
        }
        
        // Check for collisions with nearby enemies
        bool bulletHit = false;
        for (EnemyBase* baseEnemy : nearbyEnemies) {
            if (!baseEnemy || !baseEnemy->IsAlive()) continue;
            
            try {
                // Determine enemy type and check collision
                bool collided = false;
                int enemyId = baseEnemy->GetID();
                EnemyType enemyType;
                
                // Check if it's a triangle enemy (IDs >= 10000)
                if (enemyId >= 10000) {
                    TriangleEnemy* triEnemy = static_cast<TriangleEnemy*>(baseEnemy);
                    collided = triEnemy->CheckBulletCollision(bulletPos, bulletRadius);
                    enemyType = EnemyType::Triangle;
                } else {
                    Enemy* rectEnemy = static_cast<Enemy*>(baseEnemy);
                    collided = rectEnemy->CheckBulletCollision(bulletPos, bulletRadius);
                    enemyType = EnemyType::Rectangle;
                }
                
                if (collided) {
                    // Log the hit for debugging
                    std::cout << "Bullet hit enemy #" << enemyId 
                              << " of type " << (enemyType == EnemyType::Triangle ? "Triangle" : "Rectangle") 
                              << std::endl;
                    
                    // Handle collision
                    if (isHost) {
                        // Host applies actual damage
                        bool killed = baseEnemy->TakeDamage(20); // Bullet damage
                        baseEnemy->UpdateVisuals();
                        
                        // Send hit message with appropriate type
                        ParsedMessage::EnemyType msgType = 
                            (enemyType == EnemyType::Triangle) ? 
                            ParsedMessage::EnemyType::Triangle : 
                            ParsedMessage::EnemyType::Regular;
                        
                        std::string msg = MessageHandler::FormatEnemyHitMessage(
                            enemyId, 20, killed, bullet.GetShooterID(), msgType);
                        game->GetNetworkManager().BroadcastMessage(msg);
                        
                        // If enemy was killed, handle rewards
                        if (killed) {
                            RewardShooter(bullet.GetShooterID(), enemyType);
                            
                            // Remove from spatial grid when killed
                            spatialGrid.RemoveEnemy(baseEnemy);
                        }
                    } else {
                        // Client only applies visual feedback
                        baseEnemy->UpdateVisuals();
                        
                        // Send hit message to host
                        ParsedMessage::EnemyType msgType = 
                            (enemyType == EnemyType::Triangle) ? 
                            ParsedMessage::EnemyType::Triangle : 
                            ParsedMessage::EnemyType::Regular;
                        
                        std::string msg = MessageHandler::FormatEnemyHitMessage(
                            enemyId, 20, false, bullet.GetShooterID(), msgType);
                        game->GetNetworkManager().SendMessage(hostID, msg);
                    }
                    
                    // Mark bullet for removal
                    bulletsToRemove.push_back(bulletIndex);
                    bulletHit = true;
                    break; // A bullet can only hit one enemy
                }
            } 
            catch (const std::exception& e) {
                std::cout << "Error checking bullet collision: " << e.what() << std::endl;
                continue; // Skip this enemy
            }
        }
        
        // If bullet hit something, skip to next bullet
        if (bulletHit) continue;
    }
    
    // Remove bullets that hit enemies
    if (!bulletsToRemove.empty()) {
        playerManager->RemoveBullets(bulletsToRemove);
    }
}

void EnemyManager::CheckPlayerCollisions() {
    auto& players = playerManager->GetPlayers();
    
    // Check if we're the host
    CSteamID localSteamID = SteamUser()->GetSteamID();
    CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
    bool isHost = (localSteamID == hostID);
    std::string localSteamIDStr = std::to_string(SteamUser()->GetSteamID().ConvertToUint64());
    
    // Process each player
    for (auto& playerPair : players) {
        const std::string& playerID = playerPair.first;
        RemotePlayer& remotePlayer = playerPair.second;
        
        // Skip dead players
        if (remotePlayer.player.IsDead()) continue;
        
        sf::Vector2f playerPos = remotePlayer.player.GetPosition();
        const sf::RectangleShape& playerShape = remotePlayer.player.GetShape();
        
        // Create a copy of nearby enemies to avoid issues if collection is modified during iteration
        float collisionRadius = ENEMY_SIZE * 2; // Conservative estimate
        std::vector<EnemyBase*> nearbyEnemies;
        
        try {
            // Get nearby enemies
            nearbyEnemies = spatialGrid.GetNearbyEnemies(playerPos, collisionRadius);
        } catch (const std::exception& e) {
            std::cerr << "Exception in GetNearbyEnemies: " << e.what() << std::endl;
            continue; // Skip this player if there's an error
        }
        
        // Keep track of enemies to remove
        std::vector<EnemyBase*> enemiesToRemove;
        
        // Check collisions with nearby enemies
        for (EnemyBase* baseEnemy : nearbyEnemies) {
            if (!baseEnemy || !baseEnemy->IsAlive()) continue;
            
            try {
                int enemyId = baseEnemy->GetID();
                bool collision = false;
                int damage = 0;
                EnemyType enemyType;
                ParsedMessage::EnemyType msgType;
                
                // Determine enemy type and check collision
                if (enemyId >= 10000) { // Triangle enemy
                    TriangleEnemy* triEnemy = static_cast<TriangleEnemy*>(baseEnemy);
                    collision = triEnemy->CheckCollision(playerShape);
                    damage = TRIANGLE_DAMAGE;
                    enemyType = EnemyType::Triangle;
                    msgType = ParsedMessage::EnemyType::Triangle;
                } else { // Regular enemy
                    Enemy* rectEnemy = static_cast<Enemy*>(baseEnemy);
                    collision = rectEnemy->CheckCollision(playerShape);
                    damage = 20; // Default damage
                    enemyType = EnemyType::Rectangle;
                    msgType = ParsedMessage::EnemyType::Regular;
                }
                
                if (collision) {
                    // Apply damage to player
                    remotePlayer.player.TakeDamage(damage);
                    
                    // Only host should kill the enemy and send messages
                    if (isHost) {
                        // Mark enemy for removal to avoid modifying collection during iteration
                        enemiesToRemove.push_back(baseEnemy);
                        
                        // Send enemy death message - broadcast twice for reliability
                        std::string deathMsg = MessageHandler::FormatEnemyDeathMessage(
                            enemyId, "", false, msgType);
                        game->GetNetworkManager().BroadcastMessage(deathMsg);
                        game->GetNetworkManager().BroadcastMessage(deathMsg); // Send twice
                        
                        // Also send player damage message
                        std::string damageMsg = MessageHandler::FormatPlayerDamageMessage(
                            playerID, damage, enemyId);
                        game->GetNetworkManager().BroadcastMessage(damageMsg);
                    } else {
                        // For clients, don't kill the enemy locally - let host tell us
                        // Just show visual effect
                        baseEnemy->UpdateVisuals();
                    }
                    
                    // If this is the local player, check if they died
                    if (playerID == localSteamIDStr && remotePlayer.player.IsDead()) {
                        // Set respawn timer
                        remotePlayer.respawnTimer = 3.0f;
                        std::cout << "[DEATH] Local player died from enemy collision" << std::endl;
                    }
                }
            } catch (const std::exception& e) {
                std::cerr << "Exception processing enemy collision: " << e.what() << std::endl;
                continue; // Skip this enemy if there's an error
            }
        }
        
        // Now safely remove enemies marked for deletion
        for (EnemyBase* enemyToRemove : enemiesToRemove) {
            try {
                int id = enemyToRemove->GetID();
                
                // Kill enemy on collision
                enemyToRemove->TakeDamage(enemyToRemove->GetHealth());
                
                // Remove from spatial grid
                spatialGrid.RemoveEnemy(enemyToRemove);
                
                std::cout << "[COLLISION] Enemy #" << id << " killed after colliding with player " << playerID << "\n";
            } catch (const std::exception& e) {
                std::cerr << "Exception removing enemy after collision: " << e.what() << std::endl;
            }
        }
    }
}

std::unique_ptr<EnemyBase> EnemyManager::CreateEnemy(int id, const sf::Vector2f& position, EnemyType type, int health) {
    switch (type) {
        case EnemyType::Rectangle:
            // Create a standard rectangular enemy
            return std::make_unique<Enemy>(id, position, ENEMY_SPEED, health);
            
        case EnemyType::Triangle:
            // Create a triangle enemy (faster than rectangle enemies)
            return std::make_unique<TriangleEnemy>(id, position, ENEMY_SPEED * 1.2f, health);
            
        default:
            // Default to rectangle enemy if type is unknown
            std::cerr << "Unknown enemy type: " << static_cast<int>(type) << ", using Rectangle" << std::endl;
            return std::make_unique<Enemy>(id, position, ENEMY_SPEED, health);
    }
}

void EnemyManager::HandleEnemyHit(int enemyId, int damage, bool killed, const std::string& shooterID, ParsedMessage::EnemyType enemyType) {
    // Determine enemy type and route to appropriate handler
    if (enemyType == ParsedMessage::EnemyType::Triangle || enemyId >= 10000) { // Triangle (type 1) or IDs start at 10000
        HandleTriangleEnemyHit(enemyId, damage, killed, shooterID);
    } else {
        // Regular enemy hit handling
        auto it = enemyIdToIndex.find(enemyId);
        if (it != enemyIdToIndex.end() && it->second < enemies.size() && enemies[it->second].enemy) {
            auto& entry = enemies[it->second];
            
            // Apply damage based on role
            CSteamID localSteamID = SteamUser()->GetSteamID();
            CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
            bool isHost = (localSteamID == hostID);
            
            if (isHost) {
                // The host is authoritative for enemy health
                bool wasKilled = entry.enemy->TakeDamage(damage);
                
                // Force update visuals after damage
                entry.enemy->UpdateVisuals();
                
                // If killed flag is true but enemy isn't dead yet, force kill
                if (killed && !wasKilled) {
                    entry.enemy->TakeDamage(1000); // Ensure it dies
                }
                
                // Handle shooter rewards if enemy died and this wasn't done yet
                if (killed) {
                    RewardShooter(shooterID, entry.type);
                }
            } else {
                // For clients, apply visual effect and let host be authoritative
                if (killed) {
                    entry.enemy->TakeDamage(entry.enemy->GetHealth()); // Force kill
                } else {
                    // Apply damage for visual feedback only
                    int currentHealth = entry.enemy->GetHealth();
                    entry.enemy->SetHealth(currentHealth - damage);
                    
                    // Force update visuals after health change
                    entry.enemy->UpdateVisuals();
                }
            }
        }
    }
}

// Replace the existing FindClosestPlayerPosition method in EnemyManager.cpp
sf::Vector2f EnemyManager::FindClosestPlayerPosition(const sf::Vector2f& enemyPos) {
    sf::Vector2f closestPos = enemyPos; // Default: don't move if no players
    float closestDist = std::numeric_limits<float>::max();
    
    // Track if we've found any alive players
    bool foundAlivePlayer = false;
    
    // First pass: try to find the closest alive player
    auto& players = playerManager->GetPlayers();
    for (const auto& pair : players) {
        // Skip dead players
        if (pair.second.player.IsDead()) continue;
        
        sf::Vector2f playerPos = pair.second.player.GetPosition();
        float dx = playerPos.x - enemyPos.x;
        float dy = playerPos.y - enemyPos.y;
        float dist = std::sqrt(dx * dx + dy * dy);
        
        if (dist < closestDist) {
            closestDist = dist;
            closestPos = playerPos;
            foundAlivePlayer = true;
        }
    }
    
    // If no alive players found, return current position to prevent all enemies 
    // from suddenly moving to the origin or another default location
    if (!foundAlivePlayer) {
        return enemyPos; // Stay in place until a player respawns
    }
    
    return closestPos;
}

sf::Vector2f EnemyManager::GetRandomSpawnPosition() {
    // Get spawn radius from config
    float spawnRadius = SPAWN_RADIUS;
    
    // Get average position of all players
    sf::Vector2f centerPos(0.0f, 0.0f);
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
        // If no players are alive (shouldn't happen), use default position
        centerPos = sf::Vector2f(0.0f, 0.0f);
    }
    
    // Generate random angle and distance within spawn circle
    std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * 3.14159f);
    std::uniform_real_distribution<float> distDist(TRIANGLE_MIN_SPAWN_DISTANCE, spawnRadius);
    
    float angle = angleDist(rng);
    float distance = distDist(rng);
    
    // Calculate position on circle
    float x = centerPos.x + distance * std::cos(angle);
    float y = centerPos.y + distance * std::sin(angle);
    
    return sf::Vector2f(x, y);
}

void EnemyManager::UpdateSpatialGrid() {
    // Clear the grid
    spatialGrid.Clear();
    
    // Add all living enemies to the grid
    for (auto& entry : enemies) {
        if (entry.enemy && entry.enemy->IsAlive()) {
            spatialGrid.AddEnemy(entry.enemy.get());
        }
    }
}

// Replace the SyncEnemyPositions method in EnemyManager.cpp
void EnemyManager::SyncEnemyPositions() {
    CSteamID localSteamID = SteamUser()->GetSteamID();
    CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
    
    // Only host syncs enemy positions
    if (localSteamID != hostID) {
        return;
    }
    
    // Calculate center of all players
    sf::Vector2f playerCenter = GetPlayerCenterPosition();
    
    // NEW: Check if any players are alive to avoid unnecessary syncs
    auto& players = playerManager->GetPlayers();
    bool anyPlayersAlive = std::any_of(players.begin(), players.end(), 
        [](const auto& pair) { return !pair.second.player.IsDead(); });
        
    if (!anyPlayersAlive) {
        // Don't sync positions if there are no alive players to avoid jitter
        return;
    }
    
    // Create a single combined list of enemy data with priority info
    std::vector<EnemySyncPriority> allEnemiesWithPriority;
    
    // Add regular enemies
    for (auto& entry : enemies) {
        if (entry.enemy && entry.enemy->IsAlive()) {
            sf::Vector2f currentPos = entry.GetPosition();
            sf::Vector2f delta = currentPos - entry.lastSyncedPosition;
            float moveDist = std::sqrt(delta.x * delta.x + delta.y * delta.y);
            
            // Calculate distance to player center
            float distToPlayers = std::sqrt(
                std::pow(currentPos.x - playerCenter.x, 2) + 
                std::pow(currentPos.y - playerCenter.y, 2)
            );
            
            // NEW: More sophisticated priority calculation that reduces network traffic
            // 1. Movement still matters (x3 instead of x6)
            // 2. Being close to players still matters (1000 instead of 1500)
            // 3. Higher threshold for update (1.0 instead of 0.3) to reduce network traffic
            float priority = (moveDist * 3.0f) + (1000.0f / (distToPlayers + 100.0f));
            
            // Only prioritize if moved significantly or very close to players
            // The higher threshold significantly reduces network traffic during mass retargeting
            if (priority > 1.0f || moveDist > 5.0f || distToPlayers < 200.0f) {
                allEnemiesWithPriority.emplace_back(
                    entry.GetID(),
                    currentPos,
                    entry.GetHealth(),
                    priority,
                    false  // not a triangle
                );
                
                // Update last synced position
                entry.lastSyncedPosition = currentPos;
            }
        }
    }
    
    // Add triangle enemies
    for (auto& enemy : triangleEnemies) {
        if (enemy.IsAlive()) {
            sf::Vector2f currentPos = enemy.GetPosition();
            sf::Vector2f lastPos = enemy.GetLastPosition();
            sf::Vector2f delta = currentPos - lastPos;
            float moveDist = std::sqrt(delta.x * delta.x + delta.y * delta.y);
            
            // Calculate distance to player center
            float distToPlayers = std::sqrt(
                std::pow(currentPos.x - playerCenter.x, 2) + 
                std::pow(currentPos.y - playerCenter.y, 2)
            );
            
            // Same improved priority calculation with higher thresholds
            float priority = (moveDist * 3.0f) + (1000.0f / (distToPlayers + 100.0f));
            
            // Only prioritize if moved significantly or very close to players
            if (priority > 1.0f || moveDist > 5.0f || distToPlayers < 200.0f) {
                allEnemiesWithPriority.emplace_back(
                    enemy.GetID(),
                    currentPos,
                    enemy.GetHealth(),
                    priority,
                    true  // is a triangle
                );
                
                // Update last position
                enemy.SetLastPosition(currentPos);
            }
        }
    }
    
    // Sort by priority (highest first)
    std::sort(allEnemiesWithPriority.begin(), allEnemiesWithPriority.end(),
        [](const auto& a, const auto& b) {
            return a.priority > b.priority;
        }
    );
    
    // NEW: Reduced batch size when all players respawn to avoid network flood
    // This handles the retargeting scenario when a player dies and respawns
    bool isRespawnPhase = false;
    for (auto& pair : players) {
        if (pair.second.respawnTimer > 0 && pair.second.respawnTimer < 1.0f) {
            isRespawnPhase = true;
            break;
        }
    }
    
    // Adjust max enemies per sync based on game state
    const int MAX_ENEMIES_PER_SYNC = isRespawnPhase ? 8 : 12;
    
    // Limit number of enemies to send
    if (allEnemiesWithPriority.size() > MAX_ENEMIES_PER_SYNC) {
        allEnemiesWithPriority.resize(MAX_ENEMIES_PER_SYNC);
    }
    
    // Split into separate regular and triangle lists
    std::vector<std::tuple<int, sf::Vector2f, int>> regularData;
    std::vector<std::tuple<int, sf::Vector2f, int>> triangleData;
    
    for (const auto& enemy : allEnemiesWithPriority) {
        if (enemy.isTriangle) {
            triangleData.emplace_back(enemy.id, enemy.position, enemy.health);
        } else {
            regularData.emplace_back(enemy.id, enemy.position, enemy.health);
        }
    }
    
    // Send regular enemy positions
    if (!regularData.empty()) {
        std::string batchMsg = MessageHandler::FormatEnemyPositionsMessage(regularData);
        game->GetNetworkManager().BroadcastMessage(batchMsg);
    }
    
    // Send triangle enemy positions
    if (!triangleData.empty()) {
        // Add small delay to avoid network congestion
        sf::sleep(sf::milliseconds(5));
        std::string batchMsg = MessageHandler::FormatEnemyPositionsMessage(triangleData);
        game->GetNetworkManager().BroadcastMessage(batchMsg);
    }
}


TriangleEnemy* EnemyManager::GetTriangleEnemy(int id) {
    for (auto& enemy : triangleEnemies) {
        if (enemy.GetID() == id) {
            return &enemy;
        }
    }
    return nullptr;
}

std::vector<int> EnemyManager::GetAllTriangleEnemyIds() const {
    std::vector<int> ids;
    ids.reserve(triangleEnemies.size());
    for (const auto& enemy : triangleEnemies) {
        ids.push_back(enemy.GetID());
    }
    return ids;
}

int EnemyManager::GetTriangleEnemyHealth(int id) const {
    for (const auto& enemy : triangleEnemies) {
        if (enemy.GetID() == id) {
            return enemy.GetHealth();
        }
    }
    return 0; // Not found or dead
}

void EnemyManager::ValidateTriangleEnemyList(const std::vector<int>& validIds) {
    // First, identify triangle enemies that shouldn't exist
    std::vector<int> localIds = GetAllTriangleEnemyIds();
    std::vector<int> toRemove;
    
    // Find IDs in our local list that aren't in the valid list
    for (int localId : localIds) {
        bool found = false;
        for (int validId : validIds) {
            if (localId == validId) {
                found = true;
                break;
            }
        }
        
        if (!found) {
            toRemove.push_back(localId);
        }
    }
    
    // Remove triangle enemies that shouldn't exist
    for (int id : toRemove) {
        std::cout << "[CLIENT] Removing ghost triangle enemy: " << id << std::endl;
        // Find and remove the specific triangle enemy
        for (auto it = triangleEnemies.begin(); it != triangleEnemies.end(); ) {
            if (it->GetID() == id) {
                it = triangleEnemies.erase(it);
            } else {
                ++it;
            }
        }
    }
    
    // Now check for missing triangle enemies that should exist
    for (int validId : validIds) {
        if (!HasTriangleEnemy(validId)) {
            // Request the enemy's data from the host
            std::cout << "[CLIENT] Requesting missing triangle enemy: " << validId << std::endl;
            
            // Create a placeholder triangle enemy at (0,0) that will get updated
            // in the next position sync
            AddTriangleEnemy(validId, sf::Vector2f(0.f, 0.f));
        }
    }
}

void EnemyManager::UpdateTriangleEnemyHealth(int id, int health) {
    TriangleEnemy* enemy = GetTriangleEnemy(id);
    if (enemy && enemy->IsAlive()) {
        int currentHealth = enemy->GetHealth();
        if (health < currentHealth) {
            // Can only reduce health, not increase it
            enemy->TakeDamage(currentHealth - health);
        }
        // Always force update visuals
        enemy->UpdateVisuals();
    }
}

// Replace the UpdateTriangleEnemyPositions method in EnemyManager.cpp
void EnemyManager::UpdateTriangleEnemyPositions(const std::vector<std::tuple<int, sf::Vector2f, int>>& positions) {
    // Track successful updates for logging
    int updatedCount = 0;
    int skippedCount = 0;
    
    for (const auto& enemyData : positions) {
        int id = std::get<0>(enemyData);
        sf::Vector2f position = std::get<1>(enemyData);
        int health = std::get<2>(enemyData);
        
        // Skip any positions at exact origin (0,0) - likely ghost enemies
        if (std::abs(position.x) < 1.0f && std::abs(position.y) < 1.0f) {
            skippedCount++;
            continue;
        }
        
        // Skip positions that are suspiciously close to origin unless they have valid health
        if (std::abs(position.x) < 10.0f && std::abs(position.y) < 10.0f) {
            if (health <= 0 || health > TRIANGLE_HEALTH) {
                skippedCount++;
                continue;
            }
        }
        
        // Find the enemy
        TriangleEnemy* enemy = GetTriangleEnemy(id);
        if (enemy && enemy->IsAlive()) {
            // Calculate distance to new position
            sf::Vector2f currentPos = enemy->GetPosition();
            float distSquared = 
                (position.x - currentPos.x) * (position.x - currentPos.x) +
                (position.y - currentPos.y) * (position.y - currentPos.y);
            
            // For very large position changes (likely targeting a new player),
            // use the smoother SetTargetPosition method
            if (distSquared > 10000.0f) { // Over 100 units away
                enemy->SetTargetPosition(position);
            } 
            // For medium changes, use UpdatePosition with interpolation
            else if (distSquared > 25.0f) { // More than 5 units away
                enemy->UpdatePosition(position, true); // true = interpolate
            }
            // For very small changes, don't update to avoid jitter
            else if (distSquared > 1.0f) { // At least 1 unit of movement
                enemy->UpdatePosition(position, true); // true = interpolate
            }
            // For tiny movements, ignore to reduce network traffic and visual jitter
            
            // Update health if it's significantly different
            if (std::abs(enemy->GetHealth() - health) > 5) {
                // Set health directly to avoid unnecessary visual effects
                if (health <= 0) {
                    enemy->TakeDamage(enemy->GetHealth()); // Kill it
                } else if (health < enemy->GetHealth()) {
                    enemy->TakeDamage(enemy->GetHealth() - health);
                }
                // Force update visuals
                enemy->UpdateVisuals();
            }
            
            updatedCount++;
        } else if (health > 0) {
            // Enemy doesn't exist locally but should - create it
            AddTriangleEnemy(id, position, health);
            updatedCount++;
        }
    }
    
    if (skippedCount > 0) {
        std::cout << "[EM] Skipped " << skippedCount << " suspicious triangle enemy positions" << std::endl;
    }
}

void EnemyManager::HandleTriangleEnemyHit(int enemyId, int damage, bool killed, const std::string& shooterID) {
    TriangleEnemy* enemy = GetTriangleEnemy(enemyId);
    if (!enemy || !enemy->IsAlive()) {
        return;
    }
    
    // Check if we're host or client
    CSteamID localSteamID = SteamUser()->GetSteamID();
    CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
    bool isHost = (localSteamID == hostID);
    
    if (isHost) {
        // As host, we may have already applied damage in CheckBulletCollisions
        // Only ensure the enemy is dead if it should be
        if (killed && enemy->IsAlive()) {
            std::cout << "[HOST] Forcing triangle enemy " << enemyId << " to die due to network sync\n";
            enemy->TakeDamage(1000);
            
            // Reward the shooter if provided
            if (!shooterID.empty()) {
                RewardShooter(shooterID, EnemyType::Triangle);
            }
        }
        
        // Force update visuals
        enemy->UpdateVisuals();
    } else {
        // As client, apply the damage from the network message
        // This is our authoritative source of enemy health
        
        // Update health and visuals
        if (killed) {
            enemy->TakeDamage(enemy->GetHealth()); // Kill it
        } else {
            enemy->TakeDamage(damage);
        }
        
        // Force update visuals
        enemy->UpdateVisuals();
    }
}

bool EnemyManager::HasTriangleEnemy(int id) const {
    for (const auto& enemy : triangleEnemies) {
        if (enemy.GetID() == id) {
            return true;
        }
    }
    return false;
}

std::vector<std::tuple<int, sf::Vector2f, int>> EnemyManager::GetRegularEnemyDataForSync() const {
    std::vector<std::tuple<int, sf::Vector2f, int>> enemyData;
    
    for (const auto& entry : enemies) {
        if (entry.enemy && entry.enemy->IsAlive()) {
            enemyData.emplace_back(
                entry.GetID(),
                entry.GetPosition(),
                entry.GetHealth()
            );
        }
    }
    
    return enemyData;
}

std::vector<std::tuple<int, sf::Vector2f, int>> EnemyManager::GetTriangleEnemyDataForSync() const {
    std::vector<std::tuple<int, sf::Vector2f, int>> enemyData;
    
    for (const auto& enemy : triangleEnemies) {
        if (enemy.IsAlive()) {
            enemyData.emplace_back(
                enemy.GetID(),
                enemy.GetPosition(),
                enemy.GetHealth()
            );
        }
    }
    
    return enemyData;
}

std::vector<std::tuple<int, sf::Vector2f, int, int>> EnemyManager::GetEnemyDataForSync() const {
    std::vector<std::tuple<int, sf::Vector2f, int, int>> enemyData;
    
    for (const auto& entry : enemies) {
        if (entry.enemy && entry.enemy->IsAlive()) {
            enemyData.emplace_back(
                entry.GetID(),
                entry.GetPosition(),
                entry.GetHealth(),
                static_cast<int>(entry.type)
            );
        }
    }
    
    return enemyData;
}

void EnemyManager::SyncFullEnemyList() {
    // Static variable to track the last sync time
    static auto lastFullSyncTime = std::chrono::steady_clock::now() - std::chrono::seconds(10);
    auto now = std::chrono::steady_clock::now();
    
    // Limit full syncs to once every 2 seconds max to prevent flooding
    float timeSinceLast = std::chrono::duration<float>(now - lastFullSyncTime).count();
    if (timeSinceLast < 2.0f) {
        return;
    }
    
    // Update last sync time
    lastFullSyncTime = now;
    
    CSteamID localSteamID = SteamUser()->GetSteamID();
    CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
    
    // Only host sends full enemy lists
    if (localSteamID != hostID) {
        return;
    }
    
    // Efficient approach: Use small batch sizes and send more frequently
    const int BATCH_SIZE = 8; // Smaller batch size for full sync
    
    // Calculate center of all players for prioritization
    sf::Vector2f playerCenter = GetPlayerCenterPosition();
    
    // Collect all live enemies with priority info
    std::vector<std::tuple<int, sf::Vector2f, int, float>> allEnemiesWithPriority;
    
    // Add regular enemies
    for (const auto& entry : enemies) {
        if (entry.enemy && entry.enemy->IsAlive()) {
            sf::Vector2f pos = entry.GetPosition();
            
            // Calculate distance to players
            float dx = pos.x - playerCenter.x;
            float dy = pos.y - playerCenter.y;
            float distToPlayers = std::sqrt(dx * dx + dy * dy);
            
            // Priority based on distance (closer = higher priority)
            float priority = 1000.0f / (distToPlayers + 10.0f);
            
            allEnemiesWithPriority.emplace_back(
                entry.GetID(),
                pos,
                entry.GetHealth(),
                priority
            );
        }
    }
    
    // Add triangle enemies
    for (const auto& enemy : triangleEnemies) {
        if (enemy.IsAlive()) {
            sf::Vector2f pos = enemy.GetPosition();
            
            // Calculate distance to players
            float dx = pos.x - playerCenter.x;
            float dy = pos.y - playerCenter.y;
            float distToPlayers = std::sqrt(dx * dx + dy * dy);
            
            // Priority based on distance (closer = higher priority)
            float priority = 1000.0f / (distToPlayers + 10.0f);
            
            allEnemiesWithPriority.emplace_back(
                enemy.GetID(),
                pos,
                enemy.GetHealth(),
                priority
            );
        }
    }
    
    // Sort by priority (highest first)
    std::sort(allEnemiesWithPriority.begin(), allEnemiesWithPriority.end(),
        [](const auto& a, const auto& b) {
            return std::get<3>(a) > std::get<3>(b);
        }
    );
    
    // First, send validation lists for both enemy types
    std::vector<int> regularIds;
    std::vector<int> triangleIds;
    
    for (const auto& enemyData : allEnemiesWithPriority) {
        int id = std::get<0>(enemyData);
        if (id >= 10000) {
            triangleIds.push_back(id);
        } else {
            regularIds.push_back(id);
        }
    }
    
    // Send validation messages
    if (!regularIds.empty()) {
        std::string validationMsg = MessageHandler::FormatEnemyValidationMessage(regularIds);
        game->GetNetworkManager().BroadcastMessage(validationMsg);
    }
    
    if (!triangleIds.empty()) {
        std::string validationMsg = MessageHandler::FormatEnemyValidationMessage(triangleIds);
        game->GetNetworkManager().BroadcastMessage(validationMsg);
    }
    
    // Small delay to ensure validation messages are processed first
    sf::sleep(sf::milliseconds(20));
    
    // Limit to top N highest priority enemies
    const size_t MAX_ENEMIES_TO_SYNC = 50;
    if (allEnemiesWithPriority.size() > MAX_ENEMIES_TO_SYNC) {
        allEnemiesWithPriority.resize(MAX_ENEMIES_TO_SYNC);
    }
    
    // Process enemies in small batches, separating by type
    std::vector<std::tuple<int, sf::Vector2f, int>> regularBatch;
    std::vector<std::tuple<int, sf::Vector2f, int>> triangleBatch;
    
    for (const auto& enemyData : allEnemiesWithPriority) {
        int id = std::get<0>(enemyData);
        sf::Vector2f pos = std::get<1>(enemyData);
        int health = std::get<2>(enemyData);
        
        if (id >= 10000) {
            // Triangle enemy
            triangleBatch.emplace_back(id, pos, health);
            
            // Send batch if full
            if (triangleBatch.size() >= BATCH_SIZE) {
                std::string triangleMsg = MessageHandler::FormatEnemyBatchSpawnMessage(
                    triangleBatch, ParsedMessage::EnemyType::Triangle);
                game->GetNetworkManager().BroadcastMessage(triangleMsg);
                
                // Clear batch and add delay
                triangleBatch.clear();
                sf::sleep(sf::milliseconds(10));
            }
        } else {
            // Regular enemy
            regularBatch.emplace_back(id, pos, health);
            
            // Send batch if full
            if (regularBatch.size() >= BATCH_SIZE) {
                std::string regularMsg = MessageHandler::FormatEnemyBatchSpawnMessage(
                    regularBatch, ParsedMessage::EnemyType::Regular);
                game->GetNetworkManager().BroadcastMessage(regularMsg);
                
                // Clear batch and add delay
                regularBatch.clear();
                sf::sleep(sf::milliseconds(10));
            }
        }
    }
    
    // Send any remaining enemies in partial batches
    if (!triangleBatch.empty()) {
        std::string triangleMsg = MessageHandler::FormatEnemyBatchSpawnMessage(
            triangleBatch, ParsedMessage::EnemyType::Triangle);
        game->GetNetworkManager().BroadcastMessage(triangleMsg);
    }
    
    if (!regularBatch.empty()) {
        std::string regularMsg = MessageHandler::FormatEnemyBatchSpawnMessage(
            regularBatch, ParsedMessage::EnemyType::Regular);
        game->GetNetworkManager().BroadcastMessage(regularMsg);
    }
    
    std::cout << "[HOST] Completed full enemy sync with priority-based selection" << std::endl;
}

void EnemyManager::ValidateEnemyList(const std::vector<int>& validIds) {
    // Create a set for faster lookups
    std::unordered_set<int> validIdSet(validIds.begin(), validIds.end());
    std::vector<int> toRemove;
    
    // Find enemies that shouldn't exist
    for (auto& entry : enemies) {
        if (entry.enemy) {
            int id = entry.GetID();
            if (validIdSet.find(id) == validIdSet.end()) {
                toRemove.push_back(id);
            }
        }
    }
    
    // Remove invalid enemies
    for (int id : toRemove) {
        RemoveEnemy(id);
    }
    
    // Find missing enemies
    for (int validId : validIds) {
        if (enemyIdToIndex.find(validId) == enemyIdToIndex.end()) {
            // Missing enemy - add a placeholder at origin
            // It will be updated with correct position in next sync
            AddEnemy(validId, sf::Vector2f(0, 0), EnemyType::Rectangle);
        }
    }
}

std::vector<int> EnemyManager::GetAllEnemyIds() const {
    std::vector<int> ids;
    for (const auto& entry : enemies) {
        if (entry.enemy && entry.enemy->IsAlive()) {
            ids.push_back(entry.GetID());
        }
    }
    return ids;
}

bool EnemyManager::HasEnemy(int id) const {
    return enemyIdToIndex.find(id) != enemyIdToIndex.end();
}

int EnemyManager::GetEnemyHealth(int id) const {
    auto it = enemyIdToIndex.find(id);
    if (it != enemyIdToIndex.end() && it->second < enemies.size() && enemies[it->second].enemy) {
        return enemies[it->second].enemy->GetHealth();
    }
    return 0;
}

EnemyType EnemyManager::GetEnemyType(int id) const {
    auto it = enemyIdToIndex.find(id);
    if (it != enemyIdToIndex.end() && it->second < enemies.size()) {
        return enemies[it->second].type;
    }
    return EnemyType::Rectangle; // Default
}

int EnemyManager::GetRemainingEnemies() const {
    int count = 0;
    // Count regular enemies
    for (const auto& entry : enemies) {
        if (entry.enemy && entry.enemy->IsAlive()) {
            count++;
        }
    }
    // Count triangle enemies
    for (const auto& enemy : triangleEnemies) {
        if (enemy.IsAlive()) {
            count++;
        }
    }
    return count;
}

void EnemyManager::UpdateEnemyPositions(const std::vector<std::tuple<int, sf::Vector2f, int>>& positionData) {
    for (const auto& data : positionData) {
        int id = std::get<0>(data);
        sf::Vector2f newPosition = std::get<1>(data);
        int health = std::get<2>(data);
        
        // First check if this is a triangle enemy (IDs >= 10000)
        if (id >= 10000) {
            for (auto& enemy : triangleEnemies) {
                if (enemy.GetID() == id) {
                    // Calculate distance for significant movement detection
                    float distance = std::sqrt(
                        std::pow(enemy.GetPosition().x - newPosition.x, 2) +
                        std::pow(enemy.GetPosition().y - newPosition.y, 2)
                    );
                    
                    // Use interpolation for significant movement
                    if (distance > 5.0f) {
                        enemy.SetTargetPosition(newPosition);
                    } 
                    // Direct update for minor adjustments
                    else if (distance > 0.1f) {
                        enemy.UpdatePosition(newPosition, false);
                    }
                    
                    // Update health if needed
                    if (std::abs(enemy.GetHealth() - health) > 5) {
                        if (health <= 0) {
                            enemy.TakeDamage(enemy.GetHealth()); // Kill it
                        } else if (health < enemy.GetHealth()) {
                            enemy.TakeDamage(enemy.GetHealth() - health);
                        }
                        enemy.UpdateVisuals();
                    }
                    
                    break;
                }
            }
        } 
        // Otherwise it's a regular enemy
        else {
            // Find the enemy in our vector
            auto it = enemyIdToIndex.find(id);
            if (it != enemyIdToIndex.end() && it->second < enemies.size() && enemies[it->second].enemy) {
                auto& entry = enemies[it->second];
                
                // Calculate distance for significant movement detection
                float distance = std::sqrt(
                    std::pow(entry.GetPosition().x - newPosition.x, 2) +
                    std::pow(entry.GetPosition().y - newPosition.y, 2)
                );
                
                // Use interpolation for significant movement
                if (distance > 5.0f) {
                    // Cast to Enemy* to access specific methods
                    Enemy* enemy = static_cast<Enemy*>(entry.enemy.get());
                    enemy->SetTargetPosition(newPosition);
                } 
                // Direct update for minor adjustments
                else if (distance > 0.1f) {
                    entry.enemy->SetPosition(newPosition);
                }
                
                // Update health if significantly different
                if (std::abs(entry.GetHealth() - health) > 5) {
                    if (health <= 0) {
                        entry.enemy->TakeDamage(entry.GetHealth()); // Kill it
                    } else if (health < entry.GetHealth()) {
                        entry.enemy->TakeDamage(entry.GetHealth() - health);
                    } else {
                        entry.enemy->SetHealth(health);
                    }
                    entry.enemy->UpdateVisuals();
                }
            }
        }
    }
}

void EnemyManager::UpdateEnemyHealth(int id, int health) {
    auto it = enemyIdToIndex.find(id);
    if (it != enemyIdToIndex.end() && it->second < enemies.size() && enemies[it->second].enemy) {
        enemies[it->second].enemy->SetHealth(health);
        // Force update visuals after health change
        enemies[it->second].enemy->UpdateVisuals();
    }
}

void EnemyManager::RewardShooter(const std::string& shooterID, EnemyType enemyType) {
    auto& players = playerManager->GetPlayers();
    auto it = players.find(shooterID);
    if (it != players.end()) {
        int reward = 0;
        switch (enemyType) {
            case EnemyType::Rectangle:
                reward = 10; // Regular enemy reward
                break;
            case EnemyType::Triangle:
                reward = TRIANGLE_KILL_REWARD; // From config
                break;
            default:
                reward = 10; // Default reward
        }
        
        it->second.money += reward;
        it->second.kills++;
        
        // Update player stats in PlayerManager
        playerManager->AddOrUpdatePlayer(shooterID, it->second);
    }
}

std::string EnemyManager::SerializeEnemies() const {
    std::stringstream ss;
    
    // First count active enemies
    int activeCount = 0;
    for (const auto& entry : enemies) {
        if (entry.enemy && entry.enemy->IsAlive()) {
            activeCount++;
        }
    }
    
    // Write count
    ss << activeCount;
    
    // Then write each enemy
    for (const auto& entry : enemies) {
        if (entry.enemy && entry.enemy->IsAlive()) {
            ss << ";" << static_cast<int>(entry.type) << ":" << entry.enemy->Serialize();
        }
    }
    
    return ss.str();
}
// Replace the ClearAllEnemies method in EnemyManager.cpp
void EnemyManager::ClearAllEnemies() {
    // Clear spatial grid first
    spatialGrid.Clear();
    
    // Log counts before clearing
    size_t regularCount = enemies.size();
    size_t triangleCount = triangleEnemies.size();
    
    // Clear enemies
    if (!enemies.empty()) {
        std::cout << "[EM] Clearing " << regularCount << " regular enemies" << std::endl;
        enemies.clear();
        enemyIdToIndex.clear();
    }
    
    if (!triangleEnemies.empty()) {
        std::cout << "[EM] Clearing " << triangleCount << " triangle enemies" << std::endl;
        triangleEnemies.clear();
    }
    
    // Reset any static variables or counters that might affect future enemy creation
    lastProcessedRegularIndex = 0;
    lastProcessedTriangleIndex = 0;
    
    // Reset enemy sync timers to force an immediate sync after clearing
    if (triangleCount > 0 || regularCount > 0) {
        enemySyncTimer = 0.1f;  // Request a sync soon
        fullSyncTimer = 0.2f;   // Request a full sync shortly after
    }
    
    // We need to inform players of this action if we're the host
    CSteamID localSteamID = SteamUser()->GetSteamID();
    CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
    
    if (localSteamID == hostID && (regularCount > 0 || triangleCount > 0)) {
        // Create a basic message to inform clients that enemies were cleared
        std::string clearMsg = MessageHandler::FormatEnemyClearMessage();
        game->GetNetworkManager().BroadcastMessage(clearMsg);
        
        std::cout << "[HOST] Broadcast enemy clear message to all clients" << std::endl;
    }
}

void EnemyManager::DeserializeEnemies(const std::string& data) {
    // Clear existing enemies
    enemies.clear();
    enemyIdToIndex.clear();
    
    std::stringstream ss(data);
    int enemyCount;
    ss >> enemyCount;
    
    char separator;
    for (int i = 0; i < enemyCount; i++) {
        ss >> separator; // Read semicolon
        
        int type;
        ss >> type >> separator; // Read type and colon
        
        std::string enemyData;
        std::getline(ss, enemyData, ';');
        
        if (!enemyData.empty()) {
            EnemyType enemyType = static_cast<EnemyType>(type);
            
            // Create enemy based on type
            if (enemyType == EnemyType::Rectangle) {
                Enemy enemy = Enemy::Deserialize(enemyData);
                AddEnemy(enemy.GetID(), enemy.GetPosition(), EnemyType::Rectangle, enemy.GetHealth());
            } else if (enemyType == EnemyType::Triangle) {
                TriangleEnemy enemy = TriangleEnemy::Deserialize(enemyData);
                AddEnemy(enemy.GetID(), enemy.GetPosition(), EnemyType::Triangle, enemy.GetHealth());
            }
        }
    }
}