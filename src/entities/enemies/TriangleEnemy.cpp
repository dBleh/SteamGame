#include "TriangleEnemy.h"
#include "../player/PlayerManager.h"
#include <cmath>
#include <iostream>

TriangleEnemy::TriangleEnemy(int id, const sf::Vector2f& position, float health, float speed)
    : Enemy(id, position, health, speed), 
      rotationAngle(0.0f), 
      rotationSpeed(ENEMY_ROTATION_SPEED),
      currentAxisIndex(0),
      playerIntersectsLine(false),
      lastIntersectionPoint(position),
      lineLength(300.0f) {
    
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
    
    // Initialize axes (3 directions 120 degrees apart)
    InitializeAxes();
    // Update position to match the starting position
    UpdateVisualRepresentation();
}
bool TriangleEnemy::CheckLineIntersectsPlayer(const sf::Vector2f& lineStart, const sf::Vector2f& lineEnd, const sf::Vector2f& playerPos) {
    // Player radius (approximate for collision)
    const float playerRadius = 15.0f; 
    
    // Calculate the closest point on the line to the player
    sf::Vector2f lineDir = lineEnd - lineStart;
    float lineLengthSquared = lineDir.x * lineDir.x + lineDir.y * lineDir.y;
    
    // Calculate the projection of player-lineStart onto the line
    sf::Vector2f toPlayer = playerPos - lineStart;
    float dotProduct = (toPlayer.x * lineDir.x + toPlayer.y * lineDir.y) / lineLengthSquared;
    
    // Clamp to ensure the point is on the line segment
    dotProduct = std::max(0.0f, std::min(1.0f, dotProduct));
    
    // Find the closest point on the line
    sf::Vector2f closestPoint = lineStart + dotProduct * lineDir;
    
    // Check if the distance from player to the closest point is less than the player radius
    sf::Vector2f distanceVec = playerPos - closestPoint;
    float distanceSquared = distanceVec.x * distanceVec.x + distanceVec.y * distanceVec.y;
    
    return distanceSquared < (playerRadius * playerRadius);
}
void TriangleEnemy::FindTarget(PlayerManager& playerManager) {
    // First check if any player intersects with our lines
    playerIntersectsLine = CheckPlayerIntersectsAnyLine(playerManager);
    
    // If a player intersects, we already have the axis and last intersection point
    // If not, find the closest player as our target (but we won't move to them)
    if (!playerIntersectsLine) {
        Enemy::FindTarget(playerManager);
    }
}
bool TriangleEnemy::CheckPlayerIntersectsAnyLine(PlayerManager& playerManager) {
    const auto& players = playerManager.GetPlayers();
    
    for (const auto& pair : players) {
        // Skip dead players
        if (pair.second.player.IsDead()) continue;
        
        sf::Vector2f playerPos = pair.second.player.GetPosition();
        
        // Check each axis line
        for (int i = 0; i < axes.size(); i++) {
            // Line start is at the enemy position
            sf::Vector2f lineStart = position;
            // Line end is at lineLength distance along the axis
            sf::Vector2f lineEnd = position + axes[i] * lineLength;
            
            if (CheckLineIntersectsPlayer(lineStart, lineEnd, playerPos)) {
                // Record which axis the player is intersecting
                currentAxisIndex = i;
                // Record the intersection point (use player position for simplicity)
                lastIntersectionPoint = playerPos;
                return true;
            }
        }
    }
    
    return false;
}
void TriangleEnemy::InitializeAxes() {
    axes.clear();
    
    // Create three fixed axes at 120 degrees apart
    // Up
    axes.push_back(sf::Vector2f(0.0f, -1.0f));
    // Bottom-right (60 degrees)
    axes.push_back(sf::Vector2f(0.866f, 0.5f));
    // Bottom-left (120 degrees)
    axes.push_back(sf::Vector2f(-0.866f, 0.5f));
}

void TriangleEnemy::UpdateAxes() {
    axes.clear();
    
    // Create three axes at 120 degrees apart
    for (int i = 0; i < 3; i++) {
        float angle = (rotationAngle + i * 120.0f) * 3.14159f / 180.0f; // Convert to radians
        axes.push_back(sf::Vector2f(std::cos(angle), std::sin(angle)));
    }
}

std::vector<sf::Vector2f> TriangleEnemy::GetAxes() const {
    return axes;
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
            
            for (int i = 0; i < 3; i++) {
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
            velocity = moveAxis * (speed * 0.6f);
            position += velocity * dt;
        } else {
            // We've reached the last intersection point, stop moving
            velocity = sf::Vector2f(0.0f, 0.0f);
        }
    }
    
    // Continue with normal rotation update
    rotationAngle += rotationSpeed * dt;
    if (rotationAngle > 360.0f) {
        rotationAngle -= 360.0f;
    }
}

void TriangleEnemy::Render(sf::RenderWindow& window) {
    if (!IsDead()) {
        // Draw the triangle
        window.draw(shape);
        
        // Draw the three axis lines
        /*for (int i = 0; i < axes.size(); i++) {
            const auto& axis = axes[i];
            
            // Line color: green if player is on this line, yellow otherwise
            sf::Color lineColor = (playerIntersectsLine && i == currentAxisIndex) ? sf::Color::Green : sf::Color::Yellow;
            
            sf::Vertex line[] = {
                sf::Vertex(position, lineColor),
                sf::Vertex(position + axis * lineLength, lineColor)
            };
            window.draw(line, 2, sf::Lines);
        }*/
        
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