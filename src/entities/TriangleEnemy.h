#pragma once

#include <SFML/Graphics.hpp>
#include <string>
#include <unordered_set>
// Lightweight triangle enemy implementation optimized for network performance
class TriangleEnemy {
public:
    TriangleEnemy(int id, const sf::Vector2f& position, float speed = 60.f, int health = 40);
    
    // Core update method - minimized to essential operations
    void Update(float dt, const sf::Vector2f& targetPosition);
    
    // Network optimization - returns true if enemy was killed
    bool TakeDamage(int amount);
    
    // Collision detection with player
    bool CheckCollision(const sf::RectangleShape& playerShape);
    
    // Collision detection with bullet
    bool CheckBulletCollision(const sf::Vector2f& bulletPos, float bulletRadius);
    
    // Basic accessors
    const sf::ConvexShape& GetShape() const { return shape; }
    sf::Vector2f GetPosition() const { return shape.getPosition(); }
    bool IsAlive() const { return !isDead; }
    int GetID() const { return id; }
    int GetHealth() const { return health; }
    
    // Direction vector for movement optimization
    sf::Vector2f GetDirection() const { return direction; }
    
    // Network serialization - compact format
    std::string Serialize() const;
    static TriangleEnemy Deserialize(const std::string& data);
    
    // Optimized delta position for network sync
    sf::Vector2f GetPositionDelta(const sf::Vector2f& lastSyncedPosition) const;
    
    // Update position from network data
    void UpdatePosition(const sf::Vector2f& newPosition, bool interpolate = true);
    
private:
    int id;
    sf::ConvexShape shape; // Triangle shape
    float movementSpeed;
    int health;
    bool isDead;
    sf::Vector2f direction; // Cached direction vector
    sf::Vector2f lastPosition; // For delta calculations
    
    // Visual updates based on health
    void UpdateVisuals();
    
    // Initialize triangle shape
    void InitializeShape(const sf::Vector2f& position);
};