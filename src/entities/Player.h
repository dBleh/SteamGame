#ifndef PLAYER_H
#define PLAYER_H

#include <SFML/Graphics.hpp>
#include <iostream>
#include "../utils/input/InputHandler.h"
#include "../utils/input/InputManager.h" 
#include "../utils/config/Config.h"

class Player {
public:
    static constexpr float SHOOT_COOLDOWN_DURATION = 0.1f; // seconds between shots
    struct BulletParams {
        sf::Vector2f position;
        sf::Vector2f direction;
        bool success = false; // Added flag to indicate if shot was successful
    };
    Player();
    Player(const sf::Vector2f& startPosition, const sf::Color& color);
    void SetHealth(int newHealth) {
        health = newHealth;
        isDead = (health <= 0);
    }
    void Update(float dt, const InputManager& inputManager);
    BulletParams Shoot(const sf::Vector2f& mouseWorldPos);
    void Update(float dt);
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
    float GetShootCooldown() const;
    float GetBulletSpeedMultiplier() const { return bulletSpeedMultiplier; }
    void SetBulletSpeedMultiplier(float multiplier) { bulletSpeedMultiplier = multiplier; }
    
    float GetMoveSpeedMultiplier() const { return moveSpeedMultiplier; }
    void SetMoveSpeedMultiplier(float multiplier) { moveSpeedMultiplier = multiplier; }
    
    float GetMaxHealth() const { return maxHealth; }
    void SetMaxHealth(float newMaxHealth) { maxHealth = newMaxHealth; }
    
    // Also make sure there's a SetHealth method to update the player's health directly
    void SetHealth(float newHealth) { health = std::min(newHealth, maxHealth); }
private:
    float bulletSpeedMultiplier = 1.0f;
    float moveSpeedMultiplier = 1.0f;
    float maxHealth = PLAYER_HEALTH;
    sf::RectangleShape shape;
    float movementSpeed;
    float shootCooldown;
    int health;
    bool isDead;
    sf::Vector2f respawnPosition;
    
};

#endif // PLAYER_H