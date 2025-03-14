#ifndef BULLET_H
#define BULLET_H

#include <SFML/Graphics.hpp>
#include <iostream>

class Bullet {
public:
    Bullet(const sf::Vector2f& position, const sf::Vector2f& direction, float speed, const std::string& shooterID, const std::string& id);
    
    void Update(float dt);
    sf::RectangleShape& GetShape();
    const sf::RectangleShape& GetShape() const;
    sf::Vector2f GetPosition() const { return shape.getPosition(); }
    bool IsExpired() const { return lifetime <= 0.f; }
    std::string GetShooterID() const { return shooterID; }bool CheckCollision(const sf::RectangleShape& playerShape, const std::string& playerID) const;
    bool BelongsToPlayer(const std::string& playerID) const;
    std::string GetBulletID() const { return bulletID; }
private:
    sf::RectangleShape shape;  // Visual representation (small rectangle)
    sf::Vector2f velocity;     // Direction * speed
    float lifetime;            // Seconds until expiration (5s)
    std::string shooterID;     // ID of the player who shot this bullet
    std::string bulletID; 
};

#endif // BULLET_H