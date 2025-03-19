#include "TriangleEnemy.h"
#include <cmath>
#include <iostream>

TriangleEnemy::TriangleEnemy(int id, const sf::Vector2f& position, float health, float speed)
    : Enemy(id, position, health, speed), 
      rotationAngle(0.0f), 
      rotationSpeed(90.0f),
      triangleSize(TRIANGLE_SIZE) {
    
    // Setup the triangle shape
    shape.setPointCount(3);
    shape.setPoint(0, sf::Vector2f(0, -triangleSize)); // Top
    shape.setPoint(1, sf::Vector2f(-triangleSize * 0.866f, triangleSize * 0.5f)); // Bottom left
    shape.setPoint(2, sf::Vector2f(triangleSize * 0.866f, triangleSize * 0.5f)); // Bottom right
    
    // Set origin to center
    shape.setOrigin(0, 0);
    
    // Color
    shape.setFillColor(sf::Color(255, 140, 0)); // Orange color
    shape.setOutlineColor(sf::Color(200, 80, 0)); // Darker orange outline
    shape.setOutlineThickness(1.0f);
    
    // Initialize radius based on triangle size
    radius = triangleSize;
    
    // Update position to match the starting position
    UpdateVisualRepresentation();
}


TriangleEnemy::TriangleEnemy(int id, const sf::Vector2f& position, float health, float speed, float size)
    : Enemy(id, position, health, speed), 
      rotationAngle(0.0f), 
      rotationSpeed(90.0f),
      triangleSize(size) {
    
    // Setup the triangle shape
    shape.setPointCount(3);
    shape.setPoint(0, sf::Vector2f(0, -triangleSize)); // Top
    shape.setPoint(1, sf::Vector2f(-triangleSize * 0.866f, triangleSize * 0.5f)); // Bottom left
    shape.setPoint(2, sf::Vector2f(triangleSize * 0.866f, triangleSize * 0.5f)); // Bottom right
    
    // Set origin to center
    shape.setOrigin(0, 0);
    
    // Color
    shape.setFillColor(sf::Color(255, 140, 0)); // Orange color
    shape.setOutlineColor(sf::Color(200, 80, 0)); // Darker orange outline
    shape.setOutlineThickness(1.0f);
    
    // Initialize radius based on triangle size
    radius = triangleSize;
    
    // Update position to match the starting position
    UpdateVisualRepresentation();
}

void TriangleEnemy::UpdateVisualRepresentation() {
    // Set position
    shape.setPosition(position);
    
    // Update the triangle shape according to current triangleSize
    shape.setPoint(0, sf::Vector2f(0, -triangleSize)); // Top
    shape.setPoint(1, sf::Vector2f(-triangleSize * 0.866f, triangleSize * 0.5f)); // Bottom left
    shape.setPoint(2, sf::Vector2f(triangleSize * 0.866f, triangleSize * 0.5f)); // Bottom right
    
    // Update rotation to face the target if we have one
    if (hasTarget && (velocity.x != 0 || velocity.y != 0)) {
        float angle = std::atan2(velocity.y, velocity.x) * 180 / 3.14159f;
        shape.setRotation(angle + 90); // +90 to point the triangle in the movement direction
    }
}

void TriangleEnemy::UpdateMovement(float dt, PlayerManager& playerManager) {
    // Call base class movement implementation
    Enemy::UpdateMovement(dt, playerManager);
    
    // Add rotation behavior specific to the triangle
    rotationAngle += rotationSpeed * dt;
    if (rotationAngle > 360.0f) {
        rotationAngle -= 360.0f;
    }
}

void TriangleEnemy::Render(sf::RenderWindow& window) {
    if (!IsDead()) {
        window.draw(shape);
    }
}

void TriangleEnemy::ApplySettings(GameSettingsManager* settingsManager) {
    // First, apply base class settings (speed, etc.)
    Enemy::ApplySettings(settingsManager);
    
    if (!settingsManager) return;
    
    // Apply triangle-specific settings
    GameSetting* triangleHealthSetting = settingsManager->GetSetting("triangle_health");
    if (triangleHealthSetting && health == TRIANGLE_HEALTH) {
        // Only update health if not damaged (still at default)
        health = triangleHealthSetting->GetFloatValue();
    }
    
    GameSetting* triangleDamageSetting = settingsManager->GetSetting("triangle_damage");
    if (triangleDamageSetting) {
        damage = triangleDamageSetting->GetFloatValue();
    }
    
    GameSetting* triangleSizeSetting = settingsManager->GetSetting("triangle_size");
    if (triangleSizeSetting) {
        triangleSize = triangleSizeSetting->GetFloatValue();
        // Also update the collision radius
        radius = triangleSize;
        // Update the visual representation with new size
        UpdateVisualRepresentation();
    }
    
    // Optionally, if you have a rotation speed setting
    GameSetting* rotationSpeedSetting = settingsManager->GetSetting("triangle_rotation_speed");
    if (rotationSpeedSetting) {
        rotationSpeed = rotationSpeedSetting->GetFloatValue();
    }
}