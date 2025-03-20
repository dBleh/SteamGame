#include "SquareEnemy.h"
#include "../player/PlayerManager.h"
#include <cmath>
#include <iostream>

SquareEnemy::SquareEnemy(int id, const sf::Vector2f& position, float health, float speed)
    : Enemy(id, position, health, speed),
      rotationAngle(0.0f),
      rotationSpeed(ENEMY_ROTATION_SPEED * 0.8f), // Slightly slower rotation
      currentAxisIndex(0),
      playerIntersectsLine(false),
      lastIntersectionPoint(position),
      lineLength(320.0f) {
    
    // Setup the square shape
    shape.setPointCount(4);
    shape.setPoint(0, sf::Vector2f(-SQUARE_SIZE/2, -SQUARE_SIZE/2)); // Top-left
    shape.setPoint(1, sf::Vector2f(SQUARE_SIZE/2, -SQUARE_SIZE/2));  // Top-right
    shape.setPoint(2, sf::Vector2f(SQUARE_SIZE/2, SQUARE_SIZE/2));   // Bottom-right
    shape.setPoint(3, sf::Vector2f(-SQUARE_SIZE/2, SQUARE_SIZE/2));  // Bottom-left
    
    // Set origin to center
    shape.setOrigin(0, 0);
    
    // Color
    shape.setFillColor(SQUARE_FILL_COLOR); // Blue color
    shape.setOutlineColor(SQUARE_OUTLINE_COLOR); // Darker blue outline
    shape.setOutlineThickness(ENEMY_OUTLINE_THICKNESS);
    
    // Initialize axes (4 directions 90 degrees apart)
    InitializeAxes();
    
    // Update position to match the starting position
    UpdateVisualRepresentation();
}

// Constructor with default values for health and speed
SquareEnemy::SquareEnemy(int id, const sf::Vector2f& position)
    : SquareEnemy(id, position, SQUARE_HEALTH, ENEMY_SPEED) {
    // This delegates to the full constructor - no additional code needed
}

bool SquareEnemy::CheckLineIntersectsPlayer(const sf::Vector2f& lineStart, const sf::Vector2f& lineEnd, const sf::RectangleShape& playerShape) {
    // Get player bounds
    sf::FloatRect playerBounds = playerShape.getGlobalBounds();
    
    // Get player center
    sf::Vector2f playerCenter = playerShape.getPosition();
    
    // Calculate the half width and half height of the player rectangle
    float halfWidth = playerBounds.width / 2.0f;
    float halfHeight = playerBounds.height / 2.0f;
    
    // Calculate the closest point on the line to the player center
    sf::Vector2f lineDir = lineEnd - lineStart;
    float lineLengthSquared = lineDir.x * lineDir.x + lineDir.y * lineDir.y;
    
    // Calculate the projection of player-lineStart onto the line
    sf::Vector2f toPlayer = playerCenter - lineStart;
    float dotProduct = (toPlayer.x * lineDir.x + toPlayer.y * lineDir.y) / lineLengthSquared;
    
    // Clamp to ensure the point is on the line segment
    dotProduct = std::max(0.0f, std::min(1.0f, dotProduct));
    
    // Find the closest point on the line
    sf::Vector2f closestPoint = lineStart + dotProduct * lineDir;
    
    // Adjust for rotation - convert closest point to player's local coordinate system
    float playerRotation = playerShape.getRotation() * 3.14159f / 180.0f; // Convert to radians
    
    // Vector from player center to closest point
    sf::Vector2f relativePoint = closestPoint - playerCenter;
    
    // Apply inverse rotation
    sf::Vector2f rotatedPoint;
    rotatedPoint.x = relativePoint.x * std::cos(-playerRotation) - relativePoint.y * std::sin(-playerRotation);
    rotatedPoint.y = relativePoint.x * std::sin(-playerRotation) + relativePoint.y * std::cos(-playerRotation);
    
    // Check if the rotated point is inside the rectangle's bounds
    return std::abs(rotatedPoint.x) <= halfWidth && std::abs(rotatedPoint.y) <= halfHeight;
}

void SquareEnemy::FindTarget(PlayerManager& playerManager) {
    // First check if any player intersects with our lines
    playerIntersectsLine = CheckPlayerIntersectsAnyLine(playerManager);
    
    // If a player intersects, we already have the axis and last intersection point
    // If not, find the closest player as our target (but we won't move to them)
    if (!playerIntersectsLine) {
        Enemy::FindTarget(playerManager);
    }
}

