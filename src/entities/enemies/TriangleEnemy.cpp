#include "TriangleEnemy.h"
#include <cmath>
#include <iostream>

TriangleEnemy::TriangleEnemy(int id, const sf::Vector2f& position, float health, float speed)
    : Enemy(id, position, health, speed), rotationAngle(0.0f), rotationSpeed(90.0f) {
    
    // Setup the triangle shape
    shape.setPointCount(3);
    shape.setPoint(0, sf::Vector2f(0, -TRIANGLE_SIZE)); // Top
    shape.setPoint(1, sf::Vector2f(-TRIANGLE_SIZE * 0.866f, TRIANGLE_SIZE * 0.5f)); // Bottom left
    shape.setPoint(2, sf::Vector2f(TRIANGLE_SIZE * 0.866f, TRIANGLE_SIZE * 0.5f)); // Bottom right
    
    // Set origin to center
    shape.setOrigin(0, 0);
    
    // Color
    shape.setFillColor(sf::Color(255, 140, 0)); // Orange color
    shape.setOutlineColor(sf::Color(200, 80, 0)); // Darker orange outline
    shape.setOutlineThickness(1.0f);
    
    // Update position to match the starting position
    UpdateVisualRepresentation();
}

void TriangleEnemy::UpdateVisualRepresentation() {
    // Set position
    shape.setPosition(position);
    
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