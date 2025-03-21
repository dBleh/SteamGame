#include "PentagonEnemy.h"
#include "../player/PlayerManager.h"
#include <cmath>
#include <iostream>
#include <algorithm>

PentagonEnemy::PentagonEnemy(int id, const sf::Vector2f& position, float health, float speed)
    : Enemy(id, position, health, speed),
      rotationAngle(0.0f),
      rotationSpeed(ENEMY_ROTATION_SPEED * 0.6f), // Slower rotation than square
      currentAxisIndex(0),
      playerIntersectsLine(false),
      lastTargetShape(nullptr),
      lastIntersectionPoint(position),
      lineLength(350.0f),
      currentBehavior(PentagonBehavior::Stalking),
      lastBehavior(PentagonBehavior::Stalking),
      behaviorTimer(0.0f),
      stateTransitionTimer(0.0f),
      targetPlayerDistance(999.0f),
      chargeEnergy(0.0f),
      maxChargeEnergy(3.0f),
      chargingUp(false),
      isCharging(false),
      isTeleporting(false),
      teleportProgress(0.0f),
      teleportDuration(0.5f),
      pulsePhase(0.0f),
      pulseFrequency(2.0f),
      pulseAmplitude(30.0f),
      pulseCount(0),
      maxPulseCount(5),
      currentFormationIndex(0),
      formationRadius(200.0f),
      formationAngle(0.0f) {
    
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
    
    // Generate the encircling formation positions
    GenerateEncirclingFormation();
    
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
    
    // Always find the closest player as our target for advanced behaviors
    Enemy::FindTarget(playerManager);
    
    // Calculate the distance to the target player if we have one
    if (hasTarget) {
        targetPlayerDistance = std::hypot(position.x - targetPosition.x, position.y - targetPosition.y);
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
                lastTargetShape = &playerShape;
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
    
    // Special visual updates for different behaviors
    switch (currentBehavior) {
        case PentagonBehavior::Charging:
            if (chargingUp) {
                // Pulsate size while charging
                float chargeRatio = chargeEnergy / maxChargeEnergy;
                float sizePulse = 1.0f + 0.2f * sin(behaviorTimer * 10.0f) * chargeRatio;
                
                // Adjust outline thickness and color based on charge
                shape.setOutlineThickness(ENEMY_OUTLINE_THICKNESS * (1.0f + chargeRatio));
                
                // Change color based on charge level
                sf::Color chargeColor = PENTAGON_FILL_COLOR;
                chargeColor.r = std::min(255, chargeColor.r + static_cast<int>(chargeRatio * 150));
                shape.setFillColor(chargeColor);
            } else if (isCharging) {
                // Stretched shape during charge
                float stretchFactor = 1.2f;
                
                // Calculate the direction to the target
                sf::Vector2f direction = targetPosition - position;
                float distance = std::hypot(direction.x, direction.y);
                
                if (distance > 0.1f) {
                    // Normalize direction
                    direction.x /= distance;
                    direction.y /= distance;
                    
                    // Calculate angle for stretching
                    float angle = atan2(direction.y, direction.x) * 180.0f / PI;
                    shape.setRotation(angle + 90.0f); // Point toward target
                }
            }
            break;
            
        case PentagonBehavior::Pulsating:
            // Pulsating size effect
            float pulseFactor = 1.0f + 0.3f * sin(pulsePhase);
            // During pulse peaks, make the outline more prominent
            if (pulseFactor > 1.2f) {
                shape.setOutlineThickness(ENEMY_OUTLINE_THICKNESS * 1.5f);
                
                sf::Color pulseColor = PENTAGON_OUTLINE_COLOR;
                pulseColor.g = std::min(255, pulseColor.g + 50);
                pulseColor.b = std::min(255, pulseColor.b + 50);
                shape.setOutlineColor(pulseColor);
            } else {
                shape.setOutlineThickness(ENEMY_OUTLINE_THICKNESS);
                shape.setOutlineColor(PENTAGON_OUTLINE_COLOR);
            }
            break;
    }
}

void PentagonEnemy::UpdateMovement(float dt, PlayerManager& playerManager) {
    if (!hasTarget) return;
    
    // Update timers
    behaviorTimer += dt;
    stateTransitionTimer += dt;
    rotationAngle += rotationSpeed * dt;
    
    // Wrap rotation angle
    if (rotationAngle > 360.0f) {
        rotationAngle -= 360.0f;
    }
    
    // Update afterimages
    for (auto& afterImage : afterImages) {
        afterImage.lifetime -= dt;
        afterImage.alpha = afterImage.lifetime; // Alpha fades with lifetime
    }
    
    // Remove expired afterimages
    afterImages.erase(
        std::remove_if(afterImages.begin(), afterImages.end(), 
                      [](const AfterImage& img) { return img.lifetime <= 0.0f; }),
        afterImages.end());
    
    // Update behavior state machine
    UpdateBehavior(dt);
    
    // Handle movement based on current behavior
    switch (currentBehavior) {
        case PentagonBehavior::Stalking:
            HandleStalkingBehavior(dt);
            break;
        case PentagonBehavior::Charging:
            HandleChargingBehavior(dt);
            break;
        case PentagonBehavior::Pulsating:
            HandlePulsatingBehavior(dt);
            break;
        case PentagonBehavior::Encircling:
            HandleEncirclingBehavior(dt);
            break;
        case PentagonBehavior::Teleporting:
            HandleTeleportingBehavior(dt);
            break;
    }
    
    // Update axes based on rotation
    UpdateAxes();
}

void PentagonEnemy::UpdateBehavior(float dt) {
    // Store previous behavior
    lastBehavior = currentBehavior;
    
    // Potential transitions based on current behavior and conditions
    switch (currentBehavior) {
        case PentagonBehavior::Stalking:
            // After stalking for a while, choose next behavior
            if (stateTransitionTimer > 4.0f) {
                stateTransitionTimer = 0.0f;
                
                // Choose next behavior based on distance to player
                if (targetPlayerDistance < 150.0f) {
                    // When close, either charge or teleport
                    if (rand() % 2 == 0) {
                        currentBehavior = PentagonBehavior::Charging;
                        chargingUp = true;
                        isCharging = false;
                        chargeEnergy = 0.0f;
                    } else {
                        currentBehavior = PentagonBehavior::Teleporting;
                        isTeleporting = true;
                        teleportProgress = 0.0f;
                        
                        // Choose teleport destination
                        float angle = (rand() % 360) * PI / 180.0f;
                        float distance = 200.0f + (rand() % 100);
                        teleportDestination = targetPosition + sf::Vector2f(cos(angle) * distance, sin(angle) * distance);
                        
                        // Add initial afterimage
                        AddAfterImage(teleportDuration * 2.0f);
                    }
                } else if (targetPlayerDistance < 300.0f) {
                    // At medium range, start pulsating or encircling
                    if (rand() % 2 == 0) {
                        currentBehavior = PentagonBehavior::Pulsating;
                        pulsePhase = 0.0f;
                        pulseCount = 0;
                    } else {
                        currentBehavior = PentagonBehavior::Encircling;
                        GenerateEncirclingFormation();
                        currentFormationIndex = 0;
                    }
                } else {
                    // Stay in stalking mode but reset timer
                    stateTransitionTimer = 0.0f;
                }
            }
            break;
            
        case PentagonBehavior::Charging:
            if (chargingUp) {
                // Charge up until reaching max energy
                if (chargeEnergy >= maxChargeEnergy) {
                    chargingUp = false;
                    isCharging = true;
                }
            } else if (isCharging) {
                // After charge completes, go back to stalking
                if (stateTransitionTimer > 2.0f || targetPlayerDistance > 400.0f) {
                    currentBehavior = PentagonBehavior::Stalking;
                    stateTransitionTimer = 0.0f;
                    isCharging = false;
                }
            }
            break;
            
        case PentagonBehavior::Pulsating:
            // After completing all pulses, choose next behavior
            if (pulseCount >= maxPulseCount) {
                if (targetPlayerDistance < 200.0f) {
                    currentBehavior = PentagonBehavior::Charging;
                    chargingUp = true;
                    isCharging = false;
                    chargeEnergy = 0.0f;
                    stateTransitionTimer = 0.0f;
                } else {
                    currentBehavior = PentagonBehavior::Stalking;
                    stateTransitionTimer = 0.0f;
                }
            }
            break;
            
        case PentagonBehavior::Encircling:
            // After completing the formation or if player moves too far, change behavior
            if (currentFormationIndex >= formationPositions.size() || targetPlayerDistance > 400.0f) {
                if (rand() % 2 == 0 && targetPlayerDistance < 250.0f) {
                    currentBehavior = PentagonBehavior::Teleporting;
                    isTeleporting = true;
                    teleportProgress = 0.0f;
                    
                    // Teleport to other side of player
                    sf::Vector2f playerToEnemy = position - targetPosition;
                    float distance = std::hypot(playerToEnemy.x, playerToEnemy.y);
                    if (distance > 0.1f) {
                        playerToEnemy = playerToEnemy / distance;
                        teleportDestination = targetPosition - playerToEnemy * 150.0f;
                    } else {
                        // Random angle if directly on top of player
                        float angle = (rand() % 360) * PI / 180.0f;
                        teleportDestination = targetPosition + sf::Vector2f(cos(angle) * 150.0f, sin(angle) * 150.0f);
                    }
                    
                    // Add afterimage
                    AddAfterImage(teleportDuration * 2.0f);
                } else {
                    currentBehavior = PentagonBehavior::Stalking;
                }
                stateTransitionTimer = 0.0f;
            }
            break;
            
        case PentagonBehavior::Teleporting:
            // After teleport completes, return to stalking or start charging
            if (!isTeleporting) {
                if (targetPlayerDistance < 150.0f && rand() % 2 == 0) {
                    currentBehavior = PentagonBehavior::Charging;
                    chargingUp = true;
                    isCharging = false;
                    chargeEnergy = 0.0f;
                } else {
                    currentBehavior = PentagonBehavior::Stalking;
                }
                stateTransitionTimer = 0.0f;
            }
            break;
    }
    
    // Reset state transition timer if behavior changed
    if (lastBehavior != currentBehavior) {
        stateTransitionTimer = 0.0f;
        behaviorTimer = 0.0f;
    }
}

void PentagonEnemy::HandleStalkingBehavior(float dt) {
    if (!hasTarget) return;
    
    // Calculate ideal stalking distance (farther than charging distance)
    float idealDistance = 200.0f;
    float distanceTolerance = 30.0f;
    
    // Calculate direction to player
    sf::Vector2f toPlayer = targetPosition - position;
    float distance = std::hypot(toPlayer.x, toPlayer.y);
    
    if (distance < 0.1f) return; // Avoid division by zero
    
    // Normalize direction
    sf::Vector2f normalizedDir = toPlayer / distance;
    
    // Determine movement behavior based on distance
    sf::Vector2f moveDirection;
    float speedMultiplier = 1.0f;
    
    if (distance < idealDistance - distanceTolerance) {
        // Too close, move away slightly
        moveDirection = -normalizedDir;
        speedMultiplier = 0.7f;
    } else if (distance > idealDistance + distanceTolerance) {
        // Too far, approach
        moveDirection = normalizedDir;
        speedMultiplier = 0.9f;
    } else {
        // In the sweet spot, circle around player
        moveDirection = sf::Vector2f(-normalizedDir.y, normalizedDir.x);
        speedMultiplier = 0.8f;
        
        // Occasionally reverse circling direction
        if (rand() % 100 < 1) {
            moveDirection = -moveDirection;
        }
    }
    
    // Move along pentagon axes
    sf::Vector2f moveVector = GetVectorAlongBestAxis(moveDirection, speedMultiplier);
    position += moveVector * dt;
    
    // Add slight wobble to create more organic movement
    float wobbleAmplitude = 5.0f;
    float wobbleFrequency = 2.0f;
    sf::Vector2f wobble(
        wobbleAmplitude * sin(behaviorTimer * wobbleFrequency),
        wobbleAmplitude * cos(behaviorTimer * wobbleFrequency * 1.3f)
    );
    
    position += wobble * dt;
}

void PentagonEnemy::HandleChargingBehavior(float dt) {
    if (!hasTarget) return;
    
    if (chargingUp) {
        // When charging up, stay in place but slightly pulse
        float pulseScale = 0.1f;
        float pulseFreq = 5.0f;
        
        sf::Vector2f pulse(
            sin(behaviorTimer * pulseFreq) * pulseScale,
            cos(behaviorTimer * pulseFreq * 1.2f) * pulseScale
        );
        
        position += pulse * dt * speed;
        
        // Increase charge energy
        chargeEnergy += dt * 0.8f; // Takes about 3.75 seconds to fully charge
        
    } else if (isCharging) {
        // During actual charge, move quickly toward player
        sf::Vector2f toPlayer = targetPosition - position;
        float distance = std::hypot(toPlayer.x, toPlayer.y);
        
        if (distance > 0.1f) {
            // Get normalized direction
            sf::Vector2f normalizedDir = toPlayer / distance;
            
            // Move along axes but with high speed
            sf::Vector2f moveVector = GetVectorAlongBestAxis(normalizedDir, 3.0f);
            position += moveVector * dt;
            
            // Add afterimages during charge
            if (stateTransitionTimer > 0.05f) {
                AddAfterImage(0.5f);
                stateTransitionTimer = 0.0f;
            }
        }
    }
}

void PentagonEnemy::HandlePulsatingBehavior(float dt) {
    // Update pulse phase
    pulsePhase += dt * pulseFrequency;
    
    // When we complete a full pulse cycle
    if (pulsePhase >= 2.0f * PI) {
        pulsePhase -= 2.0f * PI;
        pulseCount++;
        
        // If we hit a pulse peak, create a ripple effect
        if (sin(pulsePhase) > 0.9f) {
            // Add afterimage to represent the ripple
            AddAfterImage(0.8f);
        }
    }
    
    // During pulse peaks, move toward player
    if (sin(pulsePhase) > 0.7f && hasTarget) {
        sf::Vector2f toPlayer = targetPosition - position;
        float distance = std::hypot(toPlayer.x, toPlayer.y);
        
        if (distance > 0.1f) {
            sf::Vector2f normalizedDir = toPlayer / distance;
            
            // Quick movement forward during pulse
            sf::Vector2f moveVector = GetVectorAlongBestAxis(normalizedDir, 1.2f);
            position += moveVector * dt;
        }
    } else {
        // During pulse troughs, slight drifting movement
        float angle = behaviorTimer * 0.5f;
        sf::Vector2f driftDir(cos(angle), sin(angle));
        
        sf::Vector2f moveVector = GetVectorAlongBestAxis(driftDir, 0.3f);
        position += moveVector * dt;
    }
}

void PentagonEnemy::HandleEncirclingBehavior(float dt) {
    if (formationPositions.empty() || !hasTarget) {
        currentBehavior = PentagonBehavior::Stalking;
        return;
    }
    
    // Get current target position in the formation
    sf::Vector2f targetPos = formationPositions[currentFormationIndex];
    
    // Adjust target position relative to the player's current position
    sf::Vector2f adjustedTarget = targetPosition + (targetPos - targetPosition);
    
    // Calculate direction to the target position
    sf::Vector2f toTarget = adjustedTarget - position;
    float distance = std::hypot(toTarget.x, toTarget.y);
    
    // If we're close enough to the current position, move to the next one
    if (distance < 15.0f) {
        currentFormationIndex++;
        
        // If we completed the formation, add an afterimage for visual effect
        if (currentFormationIndex >= formationPositions.size() || 
            currentFormationIndex % 5 == 0) {
            AddAfterImage(0.8f);
        }
    } else if (distance > 0.1f) {
        // Move toward the next position in the formation
        sf::Vector2f normalizedDir = toTarget / distance;
        
        // Calculate move speed (faster initially, slower as we approach)
        float speedMultiplier = std::min(2.0f, 1.0f + distance / 100.0f);
        
        // Move along axes
        sf::Vector2f moveVector = GetVectorAlongBestAxis(normalizedDir, speedMultiplier);
        position += moveVector * dt;
    }
}

void PentagonEnemy::HandleTeleportingBehavior(float dt) {
    if (!isTeleporting) return;
    
    // Update teleport progress
    teleportProgress += dt / teleportDuration;
    
    if (teleportProgress >= 1.0f) {
        // Teleport complete
        position = teleportDestination;
        isTeleporting = false;
        
        // Add afterimage at the destination
        AddAfterImage(0.8f);
    } else {
        // During teleport, fade out at origin and fade in at destination
        if (teleportProgress < 0.5f) {
            // First half: fade out at current position
            // Just stay in place and let the rendering handle the fade
        } else {
            // Second half: fade in at destination
            // Set position to destination but let opacity be handled by rendering
            position = teleportDestination;
        }
    }
}

sf::Vector2f PentagonEnemy::GetBestAxisForDirection(const sf::Vector2f& direction, bool allowReverse) {
    // Find the axis that best aligns with the given direction
    float bestDotProduct = -999999.0f;
    int bestAxisIndex = 0;
    
    for (int i = 0; i < axes.size(); i++) {
        // Calculate dot product (measures alignment)
        float dotProduct = direction.x * axes[i].x + direction.y * axes[i].y;
        
        // If we're not allowing reverse movement and dot product is negative, skip
        if (!allowReverse && dotProduct < 0) {
            continue;
        }
        
        // We want the axis most aligned with our direction (highest absolute dot product)
        if (std::abs(dotProduct) > std::abs(bestDotProduct)) {
            bestDotProduct = dotProduct;
            bestAxisIndex = i;
        }
    }
    
    // Return the best matching axis
    sf::Vector2f moveAxis = axes[bestAxisIndex];
    
    // If dot product is negative and we allow reverse, flip the direction
    if (bestDotProduct < 0 && allowReverse) {
        moveAxis = -moveAxis;
    }
    
    return moveAxis;
}

sf::Vector2f PentagonEnemy::GetVectorAlongBestAxis(const sf::Vector2f& direction, float speedMultiplier) {
    // Find the best axis for this direction
    sf::Vector2f moveAxis = GetBestAxisForDirection(direction);
    
    // Apply velocity with speed multiplier
    return moveAxis * speed * speedMultiplier;
}

void PentagonEnemy::GenerateEncirclingFormation() {
    formationPositions.clear();
    
    // Create a pentagon pattern of positions around the target
    int numPoints = 25; // 5 points per side of pentagon, total of 25 points
    
    // Base pentagon shape
    for (int segment = 0; segment < 5; segment++) {
        float startAngle = segment * 2.0f * PI / 5.0f;
        float endAngle = ((segment + 1) % 5) * 2.0f * PI / 5.0f;
        
        // Create 5 points along each segment
        for (int i = 0; i < 5; i++) {
            float t = i / 4.0f; // Normalized position along segment
            float angle = startAngle + t * (endAngle - startAngle);
            
            // Calculate position
            float x = formationRadius * std::cos(angle);
            float y = formationRadius * std::sin(angle);
            
            // Store this position
            formationPositions.push_back(sf::Vector2f(x, y));
        }
    }
    
    // Shuffle the formation positions to create more unpredictable movement
    // Use a simple swap-based shuffle
    for (int i = 0; i < formationPositions.size(); i++) {
        int j = rand() % formationPositions.size();
        std::swap(formationPositions[i], formationPositions[j]);
    }
}

void PentagonEnemy::ClearAfterImages() {
    afterImages.clear();
}

void PentagonEnemy::AddAfterImage(float lifetime) {
    // Create a new afterimage at the current position
    AfterImage image;
    image.position = position;
    image.lifetime = lifetime;
    image.alpha = 1.0f;
    
    // Add to the collection
    afterImages.push_back(image);
    
    // Limit the number of afterimages
    if (afterImages.size() > 10) {
        afterImages.pop_front();
    }
}

void PentagonEnemy::Render(sf::RenderWindow& window) {
    if (IsDead()) return;
    
    // First render any afterimages
    for (const auto& afterImage : afterImages) {
        // Create a copy of the shape for the afterimage
        sf::ConvexShape afterImageShape = shape;
        afterImageShape.setPosition(afterImage.position);
        
        // Set color with transparency based on lifetime
        sf::Color color = shape.getFillColor();
        color.a = static_cast<sf::Uint8>(255 * afterImage.alpha * 0.5f); // 50% max opacity
        afterImageShape.setFillColor(color);
        
        sf::Color outlineColor = shape.getOutlineColor();
        outlineColor.a = static_cast<sf::Uint8>(255 * afterImage.alpha * 0.7f); // 70% max opacity
        afterImageShape.setOutlineColor(outlineColor);
        
        // Draw the afterimage
        window.draw(afterImageShape);
    }
    
    // Don't render the main shape during teleportation fade-out
    if (isTeleporting && teleportProgress < 0.5f) {
        // Create a fading version of the shape
        sf::ConvexShape fadingShape = shape;
        sf::Color fillColor = shape.getFillColor();
        sf::Color outlineColor = shape.getOutlineColor();
        
        // Calculate fade based on teleport progress
        float fadeAlpha = 1.0f - (teleportProgress * 2.0f); // 1.0 to 0.0 in first half
        
        fillColor.a = static_cast<sf::Uint8>(255 * fadeAlpha);
        outlineColor.a = static_cast<sf::Uint8>(255 * fadeAlpha);
        
        fadingShape.setFillColor(fillColor);
        fadingShape.setOutlineColor(outlineColor);
        
        window.draw(fadingShape);
    } else if (isTeleporting && teleportProgress >= 0.5f) {
        // Create a fading-in version of the shape
        sf::ConvexShape fadingShape = shape;
        sf::Color fillColor = shape.getFillColor();
        sf::Color outlineColor = shape.getOutlineColor();
        
        // Calculate fade based on teleport progress
        float fadeAlpha = (teleportProgress - 0.5f) * 2.0f; // 0.0 to 1.0 in second half
        
        fillColor.a = static_cast<sf::Uint8>(255 * fadeAlpha);
        outlineColor.a = static_cast<sf::Uint8>(255 * fadeAlpha);
        
        fadingShape.setFillColor(fillColor);
        fadingShape.setOutlineColor(outlineColor);
        
        window.draw(fadingShape);
    } else {
        // Normal rendering
        window.draw(shape);
    }
    
    // Debug visualization: Draw the five axis lines
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
    
    // Debug visualization: Draw encircling formation points
    /*if (currentBehavior == PentagonBehavior::Encircling && hasTarget) {
        for (int i = 0; i < formationPositions.size(); i++) {
            sf::CircleShape point(3.0f);
            point.setFillColor(i == currentFormationIndex ? sf::Color::Red : sf::Color::Yellow);
            point.setOrigin(3.0f, 3.0f);
            point.setPosition(targetPosition + formationPositions[i]);
            window.draw(point);
        }
    }*/
}