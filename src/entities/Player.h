#ifndef PLAYER_H
#define PLAYER_H

#include <SFML/Graphics.hpp>
#include <iostream>

class Player {
public:
    static constexpr float SHOOT_COOLDOWN_DURATION = 0.5f; // seconds between shots
    
    struct BulletParams {
        sf::Vector2f position;
        sf::Vector2f direction;
    };
    
    Player();
    Player(const sf::Vector2f& startPosition, const sf::Color& color);
    
    void Update(float dt);
    BulletParams Shoot(const sf::Vector2f& mouseWorldPos);
    
    sf::Vector2f GetPosition() const;
    void SetPosition(const sf::Vector2f& pos);
    sf::RectangleShape& GetShape();
    const sf::RectangleShape& GetShape() const;
    
    void SetSpeed(float speed);
    float GetSpeed() const;
    
    void TakeDamage(int amount);
    bool IsDead() const;
    void Respawn();
    void SetRespawnPosition(const sf::Vector2f& position);
    
    // New getter for health
    int GetHealth() const { return health; }
    
private:
    sf::RectangleShape shape;
    float movementSpeed;
    float shootCooldown;
    int health;
    bool isDead;
    sf::Vector2f respawnPosition;
};

#endif // PLAYER_H