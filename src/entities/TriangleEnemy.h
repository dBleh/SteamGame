#ifndef TRIANGLE_ENEMY_H
#define TRIANGLE_ENEMY_H

#include "EnemyBase.h"
#include "../utils/config.h"
#include <SFML/Graphics.hpp>
#include <string>

// Triangle enemy implementation optimized for network performance
class TriangleEnemy : public EnemyBase {
public:
    // Use config.h constants for default values
    TriangleEnemy(int id, const sf::Vector2f& position, float speed = ENEMY_SPEED * 1.2f, int health = TRIANGLE_HEALTH);
    
    // Implementation of base class virtual methods
    void Update(float dt, const sf::Vector2f& targetPosition) override;
    bool CheckCollision(const sf::RectangleShape& playerShape) const override;
    void UpdateVisuals() override;
    std::string Serialize() const override;
    
    // Specialized collision detection with bullet
    bool CheckBulletCollision(const sf::Vector2f& bulletPos, float bulletRadius);
    
    // Static deserialize method
    static TriangleEnemy Deserialize(const std::string& data);
    
    // Additional methods specific to triangle enemy
    const sf::ConvexShape& GetShape() const { return shape; }
    sf::ConvexShape& GetShape() { return shape; }
    
    // Direction vector for movement optimization
    sf::Vector2f GetDirection() const { return direction; }
    
    // Optimized delta position for network sync
    sf::Vector2f GetPositionDelta(const sf::Vector2f& lastSyncedPosition) const;
    
    // Update position from network data
    void UpdatePosition(const sf::Vector2f& newPosition, bool interpolate = true);
    
    // Damage dealt by this enemy
    static constexpr int GetDamage() { return TRIANGLE_DAMAGE; }
    void SetTargetPosition(const sf::Vector2f& target);
    void UpdateInterpolation(float dt);
    
    // Accessor methods
    sf::Vector2f GetTargetPosition() const { return m_targetPosition; }
    bool HasTargetPosition() const { return m_hasTargetPosition; }
    // Money reward for killing this enemy
    static constexpr int GetKillReward() { return TRIANGLE_KILL_REWARD; }
    sf::Vector2f GetLastPosition() const { return lastPosition; }
    void SetLastPosition(const sf::Vector2f& pos) { lastPosition = pos; }
private:
    sf::ConvexShape shape; // Triangle shape
    sf::Vector2f direction; // Cached direction vector
    sf::Vector2f lastPosition; // For delta calculations
    sf::Vector2f m_targetPosition;
    sf::Vector2f m_currentPosition;
    float m_interpolationFactor;
    bool m_hasTargetPosition;
    
    // Constants for this enemy type
    
    // Initialize triangle shape
    void InitializeShape(const sf::Vector2f& position);
};

#endif // TRIANGLE_ENEMY_H