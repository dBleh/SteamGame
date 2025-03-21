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
      lastTargetShape(nullptr),
      lineLength(320.0f),
      flyByTimer(0.0f),
      flyByActive(false),
      flyByDuration(1.2f),  // Duration of a fly-by attack
      flyBySpeedMultiplier(2.5f),  // Speed boost during fly-by
      orbitDistance(180.0f),  // Distance to orbit around player
      orbitSpeedMultiplier(1.5f),  // Orbit speed multiplier
      movementPhase(MovementPhase::Seeking),
      phaseTimer(0.0f),
      targetPlayerDistance(0.0f),
      lastState(MovementPhase::Seeking),
      directionChangeTimer(0.0f) {
    
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
    
    // Find closest player as our target regardless if they're intersecting a line
    Enemy::FindTarget(playerManager);
    
    // Calculate the distance to the target player
    if (hasTarget) {
        targetPlayerDistance = std::hypot(position.x - targetPosition.x, position.y - targetPosition.y);
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
            if (CheckLineIntersectsPlayer(lineStart, lineEnd, playerShape)) {
                // Record which axis the player is intersecting
                currentAxisIndex = i;
                // Store the player shape for position updates
                lastTargetShape = &playerShape;
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

sf::Vector2f SquareEnemy::FindClosestPointOnRect(const sf::RectangleShape& rect) {
    if (!lastTargetShape) {
        return lastIntersectionPoint; // Fallback if no target shape
    }
    
    // Get the player's position (center of the rectangle)
    sf::Vector2f rectCenter = rect.getPosition();
    
    // Get the bounds of the rectangle
    sf::FloatRect bounds = rect.getGlobalBounds();
    float halfWidth = bounds.width / 2.0f;
    float halfHeight = bounds.height / 2.0f;
    
    // Calculate the direction vector from the enemy to the rectangle center
    sf::Vector2f direction = rectCenter - position;
    float distance = std::hypot(direction.x, direction.y);
    
    if (distance < 1.0f) {
        return rectCenter; // Avoid division by zero
    }
    
    // Normalize the direction vector
    sf::Vector2f normalizedDir = direction / distance;
    
    // Calculate the rotation of the rectangle in radians
    float rotation = rect.getRotation() * 3.14159f / 180.0f;
    
    // Create rotation matrix
    sf::Vector2f right(std::cos(rotation), std::sin(rotation));
    sf::Vector2f up(-std::sin(rotation), std::cos(rotation));
    
    // Project the direction onto the rectangle's axes
    float dotRight = normalizedDir.x * right.x + normalizedDir.y * right.y;
    float dotUp = normalizedDir.x * up.x + normalizedDir.y * up.y;
    
    // Clamp the projections to the rectangle's dimensions
    float clampedDotRight = std::max(-halfWidth, std::min(halfWidth, dotRight * distance));
    float clampedDotUp = std::max(-halfHeight, std::min(halfHeight, dotUp * distance));
    
    // Calculate the point on the rectangle in world space
    sf::Vector2f closestPoint = rectCenter + right * clampedDotRight + up * clampedDotUp;
    
    return closestPoint;
}

void SquareEnemy::UpdateMovement(float dt, PlayerManager& playerManager) {
    if (!hasTarget) return;
    
    // Update timers
    flyByTimer += dt;
    phaseTimer += dt;
    directionChangeTimer += dt;
    
    // Update rotation regardless of movement phase
    rotationAngle += rotationSpeed * dt;
    if (rotationAngle > 360.0f) {
        rotationAngle -= 360.0f;
    }
    
    // Update axes based on rotation
    UpdateAxes();
    
    // Determine if we should change movement phase
    UpdateMovementPhase(dt);
    
    // Handle movement based on the current phase
    switch (movementPhase) {
        case MovementPhase::Seeking:
            HandleSeekingMovement(dt);
            break;
        case MovementPhase::FlyBy:
            HandleFlyByMovement(dt);
            break;
        case MovementPhase::Orbiting:
            HandleOrbitingMovement(dt);
            break;
        case MovementPhase::Retreating:
            HandleRetreatMovement(dt);
            break;
    }
}

void SquareEnemy::UpdateMovementPhase(float dt) {
    // Store previous state before updating
    lastState = movementPhase;
    
    // Potential phase transitions based on conditions
    switch (movementPhase) {
        case MovementPhase::Seeking:
            // If we get close enough to the player, start a fly-by
            if (targetPlayerDistance < 200.0f && phaseTimer > 1.0f) {
                movementPhase = MovementPhase::FlyBy;
                phaseTimer = 0.0f;
                flyByTimer = 0.0f;
                flyByActive = true;
                // Store current direction for the fly-by
                flyByDirection = targetPosition - position;
                float distance = std::hypot(flyByDirection.x, flyByDirection.y);
                if (distance > 0.1f) {
                    flyByDirection = sf::Vector2f(flyByDirection.x / distance, flyByDirection.y / distance);
                }
            }
            break;
            
        case MovementPhase::FlyBy:
            // After the fly-by duration, switch to orbiting or retreating
            if (phaseTimer > flyByDuration) {
                if (targetPlayerDistance < 150.0f && rand() % 2 == 0) { // 50% chance
                    movementPhase = MovementPhase::Orbiting;
                } else {
                    movementPhase = MovementPhase::Retreating;
                }
                phaseTimer = 0.0f;
                flyByActive = false;
            }
            break;
            
        case MovementPhase::Orbiting:
            // After orbiting for a while, go back to seeking or do another fly-by
            if (phaseTimer > 3.0f) {
                if (targetPlayerDistance < 150.0f && rand() % 3 == 0) { // 33% chance
                    movementPhase = MovementPhase::FlyBy;
                    flyByTimer = 0.0f;
                    flyByActive = true;
                    flyByDirection = targetPosition - position;
                    float distance = std::hypot(flyByDirection.x, flyByDirection.y);
                    if (distance > 0.1f) {
                        flyByDirection = sf::Vector2f(flyByDirection.x / distance, flyByDirection.y / distance);
                    }
                } else {
                    movementPhase = MovementPhase::Seeking;
                }
                phaseTimer = 0.0f;
            }
            break;
            
        case MovementPhase::Retreating:
            // After retreating for a while, go back to seeking
            if (phaseTimer > 2.0f || targetPlayerDistance > 350.0f) {
                movementPhase = MovementPhase::Seeking;
                phaseTimer = 0.0f;
            }
            break;
    }
    
    // Reset directionChangeTimer if we changed state
    if (lastState != movementPhase) {
        directionChangeTimer = 0.0f;
    }
}

void SquareEnemy::HandleSeekingMovement(float dt) {
    if (!hasTarget) return;
    
    // Pick the axis that best aligns with the direction to the target
    sf::Vector2f direction = targetPosition - position;
    float distance = std::hypot(direction.x, direction.y);
    
    if (distance > 1.0f) {
        // Find the axis that best aligns with the direction to the target
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
        
        // Apply velocity with a slight pulsing effect
        float pulseMultiplier = 1.0f + 0.2f * sin(phaseTimer * 3.0f);
        velocity = moveAxis * (speed * pulseMultiplier);
        position += velocity * dt;
    }
}

void SquareEnemy::HandleFlyByMovement(float dt) {
    // During a fly-by, we move in a straight line at high speed
    if (flyByActive) {
        // Calculate position where we'd end up if we continued in this direction
        sf::Vector2f potentialEndPoint = position + flyByDirection * (speed * flyBySpeedMultiplier) * (flyByDuration - flyByTimer);
        
        // If we've gone too far from the target or are about to, start decelerating
        float distanceToTarget = std::hypot(targetPosition.x - position.x, targetPosition.y - position.y);
        float distanceToEnd = std::hypot(targetPosition.x - potentialEndPoint.x, targetPosition.y - potentialEndPoint.y);
        
        float speedMultiplier = flyBySpeedMultiplier;
        
        // Decelerate if we're getting too far from the target
        if (distanceToEnd > 300.0f || flyByTimer > flyByDuration * 0.7f) {
            speedMultiplier *= 0.5f; // Slow down
        }
        
        // Apply velocity
        velocity = flyByDirection * speed * speedMultiplier;
        position += velocity * dt;
        
        // Add a slight curve to the path
        sf::Vector2f perpendicular(-flyByDirection.y, flyByDirection.x);
        position += perpendicular * sin(flyByTimer * 5.0f) * 2.0f * dt;
    }
}

void SquareEnemy::HandleOrbitingMovement(float dt) {
    if (!hasTarget) return;
    
    // Calculate the vector from the target to the enemy
    sf::Vector2f toEnemy = position - targetPosition;
    float distance = std::hypot(toEnemy.x, toEnemy.y);
    
    if (distance < 0.1f) return; // Avoid division by zero
    
    // Normalize the vector
    sf::Vector2f normalizedToEnemy = toEnemy / distance;
    
    // Calculate the perpendicular vector (counterclockwise)
    sf::Vector2f perpendicular(-normalizedToEnemy.y, normalizedToEnemy.x);
    
    // Calculate the target orbit distance
    float orbitFactor = 1.0f;
    if (distance > orbitDistance) {
        // If we're too far away, move closer
        orbitFactor = 0.8f;
    } else if (distance < orbitDistance * 0.8f) {
        // If we're too close, move away
        orbitFactor = 1.2f;
    }
    
    // Calculate the direction to move (mostly perpendicular with a component toward/away from target)
    sf::Vector2f moveDirection = perpendicular * 0.8f + normalizedToEnemy * (orbitFactor - 1.0f);
    
    // Normalize the move direction
    float moveLength = std::hypot(moveDirection.x, moveDirection.y);
    if (moveLength > 0.1f) {
        moveDirection = moveDirection / moveLength;
    }
    
    // Find the best axis for this direction
    float bestDotProduct = -999999.0f;
    int bestAxisIndex = 0;
    
    for (int i = 0; i < 4; i++) {
        float dotProduct = moveDirection.x * axes[i].x + moveDirection.y * axes[i].y;
        if (dotProduct > bestDotProduct) {
            bestDotProduct = dotProduct;
            bestAxisIndex = i;
        }
    }
    
    // Use the best axis, flip if needed
    sf::Vector2f moveAxis = axes[bestAxisIndex];
    if (bestDotProduct < 0) {
        moveAxis = -moveAxis;
    }
    
    // Apply velocity with orbit speed multiplier
    velocity = moveAxis * (speed * orbitSpeedMultiplier);
    position += velocity * dt;
    
    // Occasionally reverse the orbit direction
    if (directionChangeTimer > 2.0f && rand() % 10 == 0) {
        orbitSpeedMultiplier = -orbitSpeedMultiplier;
        directionChangeTimer = 0.0f;
    }
}

void SquareEnemy::HandleRetreatMovement(float dt) {
    if (!hasTarget) return;
    
    // Calculate direction away from the target
    sf::Vector2f direction = position - targetPosition;
    float distance = std::hypot(direction.x, direction.y);
    
    if (distance > 1.0f) {
        // Find the axis that best aligns with the direction away from the target
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
        
        // Apply velocity, retreat a bit faster than normal movement
        velocity = moveAxis * (speed * 1.2f);
        position += velocity * dt;
        
        // Add some zigzagging during retreat
        if (directionChangeTimer > 0.5f) {
            directionChangeTimer = 0.0f;
            
            // Choose a perpendicular axis for zigzagging
            int zigzagAxis = (bestAxisIndex + 1) % 4; // Choose a perpendicular axis
            sf::Vector2f zigzagDirection = axes[zigzagAxis];
            
            // Apply a zigzag movement
            position += zigzagDirection * (10.0f * (rand() % 2 == 0 ? 1.0f : -1.0f));
        }
    }
}

void SquareEnemy::Render(sf::RenderWindow& window) {
    if (!IsDead()) {
        // Draw the square
        window.draw(shape);
        
        // Debug visualization: Draw the four axis lines
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
    }
}