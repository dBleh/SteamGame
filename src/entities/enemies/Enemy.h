#ifndef ENEMY_H
#define ENEMY_H

#include <SFML/Graphics.hpp>
#include <functional>
#include <memory>
#include <string>
#include "../../network/messages/MessageHandler.h"
#include "../../utils/config/EnemyConfig.h"
#include "EnemyTypes.h"

// Forward declarations
class Game;
class PlayerManager;
class EnemyManager;

class Enemy {
public:
    // Callback type definitions
    using DeathCallback = std::function<void(int, const sf::Vector2f&, const std::string&)>;
    using DamageCallback = std::function<void(int, float, float)>;
    using PlayerCollisionCallback = std::function<void(int, const std::string&)>;
    
    Enemy(int id, const sf::Vector2f& position, float health = ENEMY_HEALTH, float speed = ENEMY_SPEED);
    virtual ~Enemy() = default;

    // Core functionality
    virtual void Update(float dt, PlayerManager& playerManager);
    virtual void Render(sf::RenderWindow& window);
    virtual bool CheckBulletCollision(const sf::Vector2f& bulletPos, float bulletRadius);
    virtual bool CheckPlayerCollision(const sf::RectangleShape& playerShape);
    
    // Getters/Setters
    sf::Vector2f GetPosition() const { return position; }
    void SetPosition(const sf::Vector2f& pos) { position = pos; }
    bool IsDead() const { return health <= 0.0f; }
    float GetHealth() const { return health; }
    void SetHealth(float newHealth) { health = newHealth; }
    int GetID() const { return id; }
    void SetID(int newId) { id = newId; }
    virtual EnemyType GetType() const = 0;
    float GetRadius() const { return radius; }
    
    // Network serialization
    virtual std::string Serialize() const;
    virtual void Deserialize(const std::string& data);
    
    // Damage handling
    bool TakeDamage(float amount);
    bool TakeDamage(float amount, const std::string& attackerID);
    void Die(const std::string& killerID = "");
    
    // Movement
    sf::Vector2f GetVelocity() const { return velocity; }
    void SetVelocity(const sf::Vector2f& vel) { velocity = vel; }
    
    // Callback setters
    void SetDeathCallback(const DeathCallback& callback) { onDeath = callback; }
    void SetDamageCallback(const DamageCallback& callback) { onDamage = callback; }
    void SetPlayerCollisionCallback(const PlayerCollisionCallback& callback) { onPlayerCollision = callback; }
    
protected:
    // Core properties
    int id;
    sf::Vector2f position;
    sf::Vector2f velocity;
    float health;
    float speed;
    float radius;
    bool hasTarget;
    sf::Vector2f targetPosition;
    std::string lastAttackerID;
    
    // Visualization data
    virtual void UpdateVisualRepresentation();
    
    // Movement behavior 
    virtual void UpdateMovement(float dt, PlayerManager& playerManager);
    virtual void FindTarget(PlayerManager& playerManager);
    
    // Callbacks
    DeathCallback onDeath;
    DamageCallback onDamage;
    PlayerCollisionCallback onPlayerCollision;
};

// Factory function to create enemies by type
std::unique_ptr<Enemy> CreateEnemy(EnemyType type, int id, const sf::Vector2f& position);

#endif // ENEMY_H