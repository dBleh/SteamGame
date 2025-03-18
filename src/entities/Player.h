#ifndef PLAYER_H
#define PLAYER_H

#include <SFML/Graphics.hpp>
#include <iostream>
#include "../utils/input/InputHandler.h"
#include "../utils/input/InputManager.h" 
#include "ForceField.h"
#include "../utils/config/Config.h"
class ForceField;
class Player {
public:
    static constexpr float SHOOT_COOLDOWN_DURATION = 0.1f; // seconds between shots
    
    // Structure to hold bullet creation parameters
    struct BulletParams {
        sf::Vector2f position;
        sf::Vector2f direction;
        bool success = false; // Flag to indicate if shot was successful
    };
    
    // Constructors
    Player();
    Player(const sf::Vector2f& startPosition, const sf::Color& color);
    Player(Player&& other) noexcept;
    Player& operator=(Player&& other) noexcept;
    
    // Delete copy constructor and assignment
    Player(const Player&) = delete;
    Player& operator=(const Player&) = delete;
    // Update methods
    void Update(float dt); // Base update for cooldowns only
    void Update(float dt, const InputManager& inputManager); // Full update with input handling
    
    // Combat methods
    BulletParams Shoot(const sf::Vector2f& mouseWorldPos);
    
    // New method for shooting based on mouse world position
    BulletParams AttemptShoot(const sf::Vector2f& mouseWorldPos);
    
    void TakeDamage(int amount);
    bool IsDead() const;
    void Respawn();
    
    // Position methods
    sf::Vector2f GetPosition() const;
    void SetPosition(const sf::Vector2f& pos);
    void SetRespawnPosition(const sf::Vector2f& position);
    sf::Vector2f GetRespawnPosition() const;
    
    // Shape access
    sf::RectangleShape& GetShape();
    const sf::RectangleShape& GetShape() const;
    
    // Movement properties
    void SetSpeed(float speed);
    float GetSpeed() const;
    float GetMoveSpeedMultiplier() const { return moveSpeedMultiplier; }
    void SetMoveSpeedMultiplier(float multiplier) { moveSpeedMultiplier = multiplier; }
    
    // Shooting properties
    float GetShootCooldown() const;
    float GetBulletSpeedMultiplier() const { return bulletSpeedMultiplier; }
    void SetBulletSpeedMultiplier(float multiplier) { bulletSpeedMultiplier = multiplier; }
    
    // Health related methods
    int GetHealth() const { return health; }
    void SetHealth(float newHealth);
    float GetMaxHealth() const { return maxHealth; }
    void SetMaxHealth(float newMaxHealth) { maxHealth = newMaxHealth; }
    void InitializeForceField();
    void EnableForceField(bool enable);
    bool HasForceField(){ return forceFieldEnabled;}
    ForceField* GetForceField() const { return forceField.get(); }
private:
    // Visual representation
    sf::RectangleShape shape;
    
    // Movement properties
    float movementSpeed;
    float moveSpeedMultiplier;
    
    // Combat properties
    float shootCooldown;
    float bulletSpeedMultiplier;
    
    // Health properties
    float health;
    float maxHealth;
    bool isDead;
    
    // Respawn property
    sf::Vector2f respawnPosition;

    std::unique_ptr<ForceField> forceField;
    bool forceFieldEnabled = false;
};

#endif // PLAYER_H