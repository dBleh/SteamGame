#ifndef PLAYER_H
#define PLAYER_H

#include <SFML/Graphics.hpp>
#include <iostream>
#include <functional>
#include "../../utils/input/InputHandler.h"
#include "../../utils/input/InputManager.h" 
#include "ForceField.h"
#include "../../utils/config/Config.h"

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
    
    // Event callback types
    using DeathCallback = std::function<void(const std::string&, const sf::Vector2f&, const std::string&)>;
    using RespawnCallback = std::function<void(const std::string&, const sf::Vector2f&)>;
    using DamageCallback = std::function<void(const std::string&, int, int)>;
    
    // Constructors
    Player();
    Player(const sf::Vector2f& startPosition, const sf::Color& color);
    Player(Player&& other) noexcept;
    Player& operator=(Player&& other) noexcept;
    
    // Delete copy constructor and assignment
    Player(const Player&) = delete;
    Player& operator=(const Player&) = delete;
    
    // Update methods
    void Update(float dt); // Base update for cooldowns and respawn timer
    void Update(float dt, const InputManager& inputManager); // Full update with input handling
    
    // Combat methods
    BulletParams Shoot(const sf::Vector2f& mouseWorldPos);
    BulletParams AttemptShoot(const sf::Vector2f& mouseWorldPos);
    void TakeDamage(int amount);
    void TakeDamage(int amount, const std::string& attackerID);
    bool IsDead() const;
    void Die(const sf::Vector2f& deathPosition);
    void Respawn();
    
    // Collision detection
    bool CheckBulletCollision(const sf::Vector2f& bulletPos, float bulletRadius) const;
    
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
    
    // Force field methods
    void InitializeForceField();
    void EnableForceField(bool enable);
    bool HasForceField() const { return forceField && forceFieldEnabled; }
    ForceField* GetForceField() const { return forceField.get(); }
    void SetForceFieldZapCallback(const std::function<void(int, float, bool)>& callback);
    
    // Bullet properties
    float GetBulletDamage() const { return bulletDamage; }
    void SetBulletDamage(float newDamage) { bulletDamage = newDamage; }
    
    // Respawn properties
    bool IsRespawning() const { return isRespawning; }
    float GetRespawnTimer() const { return respawnTimer; }
    void UpdateRespawn(float dt);
    
    // Player identification
    void SetPlayerID(const std::string& id) { playerID = id; }
    const std::string& GetPlayerID() const { return playerID; }
    
    // Callback setters
    void SetDeathCallback(const DeathCallback& callback) { onDeath = callback; }
    void SetRespawnCallback(const RespawnCallback& callback) { onRespawn = callback; }
    void SetDamageCallback(const DamageCallback& callback) { onDamage = callback; }
    
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
    
    // Respawn properties
    sf::Vector2f respawnPosition;
    float respawnTimer;
    bool isRespawning;
    static constexpr float RESPAWN_TIME = 3.0f;
    
    // Force field properties
    std::unique_ptr<ForceField> forceField;
    bool forceFieldEnabled = false;
    
    // Bullet properties
    float bulletDamage = BULLET_DAMAGE;
    float shootCooldownDuration = SHOOT_COOLDOWN_DURATION;
    
    // Player identification
    std::string playerID = "";
    std::string lastAttackerID = "";
    
    // Event callbacks
    DeathCallback onDeath;
    RespawnCallback onRespawn;
    DamageCallback onDamage;
};

#endif // PLAYER_H