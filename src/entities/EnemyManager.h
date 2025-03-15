#ifndef ENEMY_MANAGER_H
#define ENEMY_MANAGER_H

#include "EnemyBase.h"
#include "Enemy.h"
#include "TriangleEnemy.h"
#include "EnemyTypes.h"
#include "Bullet.h"
#include "../utils/SpatialGrid.h"
#include "../utils/config.h"
#include "../utils/MessageHandler.h"  // Add this include for ParsedMessage

#include <SFML/Graphics.hpp>
#include <vector>
#include <memory>
#include <unordered_map>
#include <random>
#include <chrono>
#include <functional>
#include <unordered_set>
#include <thread>
class Game;
class PlayerManager;

// Define an entry structure to hold enemy data with type information
struct EnemyEntry {
    std::unique_ptr<EnemyBase> enemy;
    EnemyType type;
    sf::Vector2f lastSyncedPosition;
    
    bool IsAlive() const {
        return enemy && enemy->IsAlive();
    }
    
    int GetID() const {
        return enemy ? enemy->GetID() : -1;
    }
    
    sf::Vector2f GetPosition() const {
        return enemy ? enemy->GetPosition() : sf::Vector2f(0, 0);
    }
    
    int GetHealth() const {
        return enemy ? enemy->GetHealth() : 0;
    }
};

class EnemyManager {
public:
    EnemyManager(Game* game, PlayerManager* playerManager);
    ~EnemyManager();
    
    void Update(float dt);
    void Render(sf::RenderWindow& window);
    
    // Wave management
    void StartNextWave();
    int GetCurrentWave() const { return currentWave; }
    int GetRemainingEnemies() const;
    bool IsWaveComplete() const { return !waveActive && waveTimer > 0; }
    float GetWaveTimer() const { return waveTimer; }
    
    // Spawning methods
    void SpawnWave(int enemyCount, const std::vector<EnemyType>& types = {});
    void SpawnEnemyBatch(int count);
    void SpawnTriangleWave(int count = 50, uint32_t seed = 0);
    
    // Enemy management
    void AddEnemy(int id, const sf::Vector2f& position, EnemyType type = EnemyType::Rectangle, int health = TRIANGLE_HEALTH);
    void RemoveEnemy(int id);
    
    // Triangle-specific methods (for transition compatibility)
    void AddTriangleEnemy(int id, const sf::Vector2f& position);
    void AddTriangleEnemy(int id, const sf::Vector2f& position, int health);
    TriangleEnemy* GetTriangleEnemy(int id);
    
    // Collision detection
    void CheckBulletCollisions(const std::vector<Bullet>& bullets);
    void CheckPlayerCollisions();
    
    // Network synchronization
    void SyncEnemyPositions();
    void SyncFullEnemyList();
    void ValidateEnemyList(const std::vector<int>& validIds);
    void ValidateTriangleEnemyList(const std::vector<int>& validIds);
    void UpdateEnemyPositions(const std::vector<std::tuple<int, sf::Vector2f, int>>& positions);
    void UpdateTriangleEnemyPositions(const std::vector<std::tuple<int, sf::Vector2f, int>>& positions);
    
    // Enemy data access
    std::vector<int> GetAllEnemyIds() const;
    std::vector<int> GetAllTriangleEnemyIds() const;
    bool HasEnemy(int id) const;
    bool HasTriangleEnemy(int id) const;
    int GetEnemyHealth(int id) const;
    int GetTriangleEnemyHealth(int id) const;
    EnemyType GetEnemyType(int id) const;
    void ClearAllEnemies();
    // Health management
    void UpdateEnemyHealth(int id, int health);
    void UpdateTriangleEnemyHealth(int id, int health);
    
    // Hit handling with enemy type support
    void HandleEnemyHit(int enemyId, int damage, bool killed, const std::string& shooterID, ParsedMessage::EnemyType enemyType);
    void HandleTriangleEnemyHit(int enemyId, int damage, bool killed, const std::string& shooterID = "");
    
    // Serialization for save/load
    std::string SerializeEnemies() const;
    void DeserializeEnemies(const std::string& data);
    
    // Data retrieval for network sync
    std::vector<std::tuple<int, sf::Vector2f, int>> GetRegularEnemyDataForSync() const;
    std::vector<std::tuple<int, sf::Vector2f, int>> GetTriangleEnemyDataForSync() const;
    std::vector<std::tuple<int, sf::Vector2f, int, int>> GetEnemyDataForSync() const;
    sf::Vector2f GetEnemyPosition(int id) const {
        auto it = enemyIdToIndex.find(id);
        if (it != enemyIdToIndex.end() && it->second < enemies.size() && enemies[it->second].enemy) {
            return enemies[it->second].GetPosition();
        }
        return sf::Vector2f(0.f, 0.f);
    }
    
    sf::Vector2f GetTriangleEnemyPosition(int id) const {
        for (const auto& enemy : triangleEnemies) {
            if (enemy.GetID() == id) {
                return enemy.GetPosition();
            }
        }
        return sf::Vector2f(0.f, 0.f);
    }
private:
    Game* game;
    PlayerManager* playerManager;
    std::vector<EnemyEntry> enemies;  // Unified container for all enemy types
    std::vector<TriangleEnemy> triangleEnemies; // Keep separate container during transition
    
    // ID mapping for faster lookup
    std::unordered_map<int, size_t> enemyIdToIndex;
    
    // Wave management
    int currentWave;
    float waveTimer;
    float waveCooldown = 3.0f;
    bool waveActive;
    int nextEnemyId;
    int triangleNextEnemyId;  // Start at 10000 to avoid conflicts
    
    // Gradual spawning
    bool isSpawningWave = false;
    int remainingEnemiestoSpawn = 0;
    float spawnTimer = 0.0f;
    std::vector<EnemyType> spawnTypes;
    
    // Synchronization timers
    float enemySyncTimer = 0.0f;
    static constexpr float ENEMY_SYNC_INTERVAL = 0.1f;
    float fullSyncTimer = 0.0f;
    static constexpr float FULL_SYNC_INTERVAL = 5.0f;
    
    // Random number generation
    std::mt19937 rng;
    
    // Spatial partitioning for collision optimization
    SpatialGrid spatialGrid;
    
    // Helper methods
    void SpawnRegularWave();
    sf::Vector2f GetRandomSpawnPosition();
    sf::Vector2f GetPlayerCenterPosition();
    sf::Vector2f FindClosestPlayerPosition(const sf::Vector2f& enemyPos);
    void UpdateSpatialGrid();
    std::unique_ptr<EnemyBase> CreateEnemy(int id, const sf::Vector2f& position, EnemyType type, int health = TRIANGLE_HEALTH);
    
    // Reward helper
    void RewardShooter(const std::string& shooterID, EnemyType enemyType);
};

#endif // ENEMY_MANAGER_H