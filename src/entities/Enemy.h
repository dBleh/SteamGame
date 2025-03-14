#ifndef ENEMY_H
#define ENEMY_H

#include <SFML/Graphics.hpp>
#include <string>
#include <cmath>
#include <unordered_map>
#include <chrono>
#include <sstream>
#include <iostream>
class Enemy {
public:
    Enemy(int id, const sf::Vector2f& position, float speed = 60.f, int health = 40);
    
    void Update(float dt, const sf::Vector2f& targetPosition);
    bool TakeDamage(int amount);
    bool CheckCollision(const sf::RectangleShape& playerShape);
    void ProcessHit(int damage, const std::string& bulletID);
    sf::RectangleShape& GetShape();
    const sf::RectangleShape& GetShape() const;
    sf::Vector2f GetPosition() const;
    bool IsAlive() const;
    int GetID() const;
    void SetHealth(int newHealth) { health = newHealth; }
    void UpdateVisuals();
    bool GetIsDead(){return isDead;}
    // Serialization for network
    std::string Serialize() const;
    static Enemy Deserialize(const std::string& data);
    int GetHealth() const { return health; }
    
private:
    std::unordered_map<std::string, float> recentHits;
    int id;                     // Unique ID for this enemy
    sf::RectangleShape shape;   // Visual representation
    float movementSpeed;        // Speed in pixels per second
    int health;                 // Current health
    bool isDead;                // Is the enemy dead?
    sf::Color color;            // Enemy color
};

#endif // ENEMY_H