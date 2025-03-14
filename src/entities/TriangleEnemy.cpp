#include "TriangleEnemy.h"
#include <sstream>
#include <cmath>

TriangleEnemy::TriangleEnemy(int id, const sf::Vector2f& position, float speed, int health)
    : id(id), movementSpeed(speed), health(health), isDead(false), direction(0.f, 0.f), lastPosition(position)
{
    InitializeShape(position);
}

void TriangleEnemy::InitializeShape(const sf::Vector2f& position)
{
    // Create an equilateral triangle
    shape.setPointCount(3);
    float size = 15.f; // Base size for triangle
    
    // Define triangle points
    shape.setPoint(0, sf::Vector2f(0, -size));         // Top
    shape.setPoint(1, sf::Vector2f(-size, size));      // Bottom left
    shape.setPoint(2, sf::Vector2f(size, size));       // Bottom right
    
    // Set position and visual properties
    shape.setPosition(position);
    shape.setFillColor(sf::Color(255, 0, 0, 200)); // Semi-transparent red
    shape.setOrigin(0, 0); // Center origin for rotation
    
    UpdateVisuals();
}

void TriangleEnemy::Update(float dt, const sf::Vector2f& targetPosition)
{
    if (isDead) return;
    
    lastPosition = shape.getPosition();
    
    // Calculate direction vector to target
    sf::Vector2f currentPos = shape.getPosition();
    direction = targetPosition - currentPos;
    
    // Normalize direction vector
    float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
    if (length > 0) {
        direction /= length;
    }
    
    // Move toward target
    sf::Vector2f movement = direction * movementSpeed * dt;
    shape.move(movement);
    
    // Update rotation to face direction of movement
    float angle = std::atan2(direction.y, direction.x) * 180 / 3.14159f;
    shape.setRotation(angle + 90.f); // Adjust to make triangle point forward
}

bool TriangleEnemy::TakeDamage(int amount)
{
    if (isDead) return false;
    
    health -= amount;
    if (health <= 0) {
        health = 0;
        isDead = true;
        return true; // Was killed
    }
    
    UpdateVisuals();
    return false; // Still alive
}


bool TriangleEnemy::CheckCollision(const sf::RectangleShape& playerShape) {
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
    // More precise collision can be implemented if needed
    float triangleRadius = 15.f; // Same as size in InitializeShape
    
    return distance < (triangleRadius + bulletRadius);
}

void TriangleEnemy::UpdateVisuals()
{
    // Visual feedback based on health
    int healthPercent = health * 100 / 40; // Assuming max health is 40
    
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
    
    // Create enemy with parsed data
    TriangleEnemy enemy(id, sf::Vector2f(posX, posY), 60.f, health);
    
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
        sf::Vector2f targetPos = newPosition;
        
        // Move 25% of the way to the target (adjust as needed for smoothness)
        sf::Vector2f interpolatedPos = currentPos + (targetPos - currentPos) * 0.25f;
        shape.setPosition(interpolatedPos);
    }
    else {
        // Directly set position (for server-side or when correction is needed)
        shape.setPosition(newPosition);
    }
}