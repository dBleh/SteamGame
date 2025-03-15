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

void EnemyManager::Update(float dt) {
    // Update regular enemies
    if (!waveActive) {
        // If no wave is active, count down to next wave
        waveTimer -= dt;
        if (waveTimer <= 0.0f) {
            StartNextWave();
        }
        return;
    }
    
    // Update full sync timer for both enemy types
    fullSyncTimer -= dt;
    if (fullSyncTimer <= 0.0f) {
        SyncFullEnemyList();
        fullSyncTimer = FULL_SYNC_INTERVAL;
    }
    
    // Remove dead regular enemies
    size_t beforeSize = enemies.size();
    enemies.erase(std::remove_if(enemies.begin(), enemies.end(), 
        [](const EnemyEntry& e) { return !e.IsAlive(); }), enemies.end());
    size_t afterSize = enemies.size();
    
    if (beforeSize != afterSize) {
        std::cout << "Removed " << (beforeSize - afterSize) << " dead regular enemies" << std::endl;
    }
    
    // Remove dead triangle enemies
    size_t triangleBeforeSize = triangleEnemies.size();
    triangleEnemies.erase(std::remove_if(triangleEnemies.begin(), triangleEnemies.end(), 
        [](const TriangleEnemy& e) { return !e.IsAlive(); }), triangleEnemies.end());
    size_t triangleAfterSize = triangleEnemies.size();
    
    if (triangleBeforeSize != triangleAfterSize) {
        std::cout << "Removed " << (triangleBeforeSize - triangleAfterSize) << " dead triangle enemies" << std::endl;
    }
    
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
        
        return;
    }
    
    // Update each regular enemy
    for (auto& entry : enemies) {
        if (entry.enemy && entry.enemy->IsAlive()) {
            // Find closest player position
            sf::Vector2f targetPos = FindClosestPlayerPosition(entry.GetPosition());
            entry.enemy->Update(dt, targetPos);
        }
    }
    
    // Update each triangle enemy
    for (auto& enemy : triangleEnemies) {
        if (enemy.IsAlive()) {
            // Find closest player position
            sf::Vector2f targetPos = FindClosestPlayerPosition(enemy.GetPosition());
            enemy.Update(dt, targetPos);
        }
    }
    
    // Update position sync timer and sync if needed
    enemySyncTimer -= dt;
    if (enemySyncTimer <= 0.0f) {
        SyncEnemyPositions();
        enemySyncTimer = ENEMY_SYNC_INTERVAL;
    }
    
    // Check for collisions with players
    CheckPlayerCollisions();
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
    }
    
    if (!triangleEnemies.empty()) {
        std::cout << "[EM] Clearing " << triangleEnemies.size() << " triangle enemies before starting new wave" << std::endl;
        triangleEnemies.clear();
    }
    
    currentWave++;
    std::cout << "Starting wave " << currentWave << std::endl;
    waveActive = true;
    
    // Spawn both enemy types
    SpawnRegularWave();
    SpawnTriangleWave();
    
    // Reset the full sync timer to trigger a sync soon after wave start
    fullSyncTimer = 1.0f;  // Sync after 1 second
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

