#ifndef BULLET_H
#define BULLET_H

#include <SFML/Graphics.hpp>
#include "../../utils/config/BulletConfig.h"  // Using specific config instead of general Config.h
#include <iostream>

class Bullet {
public:
    Bullet(const sf::Vector2f& position, const sf::Vector2f& direction, float speed, const std::string& shooterID);
    
    void Update(float dt);
    sf::RectangleShape& GetShape();
    const sf::RectangleShape& GetShape() const;
    sf::Vector2f GetPosition() const { return shape.getPosition(); }
    bool IsExpired() const { return lifetime <= 0.f; }
    std::string GetShooterID() const { return shooterID; }
    bool CheckCollision(const sf::RectangleShape& playerShape, const std::string& playerID) const;
    bool BelongsToPlayer(const std::string& playerID) const;
    float lifetime;  
    sf::Vector2f velocity;  
private:
    sf::RectangleShape shape;  // Visual representation (small rectangle)
    std::string shooterID;     // ID of the player who shot this bullet
};

#endif // BULLET_H