#ifndef ENEMY_MANAGER_H
#define ENEMY_MANAGER_H

#include <memory>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <SFML/Graphics.hpp>
#include "Enemy.h"
#include "../../utils/config/Config.h"

// Forward declarations
class Game;
class PlayerManager;

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
    
    // Client-side prediction
    void UpdateEnemyClientPrediction(float dt);
    
    // Collision detection
    void CheckPlayerCollisions();
    void CheckBulletEnemyCollisions();
    bool CheckBulletCollision(const sf::Vector2f& bulletPos, float bulletRadius, int& outEnemyId);
    
    // Network synchronization
    void SyncEnemyPositions();
    void SyncFullState();
    void SyncFullStateForClient(CSteamID clientId);
    void HandleStateRequest(CSteamID requesterId);
    void ApplyNetworkUpdate(int enemyId, const sf::Vector2f& position, float health);
    void ApplyNetworkUpdateWithVelocity(int enemyId, const sf::Vector2f& position, 
                                      const sf::Vector2f& velocity, float health);
    void RemoteAddEnemy(int enemyId, EnemyType type, const sf::Vector2f& position, float health);
    void RemoteAddEnemyWithVelocity(int enemyId, EnemyType type, const sf::Vector2f& position, 
                                  const sf::Vector2f& velocity, float health);
    void RemoteRemoveEnemy(int enemyId);
    
    // Wave management
    void StartNewWave(int enemyCount, EnemyType type = EnemyType::Triangle);
    void UpdateSpawning(float dt);
    int GetCurrentWave() const { return currentWave; }
    void SetCurrentWave(int wave) { currentWave = wave; }
    bool IsWaveComplete() const;
    bool IsWaveSpawning() const { return remainingEnemiesInWave > 0; }
    
    // Performance optimization
    void OptimizeEnemyList();
    void UpdatePlayerPositionsCache();

    Enemy* FindEnemy(int id);
    void RemoveEnemiesNotInList(const std::vector<int>& validIds);

private:
    Game* game;
    PlayerManager* playerManager;
    std::unordered_map<int, std::unique_ptr<Enemy>> enemies;
    int nextEnemyId;
    float syncTimer;
    float fullSyncTimer;
    int currentWave;
    int remainingEnemiesInWave;
    float batchSpawnTimer;
    EnemyType currentWaveEnemyType;
    std::vector<sf::Vector2f> playerPositionsCache;
    int enemiesRemainingToSpawn;
    std::chrono::steady_clock::time_point lastEnemyStateUpdate;
    
    // Spawn and position helpers
    sf::Vector2f GetRandomSpawnPosition(const sf::Vector2f& targetPosition, float minDistance, float maxDistance);
    bool IsValidSpawnPosition(const sf::Vector2f& position);
    
    // Network sync helpers
    std::vector<int> GetEnemyUpdatePriorities();
};

#endif // ENEMY_MANAGER_H