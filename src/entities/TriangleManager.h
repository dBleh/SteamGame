#pragma once

#include "TriangleEnemy.h"
#include "SpatialGrid.h"
#include <SFML/Graphics.hpp>
#include <vector>
#include <unordered_map>
#include <tuple>
#include <memory>
#include <unordered_set>
// Forward declarations
class Game;
class PlayerManager;
class Bullet;

class TriangleEnemyManager {
public:
    TriangleEnemyManager(Game* game, PlayerManager* playerManager);
    ~TriangleEnemyManager();
    
    // Core update method
    void Update(float dt);
    
    // Render all visible enemies
    void Render(sf::RenderWindow& window);
    
    // Spawn a wave of enemies
    void SpawnWave(int enemyCount = 100);
    
    // Optimized collision detection with bullets
    void CheckBulletCollisions(const std::vector<Bullet>& bullets);
    
    // Check collisions with players
    void CheckPlayerCollisions();
    
    // Find closest player for enemy targeting
    sf::Vector2f FindClosestPlayerPosition(const sf::Vector2f& enemyPos);
    
    // Network synchronization methods
    void SyncEnemyPositions();
    std::vector<std::tuple<int, sf::Vector2f, int>> GetEnemyDataForSync() const;
    void SyncFullEnemyList();
    void ValidateEnemyList(const std::vector<int>& validIds);
    
    // Handle enemy hit from network message
    void HandleEnemyHit(int enemyId, int damage, bool killed);
    
    // Update enemy positions from network data
    void UpdateEnemyPositions(const std::vector<std::tuple<int, sf::Vector2f, int>>& enemyPositions);
    
    // Update enemy health from network data
    void UpdateEnemyHealth(int enemyId, int health);
    void AddEnemy(int id, const sf::Vector2f& position);
    
    // Overloaded version that also sets health
    void AddEnemy(int id, const sf::Vector2f& position, int health);
    
    // Method to get the next available enemy ID
    int GetNextEnemyId() const;
    // Get current enemy count
    size_t GetEnemyCount() const;
    
    // Get enemy by ID
    TriangleEnemy* GetEnemy(int enemyId);
    
private:
    Game* game;
    PlayerManager* playerManager;
    std::vector<std::unique_ptr<TriangleEnemy>> enemies;
    std::unordered_map<int, TriangleEnemy*> enemyMap; // Fast lookup by ID
    SpatialGrid spatialGrid;
    
    int nextEnemyId;
    float enemySyncTimer;
    float fullSyncTimer;
    
    // Network optimization: track last synced positions for delta compression
    std::unordered_map<int, sf::Vector2f> lastSyncedPositions;
    
    // Optimization: track visible enemies for rendering
    std::vector<TriangleEnemy*> visibleEnemies;
    
    // Variable update rates based on distance from players
    struct EnemyUpdateGroup {
        std::vector<TriangleEnemy*> enemies;
        float updateInterval;
        float timeSinceLastUpdate;
    };
    
    std::vector<EnemyUpdateGroup> updateGroups;
    
    // Constants for network synchronization
    static constexpr float ENEMY_SYNC_INTERVAL = 0.1f;
    static constexpr float FULL_SYNC_INTERVAL = 5.0f;
    
    // Constants for update optimization
    static constexpr float CLOSE_DISTANCE = 500.f;
    static constexpr float MEDIUM_DISTANCE = 1000.f;
    
    // Initialize update groups
    void InitializeUpdateGroups();
    
    // Helper method to add enemy to appropriate update group
    void AssignEnemyToUpdateGroup(TriangleEnemy* enemy);
    
    // Helper method to reassign all enemies to update groups
    void ReassignAllEnemies();
    
    // Helper method to remove an enemy
    void RemoveEnemy(int enemyId);
    
    // Helper method to update visible enemies list
    void UpdateVisibleEnemies(const sf::FloatRect& viewBounds);
    
    // Helper method to get delta-compressed position data
    std::vector<std::tuple<int, sf::Vector2f, int>> GetDeltaCompressedPositions() const;
};