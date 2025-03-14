#include "EnemyManager.h"
#include "../Game.h"
#include "../utils/MessageHandler.h"
#include <iostream>
#include <cmath>
#include <sstream>

EnemyManager::EnemyManager(Game* game, PlayerManager* playerManager)
    : game(game), 
      playerManager(playerManager),
      currentWave(0),
      waveTimer(3.0f),
      waveActive(false),
      nextEnemyId(0) {
    // Initialize random number generator with time-based seed
    rng.seed(static_cast<unsigned int>(std::chrono::system_clock::now().time_since_epoch().count()));
}

EnemyManager::~EnemyManager() {
}

void EnemyManager::Update(float dt) {
    if (!waveActive) {
        // If no wave is active, count down to next wave
        waveTimer -= dt;
        if (waveTimer <= 0.0f) {
            StartNextWave();
        }
        return;
    }
    
    // Update full sync timer
    fullSyncTimer -= dt;
    if (fullSyncTimer <= 0.0f) {
        SyncFullEnemyList();
        fullSyncTimer = FULL_SYNC_INTERVAL;
    }
    
    // Remove dead enemies from the vector more robustly
    size_t beforeSize = enemies.size();
    enemies.erase(std::remove_if(enemies.begin(), enemies.end(), 
        [](const Enemy& e) { return !e.IsAlive(); }), enemies.end());
    size_t afterSize = enemies.size();
    
    if (beforeSize != afterSize) {
        std::cout << "Removed " << (beforeSize - afterSize) << " dead enemies" << std::endl;
    }
    
    // Rest of the existing code remains the same...
    // Only update if we have enemies and a wave is active
    if (enemies.empty()) {
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
    
    // Update each enemy
    for (auto& enemy : enemies) {
        if (enemy.IsAlive()) {
            // Find closest player position
            sf::Vector2f targetPos = FindClosestPlayerPosition(enemy.GetPosition());
            enemy.Update(dt, targetPos);
        }
    }
    enemySyncTimer -= dt;
    if (enemySyncTimer <= 0.0f) {
        SyncEnemyPositions();
        enemySyncTimer = ENEMY_SYNC_INTERVAL;
    }
    // Check for collisions with players
    CheckPlayerCollisions();
}

void EnemyManager::Render(sf::RenderWindow& window) {
    for (auto& enemy : enemies) {
        if (enemy.IsAlive()) {
            window.draw(enemy.GetShape());
        }
    }
}

void EnemyManager::StartNextWave() {
    // Force clear all enemies before starting new wave
    if (!enemies.empty()) {
        std::cout << "[EM] Clearing " << enemies.size() << " enemies before starting new wave" << std::endl;
        enemies.clear();
    }
    
    currentWave++;
    std::cout << "Starting wave " << currentWave << std::endl;
    waveActive = true;
    SpawnWave();
    
    // Reset the full sync timer to trigger a sync soon after wave start
    fullSyncTimer = 1.0f;  // Sync after 1 second
}

void EnemyManager::SpawnWave() {
    // Clear existing enemies
    enemies.clear();
    
    // Number of enemies increases with each wave
    int numEnemies = 4 + currentWave;
    
    // Spawn enemies
    for (int i = 0; i < numEnemies; i++) {
        sf::Vector2f pos = GetRandomSpawnPosition();
        int enemyId = nextEnemyId++;
        enemies.emplace_back(enemyId, pos);
        
        // If we're the host, broadcast enemy spawn
        CSteamID localSteamID = SteamUser()->GetSteamID();
        CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
        
        if (localSteamID == hostID) {
            std::string msg = MessageHandler::FormatEnemySpawnMessage(enemyId, pos);
            game->GetNetworkManager().BroadcastMessage(msg);
        }
    }
}

sf::Vector2f EnemyManager::GetRandomSpawnPosition() {
    // Constants for spawn circle
    float spawnRadius = 300.0f;
    
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
    std::uniform_real_distribution<float> distDist(200.0f, spawnRadius);
    
    float angle = angleDist(rng);
    float distance = distDist(rng);
    
    // Calculate position on circle
    float x = centerPos.x + distance * std::cos(angle);
    float y = centerPos.y + distance * std::sin(angle);
    
    return sf::Vector2f(x, y);
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

int EnemyManager::GetCurrentWave() const {
    return currentWave;
}

int EnemyManager::GetRemainingEnemies() const {
    int count = 0;
    for (const auto& enemy : enemies) {
        if (enemy.IsAlive()) {
            count++;
        }
    }
    return count;
}

bool EnemyManager::IsWaveComplete() const {
    return !waveActive;
}

float EnemyManager::GetWaveTimer() const {
    return waveTimer;
}

void EnemyManager::AddEnemy(int id, const sf::Vector2f& position) {
    // Check if enemy with this ID already exists
    for (auto& enemy : enemies) {
        if (enemy.GetID() == id) {
            return;
        }
    }
    
    // Add new enemy
    enemies.emplace_back(id, position);
    waveActive = true;
    
    // Update nextEnemyId if necessary
    if (id >= nextEnemyId) {
        nextEnemyId = id + 1;
    }
}

void EnemyManager::RemoveEnemy(int id) {
    bool found = false;
    size_t index = 0;
    
    // Find the enemy by ID
    for (size_t i = 0; i < enemies.size(); ++i) {
        if (enemies[i].GetID() == id) {
            found = true;
            index = i;
            break;
        }
    }
    
    if (found) {
        std::cout << "[EM] Removing enemy " << id << " at position (" 
                  << enemies[index].GetPosition().x << "," 
                  << enemies[index].GetPosition().y << ")" << std::endl;
                  
        enemies.erase(enemies.begin() + index);
    } else {
        std::cout << "[EM] Attempted to remove non-existent enemy " << id << std::endl;
    }
}

void EnemyManager::SyncEnemyState(int id, const sf::Vector2f& position, int health, bool isDead) {
    for (auto& enemy : enemies) {
        if (enemy.GetID() == id) {
            // Update enemy state
            enemy.GetShape().setPosition(position);
            
            // If enemy is dead in sync data but alive locally, kill it
            if (isDead && enemy.IsAlive()) {
                enemy.TakeDamage(1000); // Ensure it dies
            }
            return;
        }
    }
    
    // If we get here, the enemy wasn't found - add it
    AddEnemy(id, position);
}

void EnemyManager::CheckBulletCollisions(const std::vector<Bullet>& bullets) {
    // Create a vector to track which bullets need to be removed
    std::vector<size_t> bulletsToRemove;
    
    // Check each bullet against each enemy
    for (size_t bulletIndex = 0; bulletIndex < bullets.size(); bulletIndex++) {
        const Bullet& bullet = bullets[bulletIndex];
        bool bulletHit = false;
        
        for (auto& enemy : enemies) {
            if (enemy.IsAlive()) {
                if (enemy.GetShape().getGlobalBounds().intersects(bullet.GetShape().getGlobalBounds())) {
                    // Enemy hit by bullet
                    bool killed = enemy.TakeDamage(20); // Each bullet does 20 damage
                    
                    // If we're the host, broadcast this hit
                    CSteamID localSteamID = SteamUser()->GetSteamID();
                    CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
                    
                    if (localSteamID == hostID) {
                        std::string msg = MessageHandler::FormatEnemyHitMessage(
                            enemy.GetID(), 20, killed, bullet.GetShooterID());
                        game->GetNetworkManager().BroadcastMessage(msg);
                    }
                    
                    // Mark this bullet for removal
                    bulletsToRemove.push_back(bulletIndex);
                    bulletHit = true;
                    break; // A bullet can only hit one enemy
                }
            }
        }
        
        if (bulletHit) {
            // No need to check this bullet against other enemies
            continue;
        }
    }
    
    // If we have bullets to remove, inform the PlayerManager
    if (!bulletsToRemove.empty()) {
        playerManager->RemoveBullets(bulletsToRemove);
    }
}
void EnemyManager::SyncEnemyPositions() {
    CSteamID localSteamID = SteamUser()->GetSteamID();
    CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
    
    // Only host syncs enemy positions
    if (localSteamID != hostID || enemies.empty()) {
        return;
    }
    
    std::vector<std::pair<int, sf::Vector2f>> enemyPositions;
    
    // Collect all alive enemies' positions
    for (const auto& enemy : enemies) {
        if (enemy.IsAlive()) {
            enemyPositions.emplace_back(enemy.GetID(), enemy.GetPosition());
        }
    }
    
    // If we have enemy positions to sync, broadcast them
    if (!enemyPositions.empty()) {
        std::string msg = MessageHandler::FormatEnemyPositionsMessage(enemyPositions);
        game->GetNetworkManager().BroadcastMessage(msg);
    }
}

void EnemyManager::UpdateEnemyPositions(const std::vector<std::pair<int, sf::Vector2f>>& positions) {
    for (const auto& enemyPos : positions) {
        int id = enemyPos.first;
        sf::Vector2f position = enemyPos.second;
        
        // Find enemy with this ID
        for (auto& enemy : enemies) {
            if (enemy.GetID() == id && enemy.IsAlive()) {
                // Update position
                enemy.GetShape().setPosition(position);
                break;
            }
        }
    }
}
void EnemyManager::CheckPlayerCollisions() {
    auto& players = playerManager->GetPlayers();
    
    for (auto& enemy : enemies) {
        if (!enemy.IsAlive()) continue;
        
        for (auto& playerPair : players) {
            std::string playerID = playerPair.first;
            RemotePlayer& rp = playerPair.second;
            
            // Skip dead players
            if (rp.player.IsDead()) continue;
            
            if (enemy.CheckCollision(rp.player.GetShape())) {
                // Enemy hit player - player takes damage and enemy dies
                rp.player.TakeDamage(20);
                
                // Kill the enemy
                enemy.TakeDamage(100); // Ensure it dies
                
                // If we're the host, broadcast this collision
                CSteamID localSteamID = SteamUser()->GetSteamID();
                CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
                
                if (localSteamID == hostID) {
                    // Send player damage message
                    std::string damageMsg = MessageHandler::FormatPlayerDamageMessage(
                        playerID, 20, enemy.GetID());
                    game->GetNetworkManager().BroadcastMessage(damageMsg);
                    
                    // Send enemy death message
                    std::string enemyDeathMsg = MessageHandler::FormatEnemyDeathMessage(
                        enemy.GetID(), "", false);  // No killer, no reward
                    game->GetNetworkManager().BroadcastMessage(enemyDeathMsg);
                }
                
                // If local player died, handle death
                if (playerID == std::to_string(SteamUser()->GetSteamID().ConvertToUint64()) && 
                    rp.player.IsDead()) {
                    // Set respawn timer
                    rp.respawnTimer = 3.0f;
                }
                
                // Break since this enemy has collided
                break;
            }
        }
    }
}

void EnemyManager::HandleEnemyHit(int enemyId, int damage, bool killed) {
    for (auto& enemy : enemies) {
        if (enemy.GetID() == enemyId) {
            // Apply damage to sync state
            if (!enemy.IsAlive()) {
                // Enemy already dead locally
                return;
            }
            
            // Apply damage
            bool diedLocally = enemy.TakeDamage(damage);
            
            // Handle discrepancy
            if (killed && !diedLocally) {
                // Force enemy to die if network says it should be dead
                enemy.TakeDamage(1000);
            }
            return;
        }
    }
}

std::string EnemyManager::SerializeEnemies() const {
    std::ostringstream oss;
    oss << enemies.size();
    
    for (const auto& enemy : enemies) {
        oss << ";" << enemy.Serialize();
    }
    
    return oss.str();
}

void EnemyManager::DeserializeEnemies(const std::string& data) {
    enemies.clear();
    
    std::istringstream iss(data);
    int enemyCount;
    char delimiter;
    
    iss >> enemyCount;
    
    for (int i = 0; i < enemyCount; i++) {
        iss >> delimiter; // Read semicolon
        std::string enemyData;
        std::getline(iss, enemyData, ';');
        
        if (!enemyData.empty()) {
            Enemy enemy = Enemy::Deserialize(enemyData);
            enemies.push_back(enemy);
            
            // Update nextEnemyId if necessary
            if (enemy.GetID() >= nextEnemyId) {
                nextEnemyId = enemy.GetID() + 1;
            }
        }
    }
    
    // If we have enemies, the wave is active
    waveActive = !enemies.empty();
}


std::vector<int> EnemyManager::GetAllEnemyIds() const {
    std::vector<int> ids;
    ids.reserve(enemies.size());
    for (const auto& enemy : enemies) {
        ids.push_back(enemy.GetID());
    }
    return ids;
}

bool EnemyManager::HasEnemy(int id) const {
    for (const auto& enemy : enemies) {
        if (enemy.GetID() == id) {
            return true;
        }
    }
    return false;
}

void EnemyManager::SyncFullEnemyList() {
    CSteamID localSteamID = SteamUser()->GetSteamID();
    CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
    
    // Only host sends full enemy list
    if (localSteamID != hostID || enemies.empty()) {
        return;
    }
    
    // Collect all enemy IDs in a format suitable for transmission
    std::vector<int> enemyIds;
    std::vector<std::pair<int, sf::Vector2f>> enemyData;
    
    for (const auto& enemy : enemies) {
        if (enemy.IsAlive()) {
            enemyIds.push_back(enemy.GetID());
            enemyData.emplace_back(enemy.GetID(), enemy.GetPosition());
        }
    }
    
    // Create and send a message with all active enemy IDs
    std::ostringstream oss;
    oss << "EV|" << enemyIds.size();
    for (int id : enemyIds) {
        oss << "|" << id;
    }
    game->GetNetworkManager().BroadcastMessage(oss.str());
    
    // Also send positions for these enemies
    std::string posMsg = MessageHandler::FormatEnemyPositionsMessage(enemyData);
    game->GetNetworkManager().BroadcastMessage(posMsg);
    
    std::cout << "[HOST] Sent full enemy validation with " << enemyIds.size() << " enemies" << std::endl;
}

void EnemyManager::ValidateEnemyList(const std::vector<int>& validIds) {
    // First, identify enemies that shouldn't exist
    std::vector<int> localIds = GetAllEnemyIds();
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
    
    // Remove enemies that shouldn't exist
    for (int id : toRemove) {
        std::cout << "[CLIENT] Removing ghost enemy: " << id << std::endl;
        RemoveEnemy(id);
    }
    
    // Now check for missing enemies that should exist
    for (int validId : validIds) {
        if (!HasEnemy(validId)) {
            // Request the enemy's data from the host
            std::cout << "[CLIENT] Requesting missing enemy: " << validId << std::endl;
            
            // This could be a new network message type, but for now
            // we'll just create a placeholder enemy at (0,0) that will
            // get its position updated in the next position sync
            AddEnemy(validId, sf::Vector2f(0.f, 0.f));
        }
    }
}

void EnemyManager::RemoveStaleEnemies(const std::vector<int>& validIds) {
    // Helper method to remove any enemies not in the valid list
    std::vector<int> toRemove;
    
    for (const auto& enemy : enemies) {
        int id = enemy.GetID();
        bool found = false;
        
        for (int validId : validIds) {
            if (id == validId) {
                found = true;
                break;
            }
        }
        
        if (!found) {
            toRemove.push_back(id);
        }
    }
    
    for (int id : toRemove) {
        RemoveEnemy(id);
        std::cout << "[SYNC] Removed stale enemy: " << id << std::endl;
    }
}