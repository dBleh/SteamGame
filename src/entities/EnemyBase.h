#ifndef ENEMY_BASE_H
#define ENEMY_BASE_H

#include "../utils/config.h"
#include <SFML/Graphics.hpp>
#include <string>

// Base class for all enemy types
class EnemyBase {
public:
    EnemyBase(int id, const sf::Vector2f& position, float speed = ENEMY_SPEED, int health = TRIANGLE_HEALTH)
        : id(id), position(position), movementSpeed(speed), health(health), isDead(false) {}
    
    virtual ~EnemyBase() = default;
    
    // Core methods that all enemy types must implement
    virtual void Update(float dt, const sf::Vector2f& targetPosition) = 0;
    virtual bool CheckCollision(const sf::RectangleShape& playerShape) const = 0;
    virtual void UpdateVisuals() = 0;
    virtual std::string Serialize() const = 0;
    
    // Common enemy methods
    virtual bool TakeDamage(int amount) {
        health -= amount;
        UpdateVisuals();
        
        if (health <= 0) {
            health = 0;
            isDead = true;
            return true; // Enemy died from this damage
        }
        return false; // Enemy still alive
    }
    
    // Common getters
    int GetID() const { return id; }
    sf::Vector2f GetPosition() const { return position; }
    bool IsAlive() const { return !isDead; }
    int GetHealth() const { return health; }
    float GetSpeed() const { return movementSpeed; }
    
    // Common setters
    void SetHealth(int newHealth) { 
        health = newHealth;
        isDead = (health <= 0);
        UpdateVisuals();
    }
    
protected:
    int id;
    sf::Vector2f position;
    float movementSpeed;
    int health;
    bool isDead;
};

#endif // ENEMY_BASE_H