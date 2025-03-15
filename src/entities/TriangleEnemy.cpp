#include "TriangleEnemy.h"
#include <sstream>
#include <cmath>

TriangleEnemy::TriangleEnemy(int id, const sf::Vector2f& position, float speed, int health)
    : EnemyBase(id, position, speed, health), direction(0.f, 0.f), lastPosition(position)
{
    InitializeShape(position);
}

void TriangleEnemy::InitializeShape(const sf::Vector2f& position)
{
    // Create an equilateral triangle
    shape.setPointCount(3);
    
    // Define triangle points
    shape.setPoint(0, sf::Vector2f(0, -TRIANGLE_SIZE));         // Top
    shape.setPoint(1, sf::Vector2f(-TRIANGLE_SIZE, TRIANGLE_SIZE));      // Bottom left
    shape.setPoint(2, sf::Vector2f(TRIANGLE_SIZE, TRIANGLE_SIZE));       // Bottom right
    
    // Set position and visual properties
    shape.setPosition(position);
    shape.setFillColor(sf::Color(255, 0, 0, 200)); // Semi-transparent red
    shape.setOrigin(0, 0); // Center origin for rotation
    
    UpdateVisuals();
}

// Replace the existing Update method in TriangleEnemy.cpp
void TriangleEnemy::Update(float dt, const sf::Vector2f& targetPosition)
{
    if (isDead) return;
    
    // Store last position for delta tracking
    lastPosition = shape.getPosition();
    
    // Calculate direction vector to target
    sf::Vector2f currentPos = shape.getPosition();
    
    // NEW: Check if the target is very far away (which can happen when a player dies)
    // and smoothly transition to the new target
    float distanceToTarget = std::sqrt(
        std::pow(targetPosition.x - currentPos.x, 2) + 
        std::pow(targetPosition.y - currentPos.y, 2)
    );
    
    // Calculate new direction with dampening to smooth sudden direction changes
    sf::Vector2f newDirection = targetPosition - currentPos;
    
    // Normalize new direction vector
    float newLength = std::sqrt(newDirection.x * newDirection.x + newDirection.y * newDirection.y);
    if (newLength > 0) {
        newDirection /= newLength;
    }
    
    // Smooth direction transition (SLERP-like for directions)
    // Use a slower transition factor when target changes significantly
    float transitionFactor = 0.1f;
    
    // If we have a large change in direction (like when a player dies and we retarget),
    // make the transition even smoother to avoid visual jitter
    float directionDot = (direction.x * newDirection.x + direction.y * newDirection.y);
    if (directionDot < 0.7f) { // Less than ~45 degrees means significant direction change
        transitionFactor = 0.05f; // Slower transition for large direction changes
    }
    
    // Calculate interpolated direction
    direction.x = direction.x + (newDirection.x - direction.x) * transitionFactor;
    direction.y = direction.y + (newDirection.y - direction.y) * transitionFactor;
    
    // Re-normalize the interpolated direction
    float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
    if (length > 0) {
        direction /= length;
    }
    
    // Move toward target using the smoothed direction
    sf::Vector2f movement = direction * movementSpeed * dt;
    shape.move(movement);
    
    // Update interpolation for smoother movement
    UpdateInterpolation(dt);
    
    // Update the base class position
    position = shape.getPosition();
    
    // Update rotation to face direction of movement with smoother transitions
    float targetAngle = std::atan2(direction.y, direction.x) * 180 / 3.14159f + 90.f;
    
    // Smoothly interpolate rotation (prevents jittery rotation)
    float currentAngle = shape.getRotation();
    
    // Find the shortest path to the target angle
    float angleDiff = targetAngle - currentAngle;
    while (angleDiff > 180.0f) angleDiff -= 360.0f;
    while (angleDiff < -180.0f) angleDiff += 360.0f;
    
    // Use a slower rotation speed for more smoothness during target transitions
    float rotationSpeed = 0.08f; // Reduced from 0.1f for smoother turns
    
    // Interpolate the rotation
    float newAngle = currentAngle + angleDiff * rotationSpeed;
    shape.setRotation(newAngle);
}
// In TriangleEnemy.cpp
void TriangleEnemy::SetTargetPosition(const sf::Vector2f& target) {
    m_currentPosition = position; // Store current position as starting point
    m_targetPosition = target;    // Set the target to move toward
    m_interpolationFactor = 0.0f; // Reset interpolation progress
    m_hasTargetPosition = true;   // Enable interpolation
}

