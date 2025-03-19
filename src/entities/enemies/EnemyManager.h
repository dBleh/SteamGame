#ifndef ENEMY_MANAGER_H
#define ENEMY_MANAGER_H

#include <memory>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <SFML/Graphics.hpp>
#include "Enemy.h"
#include "TriangleEnemy.h"
#include "../../utils/config/Config.h"
#include "../../states/GameSettingsManager.h" 

// Constants for enemy handling
constexpr float POSITION_INTERPOLATION_SPEED = 5.0f; // Speed to interpolate between positions
constexpr float POSITION_SYNC_INTERVAL = 0.1f;      // Position sync interval
constexpr float VALIDATION_CHECK_INTERVAL = 5.0f;   // Time between enemy validation checks

// Forward declarations
class Game;
class PlayerManager;
class TriangleEnemy;

// Structure to track enemy network state
struct EnemyNetworkState {
    sf::Vector2f currentPosition;
    sf::Vector2f targetPosition;
    sf::Vector2f currentVelocity;
    sf::Vector2f targetVelocity;
    float interpolationTime = 0.0f;
    bool needsInterpolation = false;
    std::chrono::steady_clock::time_point lastUpdateTime;
    
    EnemyNetworkState() : 
        currentPosition(0.0f, 0.0f),
        targetPosition(0.0f, 0.0f),
        currentVelocity(0.0f, 0.0f),
        targetVelocity(0.0f, 0.0f),
        lastUpdateTime(std::chrono::steady_clock::now()) {}
};

class EnemyManager {
public:
    explicit EnemyManager(Game* game, PlayerManager* playerManager);
    ~EnemyManager() = default;

    // Core functionality
    void Update(float dt);
    void Render(sf::RenderWindow& window);
    
    // Enemy management
    int AddEnemy(EnemyType type, const sf::Vector2f& position, float health = ENEMY_HEALTH);
    void RemoveEnemy(int id);
    void ClearEnemies();
    bool InflictDamage(int enemyId, float damage);
    bool HasEnemies() const { return !enemies.empty(); }
    size_t GetEnemyCount() const { return enemies.size(); }
    std::unique_ptr<Enemy> CreateEnemy(EnemyType type, int id, const sf::Vector2f& position);
    
    // Collision detection
    void CheckPlayerCollisions();
    bool CheckBulletCollision(const sf::Vector2f& bulletPos, float bulletRadius, int& outEnemyId);
    
    // Network synchronization
    void SyncEnemyPositions();
    void SyncFullState();
    void ApplyNetworkUpdate(int enemyId, const sf::Vector2f& position, float health);
    void ApplyNetworkUpdate(int enemyId, const sf::Vector2f& position, const sf::Vector2f& velocity, float health);
    void RemoteAddEnemy(int enemyId, EnemyType type, const sf::Vector2f& position, float health);
    void RemoteRemoveEnemy(int enemyId);
    
    // Wave management
    void StartNewWave(int enemyCount, EnemyType type = EnemyType::Triangle);
    void UpdateSpawning(float dt);
    int GetCurrentWave() const { return currentWave; }
    void SetCurrentWave(int wave) { currentWave = wave; }
    bool IsWaveComplete() const;
    bool IsWaveSpawning() const { return remainingEnemiesInWave > 0; }
    
    // Position interpolation and handling
    void UpdateEnemyInterpolation(float dt);
    void SetEnemyTargetPosition(int enemyId, const sf::Vector2f& position, const sf::Vector2f& velocity);
    void InterpolateEnemyPosition(int enemyId, float dt);
    
    // Performance optimization
    void OptimizeEnemyList();
    void ValidateEnemyStates();

    Enemy* FindEnemy(int id);
    void RemoveEnemiesNotInList(const std::vector<int>& validIds);
    
    // Apply settings from GameSettingsManager to all enemies
    void ApplySettings();
    
private:
    Game* game;
    PlayerManager* playerManager;
    std::unordered_map<int, std::unique_ptr<Enemy>> enemies;
    std::unordered_map<int, EnemyNetworkState> enemyNetworkStates;
    
    int nextEnemyId;
    float syncTimer;
    float fullSyncTimer;
    float validationTimer;
    int currentWave;
    int remainingEnemiesInWave = 0;
    float batchSpawnTimer = 0.0f;
    float spawnBatchInterval = ENEMY_SPAWN_BATCH_INTERVAL;
    int spawnBatchSize = ENEMY_SPAWN_BATCH_SIZE;
    float enemyCullingDistance = ENEMY_CULLING_DISTANCE;
    float triangleMinSpawnDistance = TRIANGLE_MIN_SPAWN_DISTANCE;
    float triangleMaxSpawnDistance = TRIANGLE_MAX_SPAWN_DISTANCE;
    EnemyType currentWaveEnemyType = EnemyType::Triangle;
    std::vector<sf::Vector2f> playerPositionsCache;
    
    // Spawn and position helpers
    sf::Vector2f GetRandomSpawnPosition(const sf::Vector2f& targetPosition, float minDistance, float maxDistance);
    bool IsValidSpawnPosition(const sf::Vector2f& position);
    
    // Network sync helpers
    std::vector<int> GetEnemyUpdatePriorities();
    bool IsLocalPlayerHost() const;
    void LogPositionUpdate(int enemyId, const sf::Vector2f& oldPos, const sf::Vector2f& newPos, const char* source);
};

#endif // ENEMY_MANAGER_H