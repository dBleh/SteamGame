#include "PentagonEnemy.h"
#include "../player/PlayerManager.h"
#include <cmath>
#include <iostream>

PentagonEnemy::PentagonEnemy(int id, const sf::Vector2f& position, float health, float speed)
    : Enemy(id, position, health, speed),
      rotationAngle(0.0f),
      rotationSpeed(ENEMY_ROTATION_SPEED * 0.6f), // Slower rotation than square
      currentAxisIndex(0),
      playerIntersectsLine(false),
      lastIntersectionPoint(position),
      lineLength(350.0f) {
    
    // Setup the pentagon shape
    shape.setPointCount(5);
    
    // Calculate points for a regular pentagon
    const float radius = PENTAGON_SIZE / 2.0f;
    
    for (int i = 0; i < 5; ++i) {
        // 5 points around a circle, starting from the top
        float angle = (i * 2 * PI / 5) - (PI / 2); // Start from top
        sf::Vector2f point(
            radius * std::cos(angle),
            radius * std::sin(angle)
        );
        shape.setPoint(i, point);
    }
    
    // Set origin to center
    shape.setOrigin(0, 0);
    
    // Color
    shape.setFillColor(PENTAGON_FILL_COLOR);
    shape.setOutlineColor(PENTAGON_OUTLINE_COLOR);
    shape.setOutlineThickness(ENEMY_OUTLINE_THICKNESS);
    
    // Initialize axes (5 directions 72 degrees apart)
    InitializeAxes();
    
    // Update position to match the starting position
    UpdateVisualRepresentation();
}

// Constructor with default values for health and speed
PentagonEnemy::PentagonEnemy(int id, const sf::Vector2f& position)
    : PentagonEnemy(id, position, PENTAGON_HEALTH, ENEMY_SPEED) {
    // This delegates to the full constructor - no additional code needed
}

bool PentagonEnemy::CheckLineIntersectsPlayer(const sf::Vector2f& lineStart, const sf::Vector2f& lineEnd, const sf::RectangleShape& playerShape) {
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
void PentagonEnemy::FindTarget(PlayerManager& playerManager) {
    // First check if any player intersects with our lines
    playerIntersectsLine = CheckPlayerIntersectsAnyLine(playerManager);
    
    // If a player intersects, we already have the axis and last intersection point
    // If not, find the closest player as our target (but we won't move to them)
    if (!playerIntersectsLine) {
        Enemy::FindTarget(playerManager);
    }
}

bool PentagonEnemy::CheckPlayerIntersectsAnyLine(PlayerManager& playerManager) {
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

void PentagonEnemy::InitializeAxes() {
    axes.clear();
    
    // Create five fixed axes at 72 degrees apart
    
    for (int i = 0; i < 5; i++) {
        float angle = (i * 2 * PI / 5) - (PI / 2); // Start from top
        axes.push_back(sf::Vector2f(std::cos(angle), std::sin(angle)));
    }
}

void PentagonEnemy::UpdateAxes() {
    axes.clear();
    
    // Create five axes at 72 degrees apart, with rotation applied
    for (int i = 0; i < 5; i++) {
        float angle = rotationAngle * PI / 180.0f + (i * 2 * PI / 5) - (PI / 2);
        axes.push_back(sf::Vector2f(std::cos(angle), std::sin(angle)));
    }
}

std::vector<sf::Vector2f> PentagonEnemy::GetAxes() const {
    return axes;
}

void PentagonEnemy::UpdateVisualRepresentation() {
    // Set position
    shape.setPosition(position);
    
    // Update rotation to match the rotation angle
    shape.setRotation(rotationAngle);
}

void PentagonEnemy::UpdateMovement(float dt, PlayerManager& playerManager) {
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
            
            for (int i = 0; i < 5; i++) {
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

void PentagonEnemy::Render(sf::RenderWindow& window) {
    if (!IsDead()) {
        // Draw the pentagon
        window.draw(shape);
        
        // Draw the last intersection point if we're not currently intersecting
        if (!playerIntersectsLine) {
            sf::CircleShape intersectionPoint(5.0f);
            intersectionPoint.setFillColor(sf::Color::Red);
            intersectionPoint.setOrigin(5.0f, 5.0f);
            intersectionPoint.setPosition(lastIntersectionPoint);
            window.draw(intersectionPoint);
        }
    }
}