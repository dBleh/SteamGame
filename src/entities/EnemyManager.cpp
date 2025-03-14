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
    
    // Remove dead enemies from the vector
    enemies.erase(std::remove_if(enemies.begin(), enemies.end(), 
        [](const Enemy& e) { return !e.IsAlive(); }), enemies.end());
    
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
    currentWave++;
    std::cout << "Starting wave " << currentWave << std::endl;
    waveActive = true;
    SpawnWave();
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
            std::cout << "Enemy ID " << id << " already exists, not adding duplicate" << std::endl;
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
    for (auto it = enemies.begin(); it != enemies.end(); ++it) {
        if (it->GetID() == id) {
            enemies.erase(it);
            return;
        }
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
    for (const auto& bullet : bullets) {
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
                    
                    // If enemy died, check if a player killed it
                    if (killed) {
                        std::cout << "Enemy " << enemy.GetID() << " killed by bullet from " 
                                  << bullet.GetShooterID() << std::endl;
                    }
                    
                    // Break since this bullet has collided
                    break;
                }
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
    
    // Enemy not found - should be spawned by host
    std::cout << "Received hit for unknown enemy ID: " << enemyId << std::endl;
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