bool SquareEnemy::CheckPlayerIntersectsAnyLine(PlayerManager& playerManager) {
    const auto& players = playerManager.GetPlayers();
    
    for (const auto& pair : players) {
        // Skip dead players
        if (pair.second.player.IsDead()) continue;
        
         // Check each axis line
         for (int i = 0; i < axes.size(); i++) {
            // Line start is at the enemy position
            const sf::RectangleShape& playerShape = pair.second.player.GetShape();
            sf::Vector2f lineStart = position;
            // Line end is at lineLength distance along the axis
            sf::Vector2f lineEnd = position + axes[i] * lineLength;
            if (CheckLineIntersectsPlayer(lineStart, lineEnd, playerShape))  {
                // Record which axis the player is intersecting
                currentAxisIndex = i;
                // Record the intersection point (use player position for simplicity)
                lastIntersectionPoint = playerShape.getPosition();

                return true;
            }
        }
    }
    
    return false;
}

void SquareEnemy::InitializeAxes() {
    axes.clear();
    
    // Create four fixed axes at 90 degrees apart
    // Up
    axes.push_back(sf::Vector2f(0.0f, -1.0f));
    // Right
    axes.push_back(sf::Vector2f(1.0f, 0.0f));
    // Down
    axes.push_back(sf::Vector2f(0.0f, 1.0f));
    // Left
    axes.push_back(sf::Vector2f(-1.0f, 0.0f));
}

void SquareEnemy::UpdateAxes() {
    axes.clear();
    
    // Create four axes at 90 degrees apart
    for (int i = 0; i < 4; i++) {
        float angle = (rotationAngle + i * 90.0f) * 3.14159f / 180.0f; // Convert to radians
        axes.push_back(sf::Vector2f(std::cos(angle), std::sin(angle)));
    }
}

std::vector<sf::Vector2f> SquareEnemy::GetAxes() const {
    return axes;
}

void SquareEnemy::UpdateVisualRepresentation() {
    // Set position
    shape.setPosition(position);
    
    // Update rotation to match the rotation angle
    shape.setRotation(rotationAngle);
}

void SquareEnemy::UpdateMovement(float dt, PlayerManager& playerManager) {
    if (!hasTarget) return;
    
    // If player is currently intersecting any line
    if (playerIntersectsLine) {
        // Use the current axis to move toward the intersection point (player position)
        sf::Vector2f direction = lastIntersectionPoint - position;
        float distance = std::hypot(direction.x, direction.y);
        
        if (distance > 1.0f) {  // Avoid division by zero
            // Get the current axis
            sf::Vector2f moveAxis = axes[currentAxisIndex];
            
            // Make sure we're moving in the correct direction along the axis
            sf::Vector2f normalizedDir = direction / distance;
            float dotProduct = normalizedDir.x * moveAxis.x + normalizedDir.y * moveAxis.y;
            if (dotProduct < 0) {
                moveAxis = -moveAxis; // Flip the direction if needed
            }
            
            // Apply velocity along the chosen axis
            velocity = moveAxis * speed;
            position += velocity * dt;
        }
    } else {
        // No player is intersecting a line, so move toward the last intersection point
        sf::Vector2f direction = lastIntersectionPoint - position;
        float distance = std::hypot(direction.x, direction.y);
        
        if (distance > 1.0f) {  // Only move if we're not at the last intersection point
            // Find the axis that best aligns with the direction to the last intersection
            float bestDotProduct = -999999.0f;
            int bestAxisIndex = 0;
            
            for (int i = 0; i < 4; i++) {
                // Normalize direction
                sf::Vector2f normalizedDir = direction / distance;
                
                // Calculate dot product (measures alignment)
                float dotProduct = normalizedDir.x * axes[i].x + normalizedDir.y * axes[i].y;
                
                // We want the axis most aligned with our direction (highest dot product)
                if (dotProduct > bestDotProduct) {
                    bestDotProduct = dotProduct;
                    bestAxisIndex = i;
                }
            }
            
            // Use the best axis
            sf::Vector2f moveAxis = axes[bestAxisIndex];
            
            // Make sure we're moving in the correct direction along the axis
            if (bestDotProduct < 0) {
                moveAxis = -moveAxis; // Flip the direction if needed
            }
            
            // Apply velocity, but move slower when returning to last intersection
            velocity = moveAxis * (speed * 0.7f);
            position += velocity * dt;
        } else {
            // We've reached the last intersection point, stop moving
            velocity = sf::Vector2f(0.0f, 0.0f);
        }
    }
    
    
    
    // Update axes based on rotation
    UpdateAxes();
}

void SquareEnemy::Render(sf::RenderWindow& window) {
    if (!IsDead()) {
        // Draw the square
        window.draw(shape);
        
        // Draw the last intersection point if we're not currently intersecting
        /*if (!playerIntersectsLine) {
            sf::CircleShape intersectionPoint(5.0f);
            intersectionPoint.setFillColor(sf::Color::Red);
            intersectionPoint.setOrigin(5.0f, 5.0f);
            intersectionPoint.setPosition(lastIntersectionPoint);
            window.draw(intersectionPoint);
        }*/
    }
}