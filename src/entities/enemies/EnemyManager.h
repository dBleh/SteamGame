#ifndef ENEMY_MANAGER_H
#define ENEMY_MANAGER_H

#include <memory>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <chrono>
#include <SFML/Graphics.hpp>
#include "Enemy.h"
#include "../../utils/config/EnemyConfig.h"
#include "../../utils/config/GameplayConfig.h"
#include "../../utils/config/Config.h" // For MAX_PACKET_SIZE

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
    bool InflictDamage(int enemyId, float damage, const std::string& attackerID);
    bool HasEnemies() const { return !enemies.empty(); }
    size_t GetEnemyCount() const { return enemies.size(); }
    
    // Callback handlers
    void HandleEnemyDeath(int enemyId, const sf::Vector2f& position, const std::string& killerID);
    void HandleEnemyDamage(int enemyId, float amount, float actualDamage);
    void HandlePlayerCollision(int enemyId, const std::string& playerID);
    
    // Collision detection
    void CheckPlayerCollisions();
    bool CheckBulletCollision(const sf::Vector2f& bulletPos, float bulletRadius, int& outEnemyId);
    
    // Network synchronization
    void SyncEnemyPositions();
    void SyncFullState();
    void ApplyNetworkUpdate(int enemyId, const sf::Vector2f& position, float health);
    void RemoteAddEnemy(int enemyId, EnemyType type, const sf::Vector2f& position, float health);
    void RemoteRemoveEnemy(int enemyId);
    void HandleSyncFullState(bool forceSend = false);
    
    // Wave management
    void StartNewWave(int enemyCount, EnemyType type = EnemyType::Triangle);
    void UpdateSpawning(float dt);
    int GetCurrentWave() const { return currentWave; }
    void SetCurrentWave(int wave) { currentWave = wave; }
    bool IsWaveComplete() const;
    bool IsWaveSpawning() const { return remainingEnemiesInWave > 0; }
    
    // Performance optimization
    void OptimizeEnemyList();

    // Enemy access
    Enemy* FindEnemy(int id);
    void RemoveEnemiesNotInList(const std::vector<int>& validIds);
    const std::unordered_map<int, std::unique_ptr<Enemy>>& GetEnemies() const { return enemies; }
    
    // Debugging
    void PrintEnemyStats() const;
    
private:
    // Helper methods
    void InitializeEnemyCallbacks(Enemy* enemy);
    
    // Private member variables
    Game* game;
    PlayerManager* playerManager;
    std::unordered_map<int, std::unique_ptr<Enemy>> enemies;
    int nextEnemyId;
    
    // Sync timers
    float syncTimer;
    float fullSyncTimer;
    std::chrono::steady_clock::time_point lastFullSyncTime;
    
    // Wave management
    int currentWave;
    int enemyCount;
    int remainingEnemiesInWave = 0;
    float batchSpawnTimer = 0.0f;
    EnemyType currentWaveEnemyType = EnemyType::Triangle;
    std::vector<sf::Vector2f> playerPositionsCache;
    
    // Spawn and position helpers
    sf::Vector2f GetRandomSpawnPosition(const sf::Vector2f& targetPosition, float minDistance, float maxDistance);
    bool IsValidSpawnPosition(const sf::Vector2f& position);
    
    // Network sync helpers
    std::vector<int> GetEnemyUpdatePriorities();
    std::unordered_set<int> syncedEnemyIds;      // Tracks which enemies were recently synced
    std::unordered_set<int> recentlyAddedIds;    // Tracks enemies added since last full sync
    std::unordered_set<int> recentlyRemovedIds;  // Tracks enemies removed since last full sync
    
    // Sync constants - these affect network performance and sync quality
    static constexpr float POSITION_SYNC_INTERVAL = 0.05f;  // 50ms for position updates
    static constexpr float URGENT_SYNC_THRESHOLD = 0.25f;   // 250ms min between urgent syncs
};

#endif // ENEMY_MANAGER_H