#ifndef ENEMY_MANAGER_H
#define ENEMY_MANAGER_H

#include "Enemy.h"
#include "../entities/PlayerManager.h"
#include "../entities/Bullet.h"
#include <vector>
#include <memory>
#include <random>
#include <chrono>

class Game;

class EnemyManager {
public:
    EnemyManager(Game* game, PlayerManager* playerManager);
    ~EnemyManager();
    
    void Update(float dt);
    void Render(sf::RenderWindow& window);
    std::vector<std::tuple<int, sf::Vector2f, int>> GetEnemyDataForSync() const;

    // Wave management
    void StartNextWave();
    int GetCurrentWave() const;
    int GetRemainingEnemies() const;
    bool IsWaveComplete() const;
    float GetWaveTimer() const;
    void SyncFullEnemyList();
    void ValidateEnemyList(const std::vector<int>& activeEnemyIds);

    // Network synchronization
    void AddEnemy(int id, const sf::Vector2f& position);
    void RemoveEnemy(int id);
    void SyncEnemyState(int id, const sf::Vector2f& position, int health, bool isDead);
    void HandleEnemyHit(int enemyId, int damage, bool killed);
    
    // Collision checking
    void CheckBulletCollisions(const std::vector<Bullet>& bullets);
    void CheckPlayerCollisions();
    void RemoveStaleEnemies(const std::vector<int>& validIds);
    std::vector<int> GetAllEnemyIds() const;
    bool HasEnemy(int id) const;

    // Serialization for network
    std::string SerializeEnemies() const;
    void DeserializeEnemies(const std::string& data);
    void SyncEnemyPositions();
    void UpdateEnemyPositions(const std::vector<std::pair<int, sf::Vector2f>>& positions);
    int GetEnemyHealth(int id) const;
    void UpdateEnemyHealth(int id, int health);
private:
    Game* game;
    PlayerManager* playerManager;
    std::vector<Enemy> enemies;
    
    int currentWave;
    float waveTimer;            // Timer between waves
    float waveCooldown = 2.0f;  // Seconds between waves
    bool waveActive;
    int nextEnemyId;
    float enemySyncTimer = 0.0f;
    static constexpr float ENEMY_SYNC_INTERVAL = 0.1f; // 10 times per second
    // Random number generation
    std::mt19937 rng;
    float fullSyncTimer = 0.0f;
    const float FULL_SYNC_INTERVAL = 5.0f;
    // Spawn enemies in a wave
    void SpawnWave();
    sf::Vector2f GetRandomSpawnPosition();
    sf::Vector2f FindClosestPlayerPosition(const sf::Vector2f& enemyPos);
};

#endif // ENEMY_MANAGER_H