#include "TriangleEnemy.h"
#include <cmath>
#include <iostream>

TriangleEnemy::TriangleEnemy(int id, const sf::Vector2f& position, float health, float speed)
    : Enemy(id, position, health, speed), rotationAngle(0.0f), rotationSpeed(ENEMY_ROTATION_SPEED) {
    
    // Setup the triangle shape
    shape.setPointCount(3);
    shape.setPoint(0, sf::Vector2f(0, -TRIANGLE_SIZE)); // Top
    shape.setPoint(1, sf::Vector2f(-TRIANGLE_SIZE * 0.866f, TRIANGLE_SIZE * 0.5f)); // Bottom left
    shape.setPoint(2, sf::Vector2f(TRIANGLE_SIZE * 0.866f, TRIANGLE_SIZE * 0.5f)); // Bottom right
    
    // Set origin to center
    shape.setOrigin(0, 0);
    
    // Color
    shape.setFillColor(TRIANGLE_FILL_COLOR); // Orange color
    shape.setOutlineColor(TRIANGLE_OUTLINE_COLOR); // Darker orange outline
    shape.setOutlineThickness(ENEMY_OUTLINE_THICKNESS);
    
    // Update position to match the starting position
    UpdateVisualRepresentation();
}

void TriangleEnemy::UpdateVisualRepresentation() {
    // Set position
    shape.setPosition(position);
    
    // Update rotation to face the target if we have one
    if (hasTarget && (velocity.x != 0 || velocity.y != 0)) {
        float angle = std::atan2(velocity.y, velocity.x) * 180 / 3.14159f;
        shape.setRotation(angle + TRIANGLE_ROTATION_OFFSET); // +90 to point the triangle in the movement direction
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

// Factory function implementation for triangle
std::unique_ptr<Enemy> CreateEnemy(EnemyType type, int id, const sf::Vector2f& position) {
    switch (type) {
        case EnemyType::Triangle:
            return std::make_unique<TriangleEnemy>(id, position);
        // Add cases for future enemy types
        default:
            std::cerr << "Unknown enemy type: " << static_cast<int>(type) << std::endl;
            return std::make_unique<TriangleEnemy>(id, position); // Default to triangle
    }
}