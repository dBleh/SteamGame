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
    
    // Initialize visuals based on health
    UpdateVisuals();
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
void Enemy::SetTargetPosition(const sf::Vector2f& target) {
    m_currentPosition = position; // Store current position as starting point
    m_targetPosition = target;    // Set the target to move toward
    m_interpolationFactor = 0.0f; // Reset interpolation progress
    m_hasTargetPosition = true;   // Enable interpolation
}

void Enemy::UpdateInterpolation(float dt) {
    // Only interpolate if we have a target
    if (m_hasTargetPosition) {
        // Increase interpolation factor based on time
        m_interpolationFactor += dt * 5.0f; // Adjust speed multiplier as needed
        
        // Cap at 1.0
        if (m_interpolationFactor > 1.0f) {
            m_interpolationFactor = 1.0f;
            
            // If we've reached the target (or very close), complete the movement
            float distance = std::sqrt(
                std::pow(position.x - m_targetPosition.x, 2) + 
                std::pow(position.y - m_targetPosition.y, 2)
            );
            
            if (distance < 0.1f) {
                position = m_targetPosition;  // Snap to exact position
                m_hasTargetPosition = false;  // Interpolation complete
            }
        }
        
        // Smoothly interpolate between current and target position
        position = m_currentPosition + (m_targetPosition - m_currentPosition) * m_interpolationFactor;
    }
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
    UpdateInterpolation(dt);
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

// Add position update method with interpolation
void Enemy::UpdatePosition(const sf::Vector2f& newPosition, bool interpolate) {
    if (isDead) return;
    
    if (interpolate) {
        // Smoothly interpolate to the new position (for client-side)
        sf::Vector2f currentPos = shape.getPosition();
        sf::Vector2f targetPos = newPosition;
        
        // Use a smoother interpolation factor (0.15 instead of 0.25)
        // This makes movement appear less jerky
        sf::Vector2f interpolatedPos = currentPos + (targetPos - currentPos) * 0.15f;
        
        // Add a small damping factor to prevent micro-oscillations
        // which can cause the visual "blinking" effect
        const float DAMPING_THRESHOLD = 1.0f;
        float distance = std::sqrt(
            std::pow(targetPos.x - currentPos.x, 2) + 
            std::pow(targetPos.y - currentPos.y, 2)
        );
        
        if (distance < DAMPING_THRESHOLD) {
            // If we're very close to the target, just snap to it
            // This prevents oscillation around the target position
            shape.setPosition(targetPos);
        } else {
            // Otherwise use the interpolated position
            shape.setPosition(interpolatedPos);
        }
    }
    else {
        // Directly set position (for server-side or when correction is needed)
        shape.setPosition(newPosition);
    }
    
    // Update the base class position
    position = shape.getPosition();
}