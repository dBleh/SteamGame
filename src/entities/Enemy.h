#ifndef ENEMY_H
#define ENEMY_H

#include "EnemyBase.h"
#include "../utils/config.h"
#include <SFML/Graphics.hpp>
#include <string>
#include <cmath>

class Enemy : public EnemyBase {
public:
    // Use config.h constants for default values
    Enemy(int id, const sf::Vector2f& position, float speed = ENEMY_SPEED, int health = TRIANGLE_HEALTH);
    
    // Implementation of base class virtual methods
    void Update(float dt, const sf::Vector2f& targetPosition) override;
    bool CheckCollision(const sf::RectangleShape& playerShape) const override;
    void UpdateVisuals() override;
    std::string Serialize() const override;
    bool CheckBulletCollision(const sf::Vector2f& bulletPos, float bulletRadius);
    // Static deserialize method
    static Enemy Deserialize(const std::string& data);
    
    // Additional methods specific to rectangular enemy
    sf::RectangleShape& GetShape() { return shape; }
    const sf::RectangleShape& GetShape() const { return shape; }
    
    // Add position update method with interpolation
    void UpdatePosition(const sf::Vector2f& newPosition, bool interpolate = true);
    void SetTargetPosition(const sf::Vector2f& target);
    void UpdateInterpolation(float dt);
    
    // Accessor methods
    sf::Vector2f GetTargetPosition() const { return m_targetPosition; }
    bool HasTargetPosition() const { return m_hasTargetPosition; }
private:
    sf::RectangleShape shape;   // Visual representation
    sf::Vector2f m_targetPosition;       // Position to interpolate toward
    sf::Vector2f m_currentPosition;      // Starting position for interpolation
    float m_interpolationFactor;         // 0.0 to 1.0 interpolation progress
    bool m_hasTargetPosition;   
    // Constants for this enemy type
};

#endif // ENEMY_H