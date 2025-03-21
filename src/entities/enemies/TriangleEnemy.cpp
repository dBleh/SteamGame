#include "TriangleEnemy.h"
#include "../player/PlayerManager.h"
#include <cmath>
#include <iostream>

TriangleEnemy::TriangleEnemy(int id, const sf::Vector2f& position, float health, float speed)
    : Enemy(id, position, health, speed * 1.5f), // Base speed boost
      rotationAngle(0.0f),
      rotationSpeed(ENEMY_ROTATION_SPEED * 2.0f), // Faster rotation
      bounceTimer(0.0f),
      bounceAmplitude(TRIANGLE_BOUNCE_AMPLITUDE * 2.0f), // Stronger bounce
      bounceFrequency(TRIANGLE_BOUNCE_FREQUENCY * 2.0f) {
    
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

void TriangleEnemy::FindTarget(PlayerManager& playerManager) {
    Enemy::FindTarget(playerManager); // Use the base class implementation to find closest player
}

void TriangleEnemy::UpdateVisualRepresentation() {
    shape.setPosition(position);
    shape.setRotation(rotationAngle);
    
    // Pulse effect to make it more noticeable
    float scale = 1.0f + 0.1f * std::sin(bounceTimer);
    shape.setScale(scale, scale);
}

void TriangleEnemy::UpdateMovement(float dt, PlayerManager& playerManager) {
    if (!hasTarget) return;

    // Update bounce timer for visual effect
    bounceTimer += dt * 2.0f;
    if (bounceTimer > 2.0f * 3.14159f / bounceFrequency) {
        bounceTimer -= 2.0f * 3.14159f / bounceFrequency;
    }

    // Get enhanced speed
    float enhancedSpeed = speed * 1.5f;
    
    // Get target position from base Enemy class
    sf::Vector2f targetPos = targetPosition;
    
    // Calculate direction to target
    sf::Vector2f direction = targetPos - position;
    float distance = std::hypot(direction.x, direction.y);
    sf::Vector2f normalizedDir = (distance > 1.0f) ? direction / distance : sf::Vector2f(0.f, 0.f);

    // Simple pursuit movement with bounce effect
    sf::Vector2f movementVector = normalizedDir * enhancedSpeed;
    
    // Add slight bounce effect perpendicular to movement
    sf::Vector2f perpendicular(-normalizedDir.y, normalizedDir.x);
    float bounceOffset = bounceAmplitude * std::sin(bounceFrequency * bounceTimer);
    movementVector += perpendicular * bounceOffset;

    // Apply movement
    velocity = movementVector;
    position += velocity * dt;

    // Update rotation for visual feedback
    rotationAngle += rotationSpeed * dt * 2.0f;
    if (rotationAngle >= 360.f) rotationAngle -= 360.f;

    // Boundary checking
    const float boundary = 1000.f;
    if (position.x < -boundary || position.x > boundary || 
        position.y < -boundary || position.y > boundary) {
        position -= velocity * dt; // Bounce back
        velocity = -velocity * 0.8f;
    }
}

void TriangleEnemy::Render(sf::RenderWindow& window) {
    if (!IsDead()) {
        window.draw(shape);
    }
}