void EnemyManager::AddTriangleEnemy(int id, const sf::Vector2f& position) {
    // Check if triangle enemy already exists
    for (auto& enemy : triangleEnemies) {
        if (enemy.GetID() == id) {
            return; // Already exists
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
    // Check if triangle enemy already exists
    for (auto& enemy : triangleEnemies) {
        if (enemy.GetID() == id) {
            return; // Already exists
        }
    }
    
    // Add new triangle enemy with specified health
    triangleEnemies.emplace_back(id, position);
    
    // Set health
    for (auto& enemy : triangleEnemies) {
        if (enemy.GetID() == id) {
            // Damage it to set correct health (reverse calculation)
            int damage = 40 - health; // 40 is default health
            if (damage > 0) {
                enemy.TakeDamage(damage);
            }
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
        batchData.emplace_back(enemyId, pos, 40); // Default health is 40
        
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
            TRIANGLE_HEALTH // Default health from config
        );
    }
    
    // Send batch spawn message to clients if we're the host
    if (!batchData.empty()) {
        CSteamID localSteamID = SteamUser()->GetSteamID();
        CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
        
        if (localSteamID == hostID) {
            std::string batchMsg = MessageHandler::FormatEnemyBatchSpawnMessage(
                batchData, static_cast<ParsedMessage::EnemyType>(1)); // 0 = Regular type
            game->GetNetworkManager().BroadcastMessage(batchMsg);
        }
    }
}

void EnemyManager::AddEnemy(int id, const sf::Vector2f& position, EnemyType type, int health) {
    // Create the enemy
    std::unique_ptr<EnemyBase> enemy = CreateEnemy(id, position, type, health);
    EnemyBase* enemyPtr = enemy.get();
    
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
    // Create a vector to track which bullets need to be removed
    std::vector<size_t> bulletsToRemove;
    
    // Check regular enemies
    for (size_t bulletIndex = 0; bulletIndex < bullets.size(); bulletIndex++) {
        const Bullet& bullet = bullets[bulletIndex];
        bool bulletHit = false;
        
        // Check against regular enemies
        for (auto& entry : enemies) {
            if (entry.enemy && entry.enemy->IsAlive()) {
                // Get the actual enemy instance based on type
                bool collided = false;
                
                if (entry.type == EnemyType::Rectangle) {
                    Enemy* rectEnemy = static_cast<Enemy*>(entry.enemy.get());
                    collided = rectEnemy->GetShape().getGlobalBounds().intersects(bullet.GetShape().getGlobalBounds());
                } else if (entry.type == EnemyType::Triangle) {
                    TriangleEnemy* triEnemy = static_cast<TriangleEnemy*>(entry.enemy.get());
                    collided = triEnemy->CheckBulletCollision(bullet.GetPosition(), 4.0f);
                }
                
                if (collided) {
                    // Enemy hit by bullet
                    CSteamID localSteamID = SteamUser()->GetSteamID();
                    CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
                    bool isHost = (localSteamID == hostID);
                    
                    if (isHost) {
                        // ONLY THE HOST APPLIES ACTUAL DAMAGE
                        bool killed = entry.enemy->TakeDamage(20); // Each bullet does 20 damage
                        
                        // Host immediately broadcasts hit message
                        std::string msg = MessageHandler::FormatEnemyHitMessage(
                            entry.GetID(), 20, killed, bullet.GetShooterID(), 
                            static_cast<ParsedMessage::EnemyType>(static_cast<int>(entry.type)));
                        game->GetNetworkManager().BroadcastMessage(msg);
                        
                        // If enemy was killed, handle rewards
                        if (killed) {
                            RewardShooter(bullet.GetShooterID(), entry.type);
                        }
                    } else {
                        // CLIENT ONLY APPLIES VISUAL EFFECT - NO DAMAGE
                        // Send hit message to host
                        std::string msg = MessageHandler::FormatEnemyHitMessage(
                            entry.GetID(), 20, false, bullet.GetShooterID(), 
                            static_cast<ParsedMessage::EnemyType>(static_cast<int>(entry.type)));
                        game->GetNetworkManager().SendMessage(hostID, msg);
                    }
                    
                    // Mark bullet for immediate removal
                    bulletsToRemove.push_back(bulletIndex);
                    bulletHit = true;
                    break; // A bullet can only hit one enemy
                }
            }
        }
        
        if (bulletHit) continue; // Skip to next bullet
        
        // If bullet didn't hit a regular enemy, check triangle enemies
        for (auto& enemy : triangleEnemies) {
            if (enemy.IsAlive()) {
                // Use the triangle-specific collision detection
                if (enemy.CheckBulletCollision(bullet.GetPosition(), 4.0f)) { // 4.0f is bullet radius
                    // Triangle enemy hit by bullet
                    CSteamID localSteamID = SteamUser()->GetSteamID();
                    CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
                    bool isHost = (localSteamID == hostID);
                    
                    if (isHost) {
                        // ONLY THE HOST APPLIES ACTUAL DAMAGE
                        bool killed = enemy.TakeDamage(20); // Each bullet does 20 damage
                        
                        // Host immediately broadcasts hit message
                        std::string msg = MessageHandler::FormatEnemyHitMessage(
                            enemy.GetID(), 20, killed, bullet.GetShooterID(), ParsedMessage::EnemyType::Triangle);
                        game->GetNetworkManager().BroadcastMessage(msg);
                        
                        // If enemy was killed, handle rewards
                        if (killed) {
                            RewardShooter(bullet.GetShooterID(), EnemyType::Triangle);
                        }
                    } else {
                        // CLIENT ONLY APPLIES VISUAL EFFECT - NO DAMAGE
                        // Send hit message to host
                        std::string msg = MessageHandler::FormatEnemyHitMessage(
                            enemy.GetID(), 20, false, bullet.GetShooterID(), ParsedMessage::EnemyType::Triangle);
                        game->GetNetworkManager().SendMessage(hostID, msg);
                    }
                    
                    // Mark bullet for immediate removal
                    bulletsToRemove.push_back(bulletIndex);
                    bulletHit = true;
                    break; // A bullet can only hit one enemy
                }
            }
        }
    }
    
    // Immediately remove bullets that hit enemies
    if (!bulletsToRemove.empty()) {
        playerManager->RemoveBullets(bulletsToRemove);
    }
}

void EnemyManager::CheckPlayerCollisions() {
    auto& players = playerManager->GetPlayers();
    
    for (auto& playerPair : players) {
        const std::string& playerID = playerPair.first;
        RemotePlayer& remotePlayer = playerPair.second;
        
        // Skip dead players
        if (remotePlayer.player.IsDead()) continue;
        
        sf::Vector2f playerPos = remotePlayer.player.GetPosition();
        
        // Check collisions with regular enemies
        for (auto& entry : enemies) {
            if (!(entry.enemy && entry.enemy->IsAlive())) continue;
            
            bool collision = false;
            if (entry.type == EnemyType::Rectangle) {
                Enemy* rectEnemy = static_cast<Enemy*>(entry.enemy.get());
                collision = rectEnemy->GetShape().getGlobalBounds().intersects(remotePlayer.player.GetShape().getGlobalBounds());
            } else if (entry.type == EnemyType::Triangle) {
                TriangleEnemy* triEnemy = static_cast<TriangleEnemy*>(entry.enemy.get());
                collision = triEnemy->CheckCollision(remotePlayer.player.GetShape());
            }
            
            if (collision) {
                // Determine damage based on enemy type
                int damage = (entry.type == EnemyType::Triangle) ? 
                    TRIANGLE_DAMAGE : 20; // Default damage for rectangle enemies
                
                // Apply damage to player
                remotePlayer.player.TakeDamage(damage);
                
                // Kill enemy on collision
                entry.enemy->TakeDamage(entry.enemy->GetHealth());
                
                // Remove from spatial grid if in it
                spatialGrid.RemoveEnemy(entry.enemy.get());
                
                // Send death message if we're the host
                CSteamID localSteamID = SteamUser()->GetSteamID();
                CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
                
                if (localSteamID == hostID) {
                    // Send enemy death message
                    std::string deathMsg = MessageHandler::FormatEnemyDeathMessage(
                        entry.GetID(), "", false, 
                        static_cast<ParsedMessage::EnemyType>(static_cast<int>(entry.type)));
                    game->GetNetworkManager().BroadcastMessage(deathMsg);
                    
                    // Also send player damage message
                    std::string damageMsg = MessageHandler::FormatPlayerDamageMessage(
                        playerID, damage, entry.GetID());
                    game->GetNetworkManager().BroadcastMessage(damageMsg);
                }
                
                // If this is the local player, check if they died
                std::string localSteamIDStr = std::to_string(SteamUser()->GetSteamID().ConvertToUint64());
                if (playerID == localSteamIDStr && remotePlayer.player.IsDead()) {
                    // Set respawn timer
                    remotePlayer.respawnTimer = 3.0f;
                    
                    std::cout << "[DEATH] Local player died from enemy collision" << std::endl;
                }
            }
        }
        
        // Check collisions with triangle enemies
        for (auto& enemy : triangleEnemies) {
            if (!enemy.IsAlive()) continue;
            
            if (enemy.CheckCollision(remotePlayer.player.GetShape())) {
                // Determine damage
                int damage = TRIANGLE_DAMAGE;
                
                // Apply damage to player
                remotePlayer.player.TakeDamage(damage);
                
                // Kill enemy on collision
                enemy.TakeDamage(enemy.GetHealth());
                
                // Send death message if we're the host
                CSteamID localSteamID = SteamUser()->GetSteamID();
                CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
                
                if (localSteamID == hostID) {
                    // Send enemy death message
                    std::string deathMsg = MessageHandler::FormatEnemyDeathMessage(
                        enemy.GetID(), "", false, ParsedMessage::EnemyType::Triangle);
                    game->GetNetworkManager().BroadcastMessage(deathMsg);
                    
                    // Also send player damage message
                    std::string damageMsg = MessageHandler::FormatPlayerDamageMessage(
                        playerID, damage, enemy.GetID());
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
                    // Apply temporary visual damage
                    entry.enemy->SetHealth(entry.enemy->GetHealth() - damage);
                    entry.enemy->UpdateVisuals();
                }
            }
        }
    }
}

sf::Vector2f EnemyManager::FindClosestPlayerPosition(const sf::Vector2f& enemyPos) {
    sf::Vector2f closestPos = enemyPos; // Default: don't move if no players
    float closestDist = std::numeric_limits<float>::max();
    
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
        }
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

void EnemyManager::SyncEnemyPositions() {
    CSteamID localSteamID = SteamUser()->GetSteamID();
    CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
    
    // Only host syncs enemy positions
    if (localSteamID != hostID) {
        return;
    }
    
    // Sync regular enemies
    if (!enemies.empty()) {
        auto regularEnemyData = GetRegularEnemyDataForSync();
        
        if (!regularEnemyData.empty()) {
            std::string regularMsg = MessageHandler::FormatEnemyPositionsMessage(regularEnemyData);
            game->GetNetworkManager().BroadcastMessage(regularMsg);
        }
    }
    
    // Sync triangle enemies
    if (!triangleEnemies.empty()) {
        auto triangleEnemyData = GetTriangleEnemyDataForSync();
        
        if (!triangleEnemyData.empty()) {
            std::string triangleMsg = MessageHandler::FormatEnemyPositionsMessage(triangleEnemyData);
            game->GetNetworkManager().BroadcastMessage(triangleMsg);
        }
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
    }
}

void EnemyManager::UpdateTriangleEnemyPositions(const std::vector<std::tuple<int, sf::Vector2f, int>>& positions) {
    for (const auto& enemyData : positions) {
        int id = std::get<0>(enemyData);
        sf::Vector2f position = std::get<1>(enemyData);
        int health = std::get<2>(enemyData);
        
        // Find the enemy
        TriangleEnemy* enemy = GetTriangleEnemy(id);
        if (enemy && enemy->IsAlive()) {
            // Update position
            enemy->UpdatePosition(position, true); // true = interpolate
            
            // Update health if it's significantly different
            if (std::abs(enemy->GetHealth() - health) > 5) {
                // Set health directly to avoid unnecessary visual effects
                if (health <= 0) {
                    enemy->TakeDamage(enemy->GetHealth()); // Kill it
                } else if (health < enemy->GetHealth()) {
                    enemy->TakeDamage(enemy->GetHealth() - health);
                }
                // Note: we can't increase health, so if server health is higher,
                // we'll just wait for it to sync naturally over time
            }
        } else if (!enemy && health > 0) {
            // Enemy doesn't exist locally but should - create it
            AddTriangleEnemy(id, position, health);
        }
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
    } else {
        // As client, apply the damage from the network message
        // This is our authoritative source of enemy health
        
        // Update health and visuals
        if (killed) {
            enemy->TakeDamage(enemy->GetHealth()); // Kill it
        } else {
            enemy->TakeDamage(damage);
        }
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
    CSteamID localSteamID = SteamUser()->GetSteamID();
    CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
    
    // Only host sends full enemy lists
    if (localSteamID != hostID) {
        return;
    }
    
    // Sync regular enemies
    std::vector<int> regularEnemyIds;
    for (const auto& entry : enemies) {
        if (entry.enemy && entry.enemy->IsAlive()) {
            regularEnemyIds.push_back(entry.GetID());
        }
    }
    
    if (!regularEnemyIds.empty()) {
        std::string regularMsg = MessageHandler::FormatEnemyFullListMessage(
            regularEnemyIds, ParsedMessage::EnemyType::Regular);
        game->GetNetworkManager().BroadcastMessage(regularMsg);
    }
    
    // Sync triangle enemies
    std::vector<int> triangleEnemyIds;
    for (const auto& enemy : triangleEnemies) {
        if (enemy.IsAlive()) {
            triangleEnemyIds.push_back(enemy.GetID());
        }
    }
    
    if (!triangleEnemyIds.empty()) {
        std::string triangleMsg = MessageHandler::FormatEnemyFullListMessage(
            triangleEnemyIds, ParsedMessage::EnemyType::Triangle);
        game->GetNetworkManager().BroadcastMessage(triangleMsg);
    }
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

void EnemyManager::UpdateEnemyPositions(const std::vector<std::tuple<int, sf::Vector2f, int>>& positions) {
    // Process each position update
    for (const auto& posData : positions) {
        int id = std::get<0>(posData);
        sf::Vector2f pos = std::get<1>(posData);
        int health = std::get<2>(posData);
        
        // Determine if this is a triangle or regular enemy
        if (id >= 10000) { // Triangle IDs start at 10000
            // Update triangle enemy
            TriangleEnemy* enemy = GetTriangleEnemy(id);
            if (enemy && enemy->IsAlive()) {
                // Update position with interpolation
                enemy->UpdatePosition(pos, true); // true = use interpolation
                
                // Update health if it's significantly different
                int currentHealth = enemy->GetHealth();
                if (std::abs(currentHealth - health) > 5) {
                    // Set health directly
                    if (health <= 0) {
                        enemy->TakeDamage(enemy->GetHealth()); // Kill it
                    } else if (health < currentHealth) {
                        enemy->TakeDamage(currentHealth - health);
                    }
                }
            } else if (!enemy && health > 0) {
                // Enemy doesn't exist locally but should - create it
                AddTriangleEnemy(id, pos, health);
            }
        } else {
            // Update regular enemy
            auto it = enemyIdToIndex.find(id);
            if (it != enemyIdToIndex.end() && it->second < enemies.size() && enemies[it->second].enemy) {
                auto& entry = enemies[it->second];
                
                // Get the correct shape based on type
                if (entry.type == EnemyType::Rectangle) {
                    Enemy* rectEnemy = static_cast<Enemy*>(entry.enemy.get());
                    rectEnemy->GetShape().setPosition(pos);
                } else if (entry.type == EnemyType::Triangle) {
                    TriangleEnemy* triEnemy = static_cast<TriangleEnemy*>(entry.enemy.get());
                    triEnemy->UpdatePosition(pos, true); // true = interpolate
                }
                
                // Update health if it's significantly different
                int currentHealth = entry.enemy->GetHealth();
                if (std::abs(currentHealth - health) > 5) {
                    // Set health directly
                    if (health <= 0) {
                        entry.enemy->TakeDamage(entry.enemy->GetHealth()); // Kill it
                    } else if (health < currentHealth) {
                        entry.enemy->TakeDamage(currentHealth - health);
                    } else if (health > currentHealth) {
                        entry.enemy->SetHealth(health);
                    }
                }
            } else if (health > 0) {
                // Enemy doesn't exist locally but should - create it
                AddEnemy(id, pos, EnemyType::Rectangle, health);
            }
        }
    }
}

void EnemyManager::UpdateEnemyHealth(int id, int health) {
    auto it = enemyIdToIndex.find(id);
    if (it != enemyIdToIndex.end() && it->second < enemies.size() && enemies[it->second].enemy) {
        enemies[it->second].enemy->SetHealth(health);
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