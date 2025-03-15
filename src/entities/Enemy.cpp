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
// Replace the existing Update method in Enemy.cpp
void Enemy::Update(float dt, const sf::Vector2f& targetPosition) {
    if (isDead) return;
    
    // Calculate direction vector to target
    sf::Vector2f currentPos = shape.getPosition();
    
    // NEW: Track the previous target direction for smooth transitions
    static sf::Vector2f prevDirection(0.f, 0.f);
    
    // Calculate new direction vector
    sf::Vector2f newDirection = targetPosition - currentPos;
    
    // Normalize new direction vector
    float newLength = std::sqrt(newDirection.x * newDirection.x + newDirection.y * newDirection.y);
    if (newLength > 0) {
        newDirection /= newLength;
    }
    
    // Initialize prevDirection if it's the first update
    if (prevDirection.x == 0.f && prevDirection.y == 0.f) {
        prevDirection = newDirection;
    }
    
    // Determine transition speed based on how much the direction has changed
    float directionDot = (prevDirection.x * newDirection.x + prevDirection.y * newDirection.y);
    float transitionFactor = 0.1f; // Default smooth transition
    
    // Slower transition for large direction changes (when retargeting after player death)
    if (directionDot < 0.7f) { // ~45 degrees or more change in direction
        transitionFactor = 0.05f; // Slower transition for large direction changes
    }
    
    // Smoothly interpolate between previous and new direction
    sf::Vector2f direction;
    direction.x = prevDirection.x + (newDirection.x - prevDirection.x) * transitionFactor;
    direction.y = prevDirection.y + (newDirection.y - prevDirection.y) * transitionFactor;
    
    // Normalize the interpolated direction
    float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
    if (length > 0) {
        direction /= length;
    }
    
    // Save current direction for next frame
    prevDirection = direction;
    
    // Handle interpolation for smooth client-side movement
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
        
        // Calculate distance to new position
        float distSquared = 
            (newPosition.x - currentPos.x) * (newPosition.x - currentPos.x) +
            (newPosition.y - currentPos.y) * (newPosition.y - currentPos.y);
        
        // NEW: Adaptive interpolation factor based on distance
        float interpFactor;
        
        if (distSquared > 10000.0f) { // More than 100 units away
            // For very large changes (like when retargeting after player death),
            // use SetTargetPosition which provides smoother animation over time
            SetTargetPosition(newPosition);
            return;
        }
        else if (distSquared > 2500.0f) { // 50 units distance
            // For larger distances use a smaller factor to move more quickly
            interpFactor = 0.2f;
        }
        else if (distSquared > 400.0f) { // 20 units distance
            // Medium distance
            interpFactor = 0.15f;
        }
        else {
            // Small distances use slowest factor for smoothest movement
            interpFactor = 0.1f;
        }
        
        // Apply the interpolation factor
        sf::Vector2f interpolatedPos = currentPos + (newPosition - currentPos) * interpFactor;
        
        // Damping threshold to prevent micro-oscillations
        const float DAMPING_THRESHOLD = 0.5f; // Reduced from 1.0f
        
        // If we're very close to the target, just snap to it
        if (distSquared < DAMPING_THRESHOLD * DAMPING_THRESHOLD) {
            shape.setPosition(newPosition);
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
