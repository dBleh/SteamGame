#include "Enemy.h"
#include <sstream>
#include <iostream>

Enemy::Enemy(int id, const sf::Vector2f& position, float speed, int health)
    : EnemyBase(id, position, speed, health) {
    // Create enemy shape (red square)
    shape.setSize(sf::Vector2f(ENEMY_SIZE, ENEMY_SIZE));
    shape.setFillColor(sf::Color::Red);
    shape.setOutlineThickness(2.f);
    shape.setOutlineColor(sf::Color::Black);
    
    // Center the enemy shape for accurate collision
    shape.setOrigin(ENEMY_ORIGIN, ENEMY_ORIGIN);
    
    // Set the initial position
    shape.setPosition(position);
}
bool Enemy::CheckBulletCollision(const sf::Vector2f& bulletPos, float bulletRadius) {
    if (isDead) return false;
    
    // Get the center of the enemy shape
    sf::Vector2f enemyPos = shape.getPosition();
    float halfSize = ENEMY_SIZE / 2.0f;
    
    // Simple AABB vs Circle collision
    float nearestX = std::max(enemyPos.x - halfSize, std::min(bulletPos.x, enemyPos.x + halfSize));
    float nearestY = std::max(enemyPos.y - halfSize, std::min(bulletPos.y, enemyPos.y + halfSize));
    
    float distX = bulletPos.x - nearestX;
    float distY = bulletPos.y - nearestY;
    float distSquared = (distX * distX) + (distY * distY);
    
    return distSquared <= (bulletRadius * bulletRadius);
}
void Enemy::Update(float dt, const sf::Vector2f& targetPosition) {
    if (isDead) return;
    
    // Calculate direction vector to target
    sf::Vector2f currentPos = shape.getPosition();
    sf::Vector2f direction = targetPosition - currentPos;
    
    // Normalize direction vector
    float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
    if (length > 0) {
        direction /= length;
    }
    
    // Move towards target
    shape.move(direction * movementSpeed * dt);
    
    // Update the base class position
    position = shape.getPosition();
}

bool Enemy::CheckCollision(const sf::RectangleShape& playerShape) const {
    if (isDead) return false;
    
    // Simple AABB collision
    return shape.getGlobalBounds().intersects(playerShape.getGlobalBounds());
}

void Enemy::UpdateVisuals() {
    // Update color based on health percentage
    float healthPercent = static_cast<float>(health) / TRIANGLE_HEALTH; // Based on initial health
    sf::Color newColor(
        255,  // Full red
        static_cast<sf::Uint8>(255 * healthPercent),  // Less green as damage increases
        static_cast<sf::Uint8>(255 * healthPercent)   // Less blue as damage increases
    );
    shape.setFillColor(newColor);
}

std::string Enemy::Serialize() const {
    std::ostringstream oss;
    sf::Vector2f pos = shape.getPosition();
    oss << id << "," << pos.x << "," << pos.y << "," << health << "," << (isDead ? "1" : "0");
    return oss.str();
}

Enemy Enemy::Deserialize(const std::string& data) {
    std::istringstream iss(data);
    int id;
    float x, y;
    int health;
    std::string deadStr;
    char comma;
    
    iss >> id >> comma >> x >> comma >> y >> comma >> health >> comma >> deadStr;
    
    Enemy enemy(id, sf::Vector2f(x, y), ENEMY_SPEED, health);
    if (deadStr == "1") {
        enemy.isDead = true;
    }
    
    return enemy;
}