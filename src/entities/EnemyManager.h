#ifndef ENEMY_MANAGER_H
#define ENEMY_MANAGER_H

#include "EnemyBase.h"
#include "Enemy.h"
#include "TriangleEnemy.h"
#include "EnemyTypes.h"
#include "Bullet.h"
#include "../utils/SpatialGrid.h"
#include "../utils/config.h"

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
class EnemyManager {
    public:
        EnemyManager(Game* game, PlayerManager* playerManager);
        ~EnemyManager();
        void ValidateTriangleEnemyList(const std::vector<int>& validIds);
        
        
        void Update(float dt);
        void Render(sf::RenderWindow& window);
    
        // Wave management
        void StartNextWave();
        int GetCurrentWave() const;
        int GetRemainingEnemies() const;
        bool IsWaveComplete() const;
        float GetWaveTimer() const;
        
        // Synchronization
        void SyncFullEnemyList();
        void ValidateEnemyList(const std::vector<int>& activeEnemyIds);
        
        // Regular enemies management
        void AddEnemy(int id, const sf::Vector2f& position);
        void RemoveEnemy(int id);
        void SyncEnemyState(int id, const sf::Vector2f& position, int health, bool isDead);
        void HandleEnemyHit(int enemyId, int damage, bool killed);
        std::vector<std::tuple<int, sf::Vector2f, int>> GetRegularEnemyDataForSync() const;
        
        // Triangle enemies management
        void SpawnTriangleWave(int count = 50, uint32_t seed = 0);
        void AddTriangleEnemy(int id, const sf::Vector2f& position);
        void AddTriangleEnemy(int id, const sf::Vector2f& position, int health);
        void HandleTriangleEnemyHit(int enemyId, int damage, bool killed);
        std::vector<std::tuple<int, sf::Vector2f, int>> GetTriangleEnemyDataForSync() const;
        TriangleEnemy* GetTriangleEnemy(int id);
        
        // Common functionality
        void CheckBulletCollisions(const std::vector<Bullet>& bullets);
        void CheckPlayerCollisions();
        void RemoveStaleEnemies(const std::vector<int>& validIds);
        
        // Regular enemy specific queries
        std::vector<int> GetAllEnemyIds() const;
        bool HasEnemy(int id) const;
        std::string SerializeEnemies() const;
        void DeserializeEnemies(const std::string& data);
        void SyncEnemyPositions();
        void UpdateEnemyPositions(const std::vector<std::pair<int, sf::Vector2f>>& positions);
        int GetEnemyHealth(int id) const;
        void UpdateEnemyHealth(int id, int health);
        
        // Triangle enemy specific queries
        std::vector<int> GetAllTriangleEnemyIds() const;
        bool HasTriangleEnemy(int id) const;
        void UpdateTriangleEnemyPositions(const std::vector<std::tuple<int, sf::Vector2f, int>>& positions);
        int GetTriangleEnemyHealth(int id) const;
        void UpdateTriangleEnemyHealth(int id, int health);
        void GenerateEnemiesWithSeed(uint32_t seed, int count);
        
    private:
        Game* game;
        PlayerManager* playerManager;
        std::vector<Enemy> enemies;            // Regular enemies
        std::vector<TriangleEnemy> triangleEnemies; // Triangle enemies
        
        int currentWave;
        float waveTimer;            // Timer between waves
        float waveCooldown = 2.0f;  // Seconds between waves
        bool waveActive;
        int nextEnemyId;            // For regular enemies
        int triangleNextEnemyId;    // For triangle enemies
        
        float enemySyncTimer = 0.0f;
        static constexpr float ENEMY_SYNC_INTERVAL = 0.1f; // 10 times per second
        
        // Random number generation
        std::mt19937 rng;
        float fullSyncTimer = 0.0f;
        const float FULL_SYNC_INTERVAL = 5.0f;
        
        // Spawn methods
        void SpawnRegularWave();
        sf::Vector2f GetRandomSpawnPosition();
        sf::Vector2f GetPlayerCenterPosition();
        sf::Vector2f FindClosestPlayerPosition(const sf::Vector2f& enemyPos);
    };
    
    #endif // ENEMY_MANAGER_H