void TriangleEnemy::UpdateInterpolation(float dt) {
    // Only interpolate if we have a target
    if (m_hasTargetPosition) {
        // Increase interpolation factor based on time
        m_interpolationFactor += dt * 5.0f;
        
        // Cap at 1.0
        if (m_interpolationFactor > 1.0f) {
            m_interpolationFactor = 1.0f;
            
            float distance = std::sqrt(
                std::pow(position.x - m_targetPosition.x, 2) + 
                std::pow(position.y - m_targetPosition.y, 2)
            );
            
            if (distance < 0.1f) {
                position = m_targetPosition;
                m_hasTargetPosition = false;
            }
        }
        
        // Smoothly interpolate position
        position = m_currentPosition + (m_targetPosition - m_currentPosition) * m_interpolationFactor;
    }
}
bool TriangleEnemy::CheckCollision(const sf::RectangleShape& playerShape) const {
    if (isDead) return false;
    
    // Get the global bounds of both shapes
    sf::FloatRect enemyBounds = shape.getGlobalBounds();
    sf::FloatRect playerBounds = playerShape.getGlobalBounds();
    
    // Check for intersection
    return enemyBounds.intersects(playerBounds);
}

bool TriangleEnemy::CheckBulletCollision(const sf::Vector2f& bulletPos, float bulletRadius)
{
    if (isDead) return false;
    
    // Simple circle vs triangle collision
    // Get triangle center
    sf::Vector2f center = shape.getPosition();
    
    // Get distance between centers
    float dx = center.x - bulletPos.x;
    float dy = center.y - bulletPos.y;
    float distance = std::sqrt(dx * dx + dy * dy);
    
    // Approximate triangle as a circle for faster collision checks
    return distance < (TRIANGLE_SIZE + bulletRadius);
}

void TriangleEnemy::UpdateVisuals()
{
    // Visual feedback based on health
    int healthPercent = health * 100 / TRIANGLE_HEALTH; // Using config value for max health
    
    // Adjust color based on health (from red to darker red as health decreases)
    sf::Color color = sf::Color(255, 
                               static_cast<sf::Uint8>(healthPercent), 
                               static_cast<sf::Uint8>(healthPercent),
                               200);
    shape.setFillColor(color);
}

std::string TriangleEnemy::Serialize() const
{
    std::stringstream ss;
    sf::Vector2f pos = shape.getPosition();
    
    // Format: id|posX|posY|health|isDead|dirX|dirY
    ss << id << "|" 
       << pos.x << "|" 
       << pos.y << "|" 
       << health << "|" 
       << (isDead ? 1 : 0) << "|"
       << direction.x << "|"
       << direction.y;
    
    return ss.str();
}

TriangleEnemy TriangleEnemy::Deserialize(const std::string& data)
{
    std::stringstream ss(data);
    std::string token;
    
    // Parse id
    std::getline(ss, token, '|');
    int id = std::stoi(token);
    
    // Parse position
    std::getline(ss, token, '|');
    float posX = std::stof(token);
    std::getline(ss, token, '|');
    float posY = std::stof(token);
    
    // Parse health
    std::getline(ss, token, '|');
    int health = std::stoi(token);
    
    // Create enemy with parsed data using config-defined values
    TriangleEnemy enemy(id, sf::Vector2f(posX, posY), ENEMY_SPEED * 1.2f, health);
    
    // Parse isDead
    std::getline(ss, token, '|');
    if (std::stoi(token) == 1) {
        enemy.TakeDamage(health); // Kill the enemy
    }
    
    // Parse direction if available
    if (std::getline(ss, token, '|')) {
        float dirX = std::stof(token);
        std::getline(ss, token, '|');
        float dirY = std::stof(token);
        enemy.direction = sf::Vector2f(dirX, dirY);
    }
    
    return enemy;
}

sf::Vector2f TriangleEnemy::GetPositionDelta(const sf::Vector2f& lastSyncedPosition) const
{
    sf::Vector2f currentPos = shape.getPosition();
    return sf::Vector2f(currentPos.x - lastSyncedPosition.x, 
                        currentPos.y - lastSyncedPosition.y);
}

void TriangleEnemy::UpdatePosition(const sf::Vector2f& newPosition, bool interpolate)
{
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

