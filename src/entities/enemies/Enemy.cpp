#include "Enemy.h"
#include "../player/PlayerManager.h"
#include "Enemy.h"
#include "TriangleEnemy.h"
#include "SquareEnemy.h"
#include "PentagonEnemy.h"
#include <cmath>
#include <sstream>
#include <iostream>

Enemy::Enemy(int id, const sf::Vector2f& position, float health, float speed)
    : id(id), 
      position(position), 
      velocity(0.0f, 0.0f),
      health(health), 
      speed(speed),
      radius(ENEMY_SIZE / 2.0f),
      hasTarget(false),
      targetPosition(0.0f, 0.0f),
      lastAttackerID("") {
}

void Enemy::Update(float dt, PlayerManager& playerManager) {
    if (IsDead()) return;

    FindTarget(playerManager);
    UpdateMovement(dt, playerManager);
    UpdateVisualRepresentation();
}

void Enemy::FindTarget(PlayerManager& playerManager) {
    // Default implementation finds closest player
    const auto& players = playerManager.GetPlayers();
    float closestDistance = 99999.0f;
    hasTarget = false;
    
    for (const auto& pair : players) {
        // Skip dead players
        if (pair.second.player.IsDead()) continue;
        
        sf::Vector2f playerPos = pair.second.player.GetPosition();
        float distance = std::hypot(position.x - playerPos.x, position.y - playerPos.y);
        
        if (distance < closestDistance) {
            closestDistance = distance;
            targetPosition = playerPos;
            hasTarget = true;
        }
    }
}

void Enemy::UpdateMovement(float dt, PlayerManager& playerManager) {
    if (!hasTarget) return;
    
    // Calculate direction to target
    sf::Vector2f direction = targetPosition - position;
    float distance = std::hypot(direction.x, direction.y);
    
    if (distance > 1.0f) {  // Avoid division by zero
        // Normalize
        direction.x /= distance;
        direction.y /= distance;
        
        // Apply velocity
        velocity = direction * speed;
        position += velocity * dt;
    }
}

void Enemy::UpdateVisualRepresentation() {
    // Base class doesn't have a specific representation
    // Derived classes will implement this
}

void Enemy::Render(sf::RenderWindow& window) {
    // Base class doesn't render anything
    // Derived classes will implement this
}

bool Enemy::CheckBulletCollision(const sf::Vector2f& bulletPos, float bulletRadius) {
    float distanceSquared = 
        (position.x - bulletPos.x) * (position.x - bulletPos.x) + 
        (position.y - bulletPos.y) * (position.y - bulletPos.y);
    
    float radiusSum = radius + bulletRadius;
    return distanceSquared <= (radiusSum * radiusSum);
}

bool Enemy::CheckPlayerCollision(const sf::RectangleShape& playerShape) {
    // Get player bounds
    sf::FloatRect playerBounds = playerShape.getGlobalBounds();
    sf::Vector2f playerCenter(
        playerBounds.left + playerBounds.width / 2.0f,
        playerBounds.top + playerBounds.height / 2.0f
    );
    
    // Calculate distances
    float closestX = std::max(playerBounds.left, std::min(position.x, playerBounds.left + playerBounds.width));
    float closestY = std::max(playerBounds.top, std::min(position.y, playerBounds.top + playerBounds.height));
    
    // Calculate distance between closest point and circle center
    float distanceX = position.x - closestX;
    float distanceY = position.y - closestY;
    float distanceSquared = (distanceX * distanceX) + (distanceY * distanceY);
    
    return distanceSquared <= (radius * radius);
}

bool Enemy::TakeDamage(float amount) {
    // Call the overloaded version with empty attacker ID
    return TakeDamage(amount, "");
}

bool Enemy::TakeDamage(float amount, const std::string& attackerID) {
    // Store the attacker ID
    if (!attackerID.empty()) {
        lastAttackerID = attackerID;
    }
    
    float oldHealth = health;
    health -= amount;
    
    // Call damage callback if set
    if (onDamage) {
        onDamage(id, amount, oldHealth - health);
    }
    
    // Check if enemy died from this damage
    if (health <= 0 && oldHealth > 0) {
        Die(attackerID);
        return true;
    }
    
    return false;
}

void Enemy::Die(const std::string& killerID) {
    // Set health to zero
    health = 0;
    
    // Call death callback if set
    if (onDeath) {
        onDeath(id, position, killerID.empty() ? lastAttackerID : killerID);
    }
}

std::string Enemy::Serialize() const {
    std::ostringstream oss;
    oss << id << "|"
        << static_cast<int>(GetType()) << "|"
        << position.x << "," << position.y << "|"
        << health;
    return oss.str();
}

void Enemy::Deserialize(const std::string& data) {
    std::istringstream iss(data);
    std::string token;
    
    // Parse ID
    if (std::getline(iss, token, '|')) {
        id = std::stoi(token);
    }
    
    // Skip type (already known when deserializing into concrete type)
    std::getline(iss, token, '|');
    
    // Parse position
    if (std::getline(iss, token, '|')) {
        size_t commaPos = token.find(',');
        if (commaPos != std::string::npos) {
            position.x = std::stof(token.substr(0, commaPos));
            position.y = std::stof(token.substr(commaPos + 1));
        }
    }
    
    // Parse health
    if (std::getline(iss, token, '|')) {
        health = std::stof(token);
    }
}

std::unique_ptr<Enemy> CreateEnemy(EnemyType type, int id, const sf::Vector2f& position) {
    switch (type) {
        case EnemyType::Triangle:
            return std::unique_ptr<Enemy>(new TriangleEnemy(id, position));
        case EnemyType::Square:
            return std::unique_ptr<Enemy>(new SquareEnemy(id, position));
        case EnemyType::Pentagon:
            return std::unique_ptr<Enemy>(new PentagonEnemy(id, position));
        default:
            std::cerr << "Unknown enemy type: " << static_cast<int>(type) << std::endl;
            return std::unique_ptr<Enemy>(new TriangleEnemy(id, position));
    }
}