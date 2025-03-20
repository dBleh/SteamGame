#include "ForceField.h"
#include "Player.h"
#include "PlayerManager.h"
#include "../enemies/EnemyManager.h"
#include "../enemies/Enemy.h"
#include "../../core/Game.h"
#include <cmath>
#include <iostream>
#include <random>
#include <array>

// Enhanced constructor with more visual flair and gameplay options
ForceField::ForceField(Player* player, float radius)
    : player(player),
      radius(radius),
      zapTimer(0.0f),
      zapCooldown(DEFAULT_COOLDOWN),
      zapDamage(DEFAULT_DAMAGE),
      targetEnemyId(-1),
      isZapping(false),
      zapEffectDuration(FIELD_ZAP_EFFECT_DURATION),
      zapEffectTimer(0.0f),
      fieldRotation(0.0f),
      fieldPulsePhase(0.0f),
      fieldIntensity(FIELD_INTENSITY_DEFAULT),
      chargeLevel(0.0f),
      powerLevel(MAX_POWER_LEVEL),
      consecutiveHits(0),
      comboTimer(0.0f),
      fieldColor(FIELD_STANDARD_COLOR),
      chainLightningEnabled(true),
      chainLightningTargets(FIELD_DEFAULT_CHAIN_TARGETS),
      fieldType(FieldType::STANDARD), 
      zapCallback(nullptr) {
    
    // Randomly select a field type based on predefined probabilities
    const int randomFieldType = rand() % 100;
    if (randomFieldType < FIELD_TYPE_SHOCK_PROB) {
        fieldType = FieldType::SHOCK;
        fieldColor = FIELD_SHOCK_COLOR;
    } else if (randomFieldType < FIELD_TYPE_PLASMA_PROB) {
        fieldType = FieldType::PLASMA;
        fieldColor = FIELD_PLASMA_COLOR;
    } else if (randomFieldType < FIELD_TYPE_VORTEX_PROB) {
        fieldType = FieldType::VORTEX;
        fieldColor = FIELD_VORTEX_COLOR;
    } else {
        // For the standard type, use the standard color
        fieldColor = FIELD_STANDARD_COLOR;
    }
    
    // Setup field shape with dynamic effects
    fieldShape.setRadius(radius);
    fieldShape.setFillColor(fieldColor);
    
    // Set outline color based on field type
    sf::Color outlineColor;
    switch(fieldType) {
        case FieldType::SHOCK:
            outlineColor = FIELD_SHOCK_OUTLINE_COLOR;
            break;
        case FieldType::PLASMA:
            outlineColor = FIELD_PLASMA_OUTLINE_COLOR;
            break;
        case FieldType::VORTEX:
            outlineColor = FIELD_VORTEX_OUTLINE_COLOR;
            break;
        default:
            outlineColor = FIELD_STANDARD_OUTLINE_COLOR;
    }
    
    fieldShape.setOutlineColor(outlineColor);
    fieldShape.setOutlineThickness(FIELD_OUTLINE_THICKNESS);
    fieldShape.setOrigin(radius, radius);
    
    // Setup secondary field effects with colorful variations
    for (int i = 0; i < NUM_FIELD_RINGS; i++) {
        float ringRadius = radius * (FIELD_RING_INNER_RADIUS_FACTOR + FIELD_RING_RADIUS_INCREMENT * i);
        fieldRings[i].setRadius(ringRadius);
        fieldRings[i].setFillColor(sf::Color::Transparent);
        
        // Create colorful ring variations based on field type
        sf::Color ringColor;
        switch(fieldType) {
            case FieldType::SHOCK:
                ringColor = SHOCK_RING_COLOR(i);
                break;
            case FieldType::PLASMA:
                ringColor = PLASMA_RING_COLOR(i);
                break;
            case FieldType::VORTEX:
                ringColor = VORTEX_RING_COLOR(i);
                break;
            default:
                ringColor = STANDARD_RING_COLOR(i);
        }
        
        fieldRings[i].setOutlineColor(ringColor);
        fieldRings[i].setOutlineThickness(FIELD_RING_MIN_THICKNESS + (NUM_FIELD_RINGS - i) * FIELD_RING_THICKNESS_DECREMENT);
        fieldRings[i].setOrigin(ringRadius, ringRadius);
    }
    
    // Setup energy orbs that orbit the force field
    for (int i = 0; i < NUM_ENERGY_ORBS; i++) {
        float orbSize = ENERGY_ORB_MIN_SIZE + (rand() % (int)ENERGY_ORB_SIZE_VARIATION);
        energyOrbs[i].setRadius(orbSize);
        
        // Create varied orb colors based on position in sequence and field type
        sf::Color orbColor;
        int orbGroup = i % ENERGY_ORB_COLOR_GROUPS;
        
        switch(fieldType) {
            case FieldType::SHOCK:
                if (orbGroup == 0)
                    orbColor = SHOCK_ORB_COLOR_1;
                else if (orbGroup == 1)
                    orbColor = SHOCK_ORB_COLOR_2;
                else
                    orbColor = SHOCK_ORB_COLOR_3;
                break;
                
            case FieldType::PLASMA:
                if (orbGroup == 0)
                    orbColor = PLASMA_ORB_COLOR_1;
                else if (orbGroup == 1)
                    orbColor = PLASMA_ORB_COLOR_2;
                else
                    orbColor = PLASMA_ORB_COLOR_3;
                break;
                
            case FieldType::VORTEX:
                if (orbGroup == 0)
                    orbColor = VORTEX_ORB_COLOR_1;
                else if (orbGroup == 1)
                    orbColor = VORTEX_ORB_COLOR_2;
                else
                    orbColor = VORTEX_ORB_COLOR_3;
                break;
                
            default:
                if (orbGroup == 0)
                    orbColor = STANDARD_ORB_COLOR_1;
                else if (orbGroup == 1)
                    orbColor = STANDARD_ORB_COLOR_2;
                else
                    orbColor = STANDARD_ORB_COLOR_3;
        }
        
        energyOrbs[i].setFillColor(orbColor);
        energyOrbs[i].setOrigin(orbSize, orbSize);
        
        // Set varied orbit parameters
        orbAngles[i] = (float)(rand() % 360);
        orbSpeeds[i] = ENERGY_ORB_MIN_SPEED + (rand() % 100) / 50.0f;
        
        // Create layered orbit distances
        if (i < NUM_ENERGY_ORBS / 3)
            orbDistances[i] = radius * ENERGY_ORB_INNER_ORBIT_FACTOR + (rand() % (int)ENERGY_ORB_DISTANCE_VARIATION);
        else if (i < 2 * NUM_ENERGY_ORBS / 3)
            orbDistances[i] = radius * ENERGY_ORB_MIDDLE_ORBIT_FACTOR + (rand() % (int)ENERGY_ORB_DISTANCE_VARIATION);
        else
            orbDistances[i] = radius * ENERGY_ORB_OUTER_ORBIT_FACTOR + (rand() % (int)ENERGY_ORB_DISTANCE_VARIATION);
    }
    
    // Setup zap effect as a line strip
    zapEffect.setPrimitiveType(sf::Lines);
    zapEffect.resize(0);
    
    // Setup chain lightning effects
    chainEffect.setPrimitiveType(sf::Lines);
    chainEffect.resize(0);
    
    // Initialize particles system
    initializeParticles();
    
    // Fire initial zap quickly
    zapTimer = INITIAL_ZAP_TIMER;
    
    // Initialize with correct field color based on type
    updateFieldColor();
}

void ForceField::Update(float dt, PlayerManager& playerManager, EnemyManager& enemyManager) {
    // Skip if player is dead
    if (player->IsDead()) {
        isZapping = false;
        return;
    }
    
    // Update the force field position to follow the player
    sf::Vector2f playerCenter = player->GetPosition() + sf::Vector2f(25.0f, 25.0f);
    fieldShape.setPosition(playerCenter);
    
    // Update field rotation for dynamic effect
    fieldRotation += dt * FIELD_ROTATION_SPEED * powf(fieldIntensity, 0.5f); // Rotation speed increases with intensity
    fieldPulsePhase += dt * FIELD_PULSE_SPEED;
    
    // Update field rings
    for (int i = 0; i < NUM_FIELD_RINGS; i++) {
        fieldRings[i].setPosition(playerCenter);
        fieldRings[i].setRotation(fieldRotation * (i % 2 == 0 ? 1 : -1)); // Alternate rotation directions
        
        // Pulsing opacity based on field intensity
        float ringAlpha = 70.0f + 30.0f * std::sin(fieldPulsePhase + i * 0.5f);
        sf::Color ringColor = fieldRings[i].getOutlineColor();
        ringColor.a = static_cast<sf::Uint8>(ringAlpha * fieldIntensity);
        fieldRings[i].setOutlineColor(ringColor);
        
        // Dynamic scaling based on power level
        float scaleFactor = 1.0f + 0.05f * std::sin(fieldPulsePhase * 1.5f + i * 0.7f);
        float baseRadius = radius * (0.4f + 0.2f * i) * (1.0f + 0.1f * (powerLevel - 1));
        fieldRings[i].setRadius(baseRadius * scaleFactor);
        fieldRings[i].setOrigin(baseRadius * scaleFactor, baseRadius * scaleFactor);
    }
    
    // Update energy orbs - they orbit the player
    for (int i = 0; i < NUM_ENERGY_ORBS; i++) {
        // Update orbit angle
        orbAngles[i] += dt * orbSpeeds[i] * 60.0f * (fieldIntensity * 0.5f + 0.5f);
        
        // Calculate position
        float orbX = playerCenter.x + std::cos(orbAngles[i] * 3.14159f / 180.0f) * orbDistances[i];
        float orbY = playerCenter.y + std::sin(orbAngles[i] * 3.14159f / 180.0f) * orbDistances[i];
        energyOrbs[i].setPosition(orbX, orbY);
        
        // Pulsing size and opacity based on field intensity
        float sizePulse = 1.0f + 0.3f * std::sin(fieldPulsePhase * 2.0f + i * 0.9f);
        float baseSize = (5.0f + (i % 5)) * (1.0f + 0.1f * (powerLevel - 1));
        energyOrbs[i].setRadius(baseSize * sizePulse);
        energyOrbs[i].setOrigin(baseSize * sizePulse, baseSize * sizePulse);
        
        // Color based on field type
        sf::Color orbColor;
        switch (fieldType) {
            case FieldType::SHOCK:
                orbColor = sf::Color(100, 200, 255, 180 + 40 * std::sin(fieldPulsePhase * 3.0f + i));
                break;
            case FieldType::PLASMA:
                orbColor = sf::Color(255, 150, 100, 180 + 40 * std::sin(fieldPulsePhase * 3.0f + i));
                break;
            case FieldType::VORTEX:
                orbColor = sf::Color(180, 100, 255, 180 + 40 * std::sin(fieldPulsePhase * 3.0f + i));
                break;
            default:
                orbColor = sf::Color(200, 200, 255, 180 + 40 * std::sin(fieldPulsePhase * 3.0f + i));
        }
        energyOrbs[i].setFillColor(orbColor);
    }
    
    // Update particles
    updateParticles(dt, playerCenter);
    
    // Update combo timer
    if (consecutiveHits > 0) {
        comboTimer -= dt;
        if (comboTimer <= 0.0f) {
            consecutiveHits = 0;
        }
    }
    
    // Update charge level - slowly decay when not zapping
    if (!isZapping) {
        chargeLevel = std::max(0.0f, chargeLevel - dt * FIELD_CHARGE_DECAY_RATE);
    }
    
    // Update zap effect timer
    if (isZapping) {
        zapEffectTimer -= dt;
        if (zapEffectTimer <= 0.0f) {
            isZapping = false;
            zapEffect.resize(0);
            chainEffect.resize(0);
        }
    }
    
    // Update cooldown timer - adjust for power level and charge
    float adjustedCooldown = zapCooldown * (1.0f - 0.1f * (powerLevel - 1)) * (1.0f - chargeLevel * 0.3f);
    zapTimer -= dt;
    if (zapTimer <= 0.0f) {
        // Time to zap a new enemy
        FindAndZapEnemy(playerManager, enemyManager);
        zapTimer = adjustedCooldown;
    }
    
    // Field intensity increases during zapping and with charge level
    fieldIntensity = FIELD_INTENSITY_DEFAULT + (isZapping ? 0.5f : 0.0f) + chargeLevel * 0.5f;
    
    // Update field color based on type and intensity
    updateFieldColor();
}

void ForceField::Render(sf::RenderWindow& window) {
    // Skip if player is dead
    if (player->IsDead()) return;
    
    // Get player position
    sf::Vector2f playerCenter = player->GetPosition() + sf::Vector2f(25.0f, 25.0f);
    
    // Render particles behind everything else
    renderParticles(window);
    
    // Render field rings
    for (int i = 0; i < NUM_FIELD_RINGS; i++) {
        window.draw(fieldRings[i]);
    }
    
    // Render main force field
    window.draw(fieldShape);
    
    // Render energy orbs
    for (int i = 0; i < NUM_ENERGY_ORBS; i++) {
        window.draw(energyOrbs[i]);
    }
    
    // Render zap effects if active
    if (isZapping) {
        renderZapEffects(window);
    }
    
    // Render power level indicator
    renderPowerIndicator(window, playerCenter);
}

// This method needs to be updated in ForceField.cpp to properly handle kills

void ForceField::FindAndZapEnemy(PlayerManager& playerManager, EnemyManager& enemyManager) {
    // Get player position
    sf::Vector2f playerCenter = player->GetPosition() + sf::Vector2f(25.0f, 25.0f);
    
    // Find enemies within range
    std::vector<std::pair<int, sf::Vector2f>> enemiesInRange;
    
    // Adjusted radius based on power level
    float effectiveRadius = radius * (1.0f + 0.1f * (powerLevel - 1));
    float closestDistanceSquared = effectiveRadius * effectiveRadius;
    int closestEnemyId = -1;
    sf::Vector2f closestEnemyPos;
    
    // Search for enemies using random sampling (more efficient)
    int samplingPoints = 200 + powerLevel * 50; // More sampling at higher power levels
    for (int i = 0; i < samplingPoints; i++) {
        // Generate random point within field radius
        float angle = (float)(rand() % 360) * 3.14159f / 180.0f;
        float distance = (float)(rand() % (int)effectiveRadius);
        
        sf::Vector2f checkPos = playerCenter + sf::Vector2f(
            cosf(angle) * distance,
            sinf(angle) * distance
        );
        
        int enemyId = -1;
        if (enemyManager.CheckBulletCollision(checkPos, 10.0f, enemyId)) {
            if (enemyId != -1) {
                Enemy* enemy = enemyManager.FindEnemy(enemyId);
                if (enemy && !enemy->IsDead()) {
                    sf::Vector2f enemyPos = enemy->GetPosition();
                    float distSquared = std::pow(enemyPos.x - playerCenter.x, 2) + 
                                      std::pow(enemyPos.y - playerCenter.y, 2);
                    
                    // Store all enemies in range for chain lightning
                    if (distSquared < effectiveRadius * effectiveRadius) {
                        // Check if we already found this enemy
                        bool alreadyFound = false;
                        for (const auto& e : enemiesInRange) {
                            if (e.first == enemyId) {
                                alreadyFound = true;
                                break;
                            }
                        }
                        
                        if (!alreadyFound) {
                            enemiesInRange.push_back(std::make_pair(enemyId, enemyPos));
                        }
                        
                        // Track closest enemy
                        if (distSquared < closestDistanceSquared) {
                            closestDistanceSquared = distSquared;
                            closestEnemyId = enemyId;
                            closestEnemyPos = enemyPos;
                        }
                    }
                }
            }
        }
    }
    
    // If we found at least one enemy, zap it
    if (closestEnemyId != -1) {
        // Apply damage with power level and combo bonus
        float damageMultiplier = 1.0f + 0.2f * (powerLevel - 1) + 0.1f * consecutiveHits;
        float effectiveDamage = zapDamage * damageMultiplier;
        
        // Apply damage to primary target and check if killed
        bool killed = enemyManager.InflictDamage(closestEnemyId, effectiveDamage);
        
        // Increment consecutive hits and reset combo timer
        consecutiveHits++;
        comboTimer = FIELD_COMBO_DURATION;
        
        // Increase charge level
        chargeLevel = std::min(1.0f, chargeLevel + FIELD_CHARGE_GAIN_RATE);
        
        // Chain lightning if enabled and we have multiple enemies
        if (chainLightningEnabled && enemiesInRange.size() > 1) {
            performChainLightning(enemyManager, playerCenter, closestEnemyId, closestEnemyPos, enemiesInRange);
        }
        
        // Notify through callback
        if (zapCallback) {
            zapCallback(closestEnemyId, effectiveDamage, killed);
        }
        
        // Show zap effect
        CreateZapEffect(playerCenter, closestEnemyPos);
        
        // Save target info
        targetEnemyId = closestEnemyId;
        zapEndPosition = closestEnemyPos;
        isZapping = true;
        zapEffectTimer = zapEffectDuration;
        
        // Create impact particles
        createImpactParticles(closestEnemyPos);
    }
}

void ForceField::CreateZapEffect(const sf::Vector2f& start, const sf::Vector2f& end) {
    // Clear existing effect
    zapEffect.clear();
    
    // Number of line segments for the zap - more at higher power levels
    const int segments = ZAP_BASE_SEGMENTS + powerLevel * ZAP_SEGMENTS_PER_POWER;
    
    // Direction vector
    sf::Vector2f direction = end - start;
    float distance = std::hypot(direction.x, direction.y);
    
    if (distance < 0.001f) return; // Prevent division by zero
    
    // Normalized perpendicular vector for zig-zag
    sf::Vector2f perpendicular(-direction.y / distance, direction.x / distance);
    
    // Determine zap color based on field type
    sf::Color zapBaseColor;
    sf::Color zapBrightColor;
    
    switch (fieldType) {
        case FieldType::SHOCK:
            zapBaseColor = ZAP_SHOCK_BASE_COLOR;
            zapBrightColor = ZAP_SHOCK_BRIGHT_COLOR;
            break;
        case FieldType::PLASMA:
            zapBaseColor = ZAP_PLASMA_BASE_COLOR;
            zapBrightColor = ZAP_PLASMA_BRIGHT_COLOR;
            break;
        case FieldType::VORTEX:
            zapBaseColor = ZAP_VORTEX_BASE_COLOR;
            zapBrightColor = ZAP_VORTEX_BRIGHT_COLOR;
            break;
        default:
            zapBaseColor = ZAP_STANDARD_BASE_COLOR;
            zapBrightColor = ZAP_STANDARD_BRIGHT_COLOR;
    }
    
    // Create a jagged lightning effect with power level influencing complexity
    sf::Vector2f currentPos = start;
    for (int i = 0; i < segments; i++) {
        // Calculate next position along the line with random offset
        float t = (i + 1.0f) / segments;
        sf::Vector2f nextPos = start + (direction * t);
        
        // Add some randomness for zig-zag effect (more pronounced at higher power)
        if (i < segments - 1) {
            float offset = (rand() % (ZAP_OFFSET_MAX - ZAP_OFFSET_MIN + 1) + ZAP_OFFSET_MIN) * 
                           (1.0f + powerLevel * ZAP_OFFSET_POWER_FACTOR) / 2.0f;
            nextPos += perpendicular * offset;
        }
        
        // Add line segment with glow effect
        sf::Color startColor = zapBaseColor;
        sf::Color endColor = zapBrightColor;
        
        // Adjust alpha for fade-out effect
        startColor.a = 255 - (i * 255 / segments);
        endColor.a = 255 - (i * 255 / segments);
        
        // Main line
        zapEffect.append(sf::Vertex(currentPos, startColor));
        zapEffect.append(sf::Vertex(nextPos, endColor));
        
        // Add parallel lines for thickness effect
        float thickness = ZAP_THICKNESS_BASE * (1.0f + ZAP_THICKNESS_POWER_FACTOR * powerLevel);
        sf::Vector2f offsetPerp = perpendicular * thickness;
        
        zapEffect.append(sf::Vertex(currentPos + offsetPerp, startColor));
        zapEffect.append(sf::Vertex(nextPos + offsetPerp, endColor));
        
        zapEffect.append(sf::Vertex(currentPos - offsetPerp, startColor));
        zapEffect.append(sf::Vertex(nextPos - offsetPerp, endColor));
        
        // Add branches based on power level
        int branchChance = ZAP_BRANCH_CHANCE_BASE + powerLevel * ZAP_BRANCH_CHANCE_PER_POWER;
        if (i > 0 && i < segments - 2 && rand() % 100 < branchChance) {
            createLightningBranch(currentPos, direction, distance, i, segments, zapBaseColor, zapBrightColor);
        }
        
        currentPos = nextPos;
    }
}

void ForceField::createLightningBranch(
    const sf::Vector2f& branchStart, 
    const sf::Vector2f& mainDirection, 
    float mainDistance,
    int currentSegment, 
    int totalSegments,
    const sf::Color& baseColor,
    const sf::Color& brightColor
) {
    // Create perpendicular vector for branch direction
    sf::Vector2f branchDir(mainDirection.y, -mainDirection.x);
    
    // Randomize branch direction
    if (rand() % 2 == 0) {
        branchDir = -branchDir;
    }
    
    // Add some randomness to branch angle
    float angleAdjust = (rand() % (ZAP_BRANCH_ANGLE_MAX - ZAP_BRANCH_ANGLE_MIN + 1) + ZAP_BRANCH_ANGLE_MIN) * PI / 180.0f;
    float cosA = cos(angleAdjust);
    float sinA = sin(angleAdjust);
    branchDir = sf::Vector2f(
        branchDir.x * cosA - branchDir.y * sinA,
        branchDir.x * sinA + branchDir.y * cosA
    );
    
    // Normalize and scale branch direction
    float branchLen = mainDistance * (ZAP_BRANCH_LENGTH_FACTOR + (rand() % 100) / 500.0f) * (1.0f + ZAP_BRANCH_LENGTH_VARIATION * powerLevel);
    float branchDirMag = std::hypot(branchDir.x, branchDir.y);
    branchDir = branchDir * (branchLen / branchDirMag);
    
    // Create the branch using configurable number of segments (more at higher power)
    sf::Vector2f branchPos = branchStart;
    int branchSegments = ZAP_BRANCH_MIN_SEGMENTS + rand() % (1 + powerLevel);
    
    for (int j = 0; j < branchSegments; j++) {
        float bt = (j + 1.0f) / branchSegments;
        sf::Vector2f nextBranchPos = branchStart + (branchDir * bt);
        
        // Add randomness to branch
        nextBranchPos += sf::Vector2f(
            (rand() % ZAP_BRANCH_RANDOMNESS - ZAP_BRANCH_RANDOMNESS/2) * (1.0f + ZAP_BRANCH_LENGTH_VARIATION * powerLevel),
            (rand() % ZAP_BRANCH_RANDOMNESS - ZAP_BRANCH_RANDOMNESS/2) * (1.0f + ZAP_BRANCH_LENGTH_VARIATION * powerLevel)
        );
        
        // Calculate colors with alpha fade
        sf::Color startBranchColor = baseColor;
        sf::Color endBranchColor = brightColor;
        
        float alphaMultiplier = 1.0f - (float)currentSegment / totalSegments;
        startBranchColor.a = static_cast<sf::Uint8>(200 * alphaMultiplier - (j * 40));
        endBranchColor.a = static_cast<sf::Uint8>(150 * alphaMultiplier - (j * 40));
        
        // Add branch segment
        zapEffect.append(sf::Vertex(branchPos, startBranchColor));
        zapEffect.append(sf::Vertex(nextBranchPos, endBranchColor));
        
        // Chance for sub-branches at higher power levels
        if (powerLevel >= ZAP_SUB_BRANCH_POWER_MIN && j < branchSegments - 1 && rand() % 100 < ZAP_SUB_BRANCH_CHANCE) {
            sf::Vector2f subBranchDir(branchDir.y, -branchDir.x);
            if (rand() % 2 == 0) subBranchDir = -subBranchDir;
            
            float subLen = branchLen * ZAP_SUB_BRANCH_LENGTH;
            subBranchDir = subBranchDir * (subLen / std::hypot(subBranchDir.x, subBranchDir.y));
            
            sf::Vector2f subBranchEnd = branchPos + subBranchDir;
            
            // Add randomness
            subBranchEnd += sf::Vector2f(
                (rand() % 30 - 15), 
                (rand() % 30 - 15)
            );
            
            // Add sub-branch
            sf::Color subColor = startBranchColor;
            subColor.a = static_cast<sf::Uint8>(subColor.a * 0.7f);
            
            zapEffect.append(sf::Vertex(branchPos, subColor));
            zapEffect.append(sf::Vertex(subBranchEnd, sf::Color(brightColor.r, brightColor.g, brightColor.b, 0)));
        }
        
        branchPos = nextBranchPos;
    }
}

void ForceField::performChainLightning(
    EnemyManager& enemyManager,
    const sf::Vector2f& playerCenter,
    int primaryTargetId,
    const sf::Vector2f& primaryTargetPos,
    const std::vector<std::pair<int, sf::Vector2f>>& enemiesInRange
) {
    // Clear existing chain effects
    chainEffect.clear();
    
    // Calculate how many chain targets based on power level
    int effectiveChainTargets = std::min(static_cast<int>(enemiesInRange.size() - 1), 
                                         chainLightningTargets + (powerLevel - 1));
    
    if (effectiveChainTargets <= 0) return;
    
    // Sort enemies by distance from primary target
    std::vector<std::pair<int, sf::Vector2f>> chainTargets;
    for (const auto& enemy : enemiesInRange) {
        if (enemy.first != primaryTargetId) {
            chainTargets.push_back(enemy);
        }
    }
    
    // Sort by distance from primary target
    std::sort(chainTargets.begin(), chainTargets.end(), 
        [primaryTargetPos](const std::pair<int, sf::Vector2f>& a, const std::pair<int, sf::Vector2f>& b) {
            float distA = std::pow(a.second.x - primaryTargetPos.x, 2) + 
                          std::pow(a.second.y - primaryTargetPos.y, 2);
            float distB = std::pow(b.second.x - primaryTargetPos.x, 2) + 
                          std::pow(b.second.y - primaryTargetPos.y, 2);
            return distA < distB;
        });
    
    // Cap the number of chain targets
    if (chainTargets.size() > effectiveChainTargets) {
        chainTargets.resize(effectiveChainTargets);
    }
    
    // Create chain lightning effects and apply damage
    sf::Vector2f prevPos = primaryTargetPos;
    float damageMultiplier = 1.0f + CHAIN_POWER_FACTOR * (powerLevel - 1);
    float chainDamage = zapDamage * CHAIN_DAMAGE_FACTOR * damageMultiplier; // Chain does percentage of primary damage
    
    for (size_t i = 0; i < chainTargets.size(); i++) {
        int enemyId = chainTargets[i].first;
        sf::Vector2f enemyPos = chainTargets[i].second;
        
        // Create lightning effect between previous target and this one
        createChainLightningEffect(prevPos, enemyPos);
        
        // Apply reduced damage to chained targets - damage falls off with each jump
        float targetDamage = chainDamage * (1.0f - CHAIN_DAMAGE_DROPOFF * i);
        bool killed = enemyManager.InflictDamage(enemyId, targetDamage);
        
        // Notify through callback
        if (zapCallback) {
            zapCallback(enemyId, targetDamage, killed);
        }
        
        // Create impact particles
        createImpactParticles(enemyPos);
        
        prevPos = enemyPos;
    }
}

void ForceField::createChainLightningEffect(const sf::Vector2f& start, const sf::Vector2f& end) {
    // Direction vector
    sf::Vector2f direction = end - start;
    float distance = std::hypot(direction.x, direction.y);
    
    if (distance < 0.001f) return; // Prevent division by zero
    
    // Normalized perpendicular vector for zig-zag
    sf::Vector2f perpendicular(-direction.y / distance, direction.x / distance);
    
    // Determine chain color based on field type but more transparent
    sf::Color chainBaseColor;
    sf::Color chainBrightColor;
    
    switch (fieldType) {
        case FieldType::SHOCK:
            chainBaseColor = CHAIN_SHOCK_BASE_COLOR;
            chainBrightColor = CHAIN_SHOCK_BRIGHT_COLOR;
            break;
        case FieldType::PLASMA:
            chainBaseColor = CHAIN_PLASMA_BASE_COLOR;
            chainBrightColor = CHAIN_PLASMA_BRIGHT_COLOR;
            break;
        case FieldType::VORTEX:
            chainBaseColor = CHAIN_VORTEX_BASE_COLOR;
            chainBrightColor = CHAIN_VORTEX_BRIGHT_COLOR;
            break;
        default:
            chainBaseColor = CHAIN_STANDARD_BASE_COLOR;
            chainBrightColor = CHAIN_STANDARD_BRIGHT_COLOR;
    }
    
    // Create chain lightning with fewer segments than primary
    const int segments = CHAIN_ZAP_BASE_SEGMENTS + powerLevel;
    sf::Vector2f currentPos = start;
    
    for (int i = 0; i < segments; i++) {
        // Calculate next position along the line with random offset
        float t = (i + 1.0f) / segments;
        sf::Vector2f nextPos = start + (direction * t);
        
        // Add some randomness for zig-zag effect
        if (i < segments - 1) {
            float offset = (rand() % (CHAIN_OFFSET_MAX - CHAIN_OFFSET_MIN + 1) + CHAIN_OFFSET_MIN) * 
                          (1.0f + powerLevel * CHAIN_OFFSET_POWER_FACTOR) / 2.0f;
            nextPos += perpendicular * offset;
        }
        
        // Adjusted colors with fade-out
        sf::Color startColor = chainBaseColor;
        sf::Color endColor = chainBrightColor;
        
        startColor.a = static_cast<sf::Uint8>(startColor.a * (1.0f - (float)i / segments));
        endColor.a = static_cast<sf::Uint8>(endColor.a * (1.0f - (float)i / segments));
        
        // Add chain segment
        chainEffect.append(sf::Vertex(currentPos, startColor));
        chainEffect.append(sf::Vertex(nextPos, endColor));
        
        currentPos = nextPos;
    }
}

void ForceField::renderZapEffects(sf::RenderWindow& window) {
    // Main zap effect rendering with enhanced glow
    if (zapEffect.getVertexCount() > 0) {
        // Add background glow effect for more dramatic lighting
        sf::CircleShape backgroundGlow;
        backgroundGlow.setRadius(ZAP_GLOW_RADIUS_BASE + ZAP_GLOW_RADIUS_PER_POWER * powerLevel);
        
        sf::Color bgGlowColor;
        switch (fieldType) {
            case FieldType::SHOCK:
                bgGlowColor = sf::Color(100, 200, 255, 50);
                break;
            case FieldType::PLASMA:
                bgGlowColor = sf::Color(255, 150, 100, 50);
                break;
            case FieldType::VORTEX:
                bgGlowColor = sf::Color(200, 100, 255, 50);
                break;
            default:
                bgGlowColor = sf::Color(150, 200, 255, 50);
        }
        
        backgroundGlow.setFillColor(bgGlowColor);
        backgroundGlow.setOrigin(backgroundGlow.getRadius(), backgroundGlow.getRadius());
        
        // Place larger background glows at major vertices
        for (size_t i = 0; i < zapEffect.getVertexCount(); i += 12) {
            backgroundGlow.setPosition(zapEffect[i].position);
            window.draw(backgroundGlow);
        }
        
        // Add primary glow effect
        sf::CircleShape zapGlow;
        zapGlow.setRadius(ZAP_PRIMARY_GLOW_RADIUS_BASE + ZAP_PRIMARY_GLOW_RADIUS_PER_POWER * powerLevel);
        
        sf::Color glowColor;
        switch (fieldType) {
            case FieldType::SHOCK:
                glowColor = sf::Color(100, 200, 255, 100);
                break;
            case FieldType::PLASMA:
                glowColor = sf::Color(255, 150, 100, 100);
                break;
            case FieldType::VORTEX:
                glowColor = sf::Color(200, 100, 255, 100);
                break;
            default:
                glowColor = sf::Color(150, 200, 255, 100);
        }
        
        zapGlow.setFillColor(glowColor);
        zapGlow.setOrigin(zapGlow.getRadius(), zapGlow.getRadius());
        
        // Place glows at key vertices with pulsing effect
        for (size_t i = 0; i < zapEffect.getVertexCount(); i += 6) {
            // Add subtle variation to each glow
            float pulseOffset = (i * 0.01f) + fieldPulsePhase * 3.0f;
            float pulseFactor = 0.8f + 0.2f * std::sin(pulseOffset);
            
            zapGlow.setPosition(zapEffect[i].position);
            zapGlow.setScale(pulseFactor, pulseFactor);
            window.draw(zapGlow);
        }
        
        // Draw the actual zap effect
        window.draw(zapEffect);
        
        // Add dynamic electricity particles along the zap path
        if (powerLevel >= ZAP_SPARKLE_MIN_POWER) {
            sf::CircleShape sparkle;
            sparkle.setRadius(ZAP_SPARKLE_RADIUS);
            sparkle.setOrigin(sparkle.getRadius() / 2, sparkle.getRadius() / 2);
            
            sf::Color sparkleColor;
            switch (fieldType) {
                case FieldType::SHOCK:
                    sparkleColor = sf::Color(220, 240, 255, 200);
                    break;
                case FieldType::PLASMA:
                    sparkleColor = sf::Color(255, 220, 180, 200);
                    break;
                case FieldType::VORTEX:
                    sparkleColor = sf::Color(230, 200, 255, 200);
                    break;
                default:
                    sparkleColor = sf::Color(220, 240, 255, 200);
            }
            sparkle.setFillColor(sparkleColor);
            
            // Add random sparkles along the path
            int numSparkles = ZAP_SPARKLE_BASE + ZAP_SPARKLE_PER_POWER * powerLevel;
            for (int i = 0; i < numSparkles; i++) {
                // Pick a random point along the zap
                size_t idx = rand() % (zapEffect.getVertexCount() / 2);
                idx = idx * 2; // Ensure we get the start vertex of a line segment
                
                if (idx < zapEffect.getVertexCount() - 1) {
                    // Interpolate between vertices
                    float t = (rand() % 100) / 100.0f;
                    sf::Vector2f pos = zapEffect[idx].position * (1 - t) + zapEffect[idx + 1].position * t;
                    
                    // Offset slightly
                    pos += sf::Vector2f(
                        (rand() % (ZAP_SPARKLE_OFFSET_MAX - ZAP_SPARKLE_OFFSET_MIN + 1) + ZAP_SPARKLE_OFFSET_MIN) * 
                        (1.0f + ZAP_SPARKLE_OFFSET_POWER_FACTOR * powerLevel),
                        (rand() % (ZAP_SPARKLE_OFFSET_MAX - ZAP_SPARKLE_OFFSET_MIN + 1) + ZAP_SPARKLE_OFFSET_MIN) * 
                        (1.0f + ZAP_SPARKLE_OFFSET_POWER_FACTOR * powerLevel)
                    );
                    
                    sparkle.setPosition(pos);
                    float scale = 0.5f + (rand() % 100) / 50.0f;
                    sparkle.setScale(scale, scale);
                    window.draw(sparkle);
                }
            }
        }
        
        // Add impact flash at target with enhanced effects
        sf::CircleShape impactFlash;
        impactFlash.setRadius(ZAP_IMPACT_FLASH_RADIUS_BASE + ZAP_IMPACT_FLASH_RADIUS_PER_POWER * powerLevel);
        impactFlash.setOrigin(impactFlash.getRadius(), impactFlash.getRadius());
        impactFlash.setPosition(zapEndPosition);
        
        // Pulsing impact flash
        float flashPulse = ZAP_IMPACT_PULSE_MIN + ZAP_IMPACT_PULSE_MAX * std::sin(fieldPulsePhase * ZAP_IMPACT_PULSE_FREQUENCY);
        impactFlash.setScale(flashPulse, flashPulse);
        
        // Impact color based on field type
        sf::Color impactColor;
        switch (fieldType) {
            case FieldType::SHOCK:
                impactColor = sf::Color(150, 220, 255, 180);
                break;
            case FieldType::PLASMA:
                impactColor = sf::Color(255, 180, 120, 180);
                break;
            case FieldType::VORTEX:
                impactColor = sf::Color(220, 150, 255, 180);
                break;
            default:
                impactColor = sf::Color(180, 220, 255, 180);
        }
        impactFlash.setFillColor(impactColor);
        window.draw(impactFlash);
        
        // Add secondary impact rings for more dramatic effect
        if (zapEffectTimer > zapEffectDuration * ZAP_IMPACT_RING_DURATION_FACTOR) { // Only during initial part of effect
            int ringCount = 1 + powerLevel / 2; // More rings at higher power
            
            for (int i = 0; i < ringCount; i++) {
                // Calculate ring expansion over time
                float ringProgress = 1.0f - (zapEffectTimer / zapEffectDuration);
                float ringSize = (10.0f + 40.0f * ringProgress) * (1.0f + 0.2f * i);
                
                sf::CircleShape impactRing;
                impactRing.setRadius(ringSize);
                impactRing.setOrigin(ringSize, ringSize);
                impactRing.setPosition(zapEndPosition);
                impactRing.setFillColor(sf::Color::Transparent);
                
                // Ring color based on field type but fading out as it expands
                sf::Color ringColor = impactColor;
                ringColor.a = static_cast<sf::Uint8>(200 * (1.0f - ringProgress) / (i + 1));
                impactRing.setOutlineColor(ringColor);
                impactRing.setOutlineThickness(2.0f);
                
                window.draw(impactRing);
            }
        }
        
        // Add dramatic shockwave if this was a critical hit (combo >= 3 or full charge)
        if (consecutiveHits >= ZAP_CRITICAL_COMBO_THRESHOLD || chargeLevel > ZAP_CRITICAL_CHARGE_THRESHOLD) {
            sf::CircleShape shockwave;
            float shockwaveProgress = 1.0f - (zapEffectTimer / zapEffectDuration);
            float shockwaveSize = ZAP_SHOCKWAVE_SIZE_BASE * shockwaveProgress * 
                                  (1.0f + ZAP_SHOCKWAVE_POWER_FACTOR * powerLevel);
            
            shockwave.setRadius(shockwaveSize);
            shockwave.setOrigin(shockwaveSize, shockwaveSize);
            shockwave.setPosition(zapEndPosition);
            shockwave.setFillColor(sf::Color::Transparent);
            
            // Shockwave color based on field type
            sf::Color shockwaveColor = impactColor;
            shockwaveColor.a = static_cast<sf::Uint8>(150 * (1.0f - shockwaveProgress));
            shockwave.setOutlineColor(shockwaveColor);
            shockwave.setOutlineThickness(3.0f + 2.0f * (1.0f - shockwaveProgress));
            
            window.draw(shockwave);
        }
    }
    
    // Render chain lightning effects with added glow
    if (chainEffect.getVertexCount() > 0) {
        // Add glow along chain lightning path
        sf::CircleShape chainGlow;
        chainGlow.setRadius(CHAIN_GLOW_RADIUS_BASE + powerLevel);
        chainGlow.setOrigin(chainGlow.getRadius(), chainGlow.getRadius());
        
        sf::Color chainGlowColor;
        switch (fieldType) {
            case FieldType::SHOCK:
                chainGlowColor = sf::Color(100, 200, 255, CHAIN_GLOW_ALPHA);
                break;
            case FieldType::PLASMA:
                chainGlowColor = sf::Color(255, 150, 100, CHAIN_GLOW_ALPHA);
                break;
            case FieldType::VORTEX:
                chainGlowColor = sf::Color(200, 100, 255, CHAIN_GLOW_ALPHA);
                break;
            default:
                chainGlowColor = sf::Color(150, 200, 255, CHAIN_GLOW_ALPHA);
        }
        chainGlow.setFillColor(chainGlowColor);
        
        // Place glows along chain path
        for (size_t i = 0; i < chainEffect.getVertexCount(); i += 4) {
            if (i < chainEffect.getVertexCount()) {
                chainGlow.setPosition(chainEffect[i].position);
                window.draw(chainGlow);
            }
        }
        
        // Draw the chain lightning effect
        window.draw(chainEffect);
    }
}

void ForceField::initializeParticles() {
    // Initialize all particles as inactive
    for (int i = 0; i < MAX_PARTICLES; i++) {
        particles[i].active = false;
    }
}

void ForceField::updateParticles(float dt, const sf::Vector2f& playerCenter) {
    // Update existing particles
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (particles[i].active) {
            // Update lifetime
            particles[i].lifetime -= dt;
            if (particles[i].lifetime <= 0.0f) {
                particles[i].active = false;
                continue;
            }
            
            // Update position
            particles[i].position += particles[i].velocity * dt;
            
            // Update alpha based on lifetime
            float lifeRatio = particles[i].lifetime / particles[i].maxLifetime;
            particles[i].color.a = static_cast<sf::Uint8>(255 * lifeRatio * lifeRatio);
            
            // Update size with a slight pulse
            float sizePulse = 1.0f + 0.2f * std::sin(particles[i].lifetime * 5.0f);
            particles[i].size = particles[i].baseSize * lifeRatio * sizePulse;
            
            // For orbiting particles, update orbit position
            if (particles[i].type == ParticleType::ORBIT) {
                // Update orbit angle
                particles[i].orbitAngle += dt * particles[i].orbitSpeed;
                
                // Calculate orbit position
                float orbitX = playerCenter.x + std::cos(particles[i].orbitAngle) * particles[i].orbitDistance;
                float orbitY = playerCenter.y + std::sin(particles[i].orbitAngle) * particles[i].orbitDistance;
                
                // Gradually move toward orbit position
                particles[i].position = particles[i].position * 0.9f + sf::Vector2f(orbitX, orbitY) * 0.1f;
            }
        }
    }
    
    // Generate new ambient particles when field is active
    if (fieldIntensity > 1.0f && rand() % 100 < 30 * fieldIntensity) {
        createAmbientParticle(playerCenter);
    }
}

void ForceField::renderParticles(sf::RenderWindow& window) {
    sf::CircleShape particleShape;
    
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (particles[i].active) {
            particleShape.setRadius(particles[i].size);
            particleShape.setOrigin(particles[i].size, particles[i].size);
            particleShape.setPosition(particles[i].position);
            particleShape.setFillColor(particles[i].color);
            window.draw(particleShape);
        }
    }
}

void ForceField::createImpactParticles(const sf::Vector2f& impactPos) {
    // Create a burst of particles at impact position
    const int numParticles = IMPACT_PARTICLES_BASE + powerLevel * IMPACT_PARTICLES_PER_POWER;
    
    for (int j = 0; j < numParticles; j++) {
        // Find an inactive particle
        for (int i = 0; i < MAX_PARTICLES; i++) {
            if (!particles[i].active) {
                // Random angle
                float angle = (rand() % 360) * PI / 180.0f;
                float speed = IMPACT_PARTICLE_SPEED_MIN + (rand() % (int)(IMPACT_PARTICLE_SPEED_MAX - IMPACT_PARTICLE_SPEED_MIN));
                
                particles[i].active = true;
                particles[i].type = ParticleType::IMPACT;
                particles[i].position = impactPos;
                particles[i].velocity = sf::Vector2f(
                    cosf(angle) * speed,
                    sinf(angle) * speed
                );
                
                particles[i].baseSize = IMPACT_PARTICLE_SIZE_MIN + (rand() % (int)(IMPACT_PARTICLE_SIZE_MAX - IMPACT_PARTICLE_SIZE_MIN));
                particles[i].size = particles[i].baseSize;
                particles[i].maxLifetime = IMPACT_PARTICLE_LIFETIME_MIN + 
                                         (rand() % 100) / 200.0f * 
                                         (IMPACT_PARTICLE_LIFETIME_MAX - IMPACT_PARTICLE_LIFETIME_MIN);
                particles[i].lifetime = particles[i].maxLifetime;
                
                // Color based on field type, but brighter
                switch (fieldType) {
                    case FieldType::SHOCK:
                        particles[i].color = sf::Color(150 + rand() % 105, 200 + rand() % 55, 255, 255);
                        break;
                    case FieldType::PLASMA:
                        particles[i].color = sf::Color(255, 150 + rand() % 105, 100 + rand() % 100, 255);
                        break;
                    case FieldType::VORTEX:
                        particles[i].color = sf::Color(200 + rand() % 55, 100 + rand() % 100, 255, 255);
                        break;
                    default:
                        particles[i].color = sf::Color(200 + rand() % 55, 200 + rand() % 55, 255, 255);
                }
                
                break;
            }
        }
    }
}

void ForceField::createAmbientParticle(const sf::Vector2f& center) {
    // Find an inactive particle
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!particles[i].active) {
            // Random angle and distance from center
            float angle = (rand() % 360) * PI / 180.0f;
            float distance = radius * (0.2f + 0.8f * (rand() % 100) / 100.0f);
            
            particles[i].active = true;
            particles[i].position = center + sf::Vector2f(
                cosf(angle) * distance,
                sinf(angle) * distance
            );
            
            // Random type with weights based on field type
            int typeRoll = rand() % 100;
            if (typeRoll < PARTICLE_AMBIENT_CHANCE) { // Chance for ambient particles
                particles[i].type = ParticleType::AMBIENT;
                particles[i].velocity = sf::Vector2f(
                    (rand() % (PARTICLE_VELOCITY_RANGE * 2) - PARTICLE_VELOCITY_RANGE) * PARTICLE_VELOCITY_MULTIPLIER,
                    (rand() % (PARTICLE_VELOCITY_RANGE * 2) - PARTICLE_VELOCITY_RANGE) * PARTICLE_VELOCITY_MULTIPLIER
                );
                particles[i].baseSize = PARTICLE_AMBIENT_SIZE_MIN + (rand() % (int)(PARTICLE_AMBIENT_SIZE_MAX - PARTICLE_AMBIENT_SIZE_MIN));
                particles[i].size = particles[i].baseSize;
                particles[i].maxLifetime = PARTICLE_AMBIENT_LIFETIME_MIN + (rand() % 100) / 100.0f * 
                                         (PARTICLE_AMBIENT_LIFETIME_MAX - PARTICLE_AMBIENT_LIFETIME_MIN);
            } else { // Chance for orbiting particles
                particles[i].type = ParticleType::ORBIT;
                particles[i].orbitAngle = angle;
                particles[i].orbitSpeed = (PARTICLE_ORBIT_SPEED_MIN + rand() % (int)(PARTICLE_ORBIT_SPEED_MAX - PARTICLE_ORBIT_SPEED_MIN)) * 
                                         (rand() % 2 == 0 ? 1.0f : -1.0f);
                particles[i].orbitDistance = distance;
                particles[i].baseSize = PARTICLE_ORBIT_SIZE_MIN + (rand() % (int)(PARTICLE_ORBIT_SIZE_MAX - PARTICLE_ORBIT_SIZE_MIN));
                particles[i].size = particles[i].baseSize;
                particles[i].maxLifetime = PARTICLE_ORBIT_LIFETIME_MIN + (rand() % 200) / 100.0f * 
                                         (PARTICLE_ORBIT_LIFETIME_MAX - PARTICLE_ORBIT_LIFETIME_MIN);
            }
            
            particles[i].lifetime = particles[i].maxLifetime;
            
            // Color based on field type
            switch (fieldType) {
                case FieldType::SHOCK:
                    particles[i].color = sf::Color(
                        100 + rand() % PARTICLE_COLOR_VARIATION, 
                        180 + rand() % (int)(PARTICLE_COLOR_VARIATION * 0.75f), 
                        255, 
                        PARTICLE_DEFAULT_ALPHA);
                    break;
                case FieldType::PLASMA:
                    particles[i].color = sf::Color(
                        255, 
                        100 + rand() % PARTICLE_COLOR_VARIATION, 
                        50 + rand() % PARTICLE_COLOR_VARIATION, 
                        PARTICLE_DEFAULT_ALPHA);
                    break;
                case FieldType::VORTEX:
                    particles[i].color = sf::Color(
                        150 + rand() % PARTICLE_COLOR_VARIATION, 
                        50 + rand() % PARTICLE_COLOR_VARIATION, 
                        255, 
                        PARTICLE_DEFAULT_ALPHA);
                    break;
                default:
                    particles[i].color = sf::Color(
                        150 + rand() % PARTICLE_COLOR_VARIATION, 
                        150 + rand() % PARTICLE_COLOR_VARIATION, 
                        255, 
                        PARTICLE_DEFAULT_ALPHA);
            }
            
            // Only create one particle per call
            break;
        }
    }
}

void ForceField::renderPowerIndicator(sf::RenderWindow& window, const sf::Vector2f& playerCenter) {
    // Only show indicator when charged or at higher power levels
    if (powerLevel <= POWER_MIN_LEVEL && chargeLevel < POWER_INDICATOR_MIN) return;
    
    // Power level indicator as orbiting diamonds
    for (int i = 0; i < powerLevel; i++) {
        sf::CircleShape powerMarker(POWER_MARKER_RADIUS, POWER_MARKER_SIDES); // Diamond shape
        powerMarker.setOrigin(POWER_MARKER_RADIUS, POWER_MARKER_RADIUS);
        
        // Position in orbit around player
        float markerAngle = fieldRotation * 0.5f + (i * 360.0f / powerLevel);
        float markerDist = radius * POWER_MARKER_DISTANCE_FACTOR;
        sf::Vector2f markerPos = playerCenter + sf::Vector2f(
            cosf(markerAngle * PI / 180.0f) * markerDist,
            sinf(markerAngle * PI / 180.0f) * markerDist
        );
        
        powerMarker.setPosition(markerPos);
        
        // Color based on field type
        sf::Color markerColor;
        switch (fieldType) {
            case FieldType::SHOCK:
                markerColor = sf::Color(100, 200, 255, POWER_MARKER_ALPHA);
                break;
            case FieldType::PLASMA:
                markerColor = sf::Color(255, 150, 100, POWER_MARKER_ALPHA);
                break;
            case FieldType::VORTEX:
                markerColor = sf::Color(200, 100, 255, POWER_MARKER_ALPHA);
                break;
            default:
                markerColor = sf::Color(150, 220, 255, POWER_MARKER_ALPHA);
        }
        powerMarker.setFillColor(markerColor);
        
        // Pulsing size
        float pulseFactor = POWER_MARKER_PULSE_MIN + POWER_MARKER_PULSE_MAX * 
                           std::sin(fieldPulsePhase * POWER_MARKER_PULSE_FREQUENCY + i * 0.5f);
        powerMarker.setScale(pulseFactor, pulseFactor);
        
        window.draw(powerMarker);
    }
    
    // Charge level indicator
    if (chargeLevel > CHARGE_DISPLAY_THRESHOLD) {
        sf::RectangleShape chargeBar;
        float chargeWidth = CHARGE_BAR_WIDTH * chargeLevel;
        chargeBar.setSize(sf::Vector2f(chargeWidth, CHARGE_BAR_HEIGHT));
        chargeBar.setPosition(playerCenter.x - CHARGE_BAR_WIDTH/2, playerCenter.y - radius * CHARGE_BAR_DISTANCE_FACTOR);
        
        // Color based on charge level and field type
        sf::Color chargeColor;
        switch (fieldType) {
            case FieldType::SHOCK:
                chargeColor = sf::Color(100, 150 + static_cast<int>(105 * chargeLevel), 255, POWER_MARKER_ALPHA);
                break;
            case FieldType::PLASMA:
                chargeColor = sf::Color(255, 100 + static_cast<int>(155 * chargeLevel), 50, POWER_MARKER_ALPHA);
                break;
            case FieldType::VORTEX:
                chargeColor = sf::Color(150 + static_cast<int>(105 * chargeLevel), 50, 255, POWER_MARKER_ALPHA);
                break;
            default:
                chargeColor = sf::Color(100 + static_cast<int>(155 * chargeLevel), 200, 255, POWER_MARKER_ALPHA);
        }
        chargeBar.setFillColor(chargeColor);
        
        // Pulsing opacity for high charge
        if (chargeLevel > CHARGE_HIGH_THRESHOLD) {
            float pulseAlpha = CHARGE_PULSE_ALPHA_MIN + CHARGE_PULSE_ALPHA_MAX * 
                              std::sin(fieldPulsePhase * CHARGE_PULSE_FREQUENCY);
            chargeColor.a = static_cast<sf::Uint8>(pulseAlpha);
            chargeBar.setFillColor(chargeColor);
        }
        
        window.draw(chargeBar);
    }
}

void ForceField::updateFieldColor() {
    // Update field colors based on field type and intensity
    sf::Color newBaseColor;
    sf::Color newOutlineColor;
    
    switch (fieldType) {
        case FieldType::SHOCK:
            newBaseColor = UPDATE_SHOCK_FIELD_COLOR;
            newOutlineColor = UPDATE_SHOCK_OUTLINE_COLOR;
            break;
        case FieldType::PLASMA:
            newBaseColor = UPDATE_PLASMA_FIELD_COLOR;
            newOutlineColor = UPDATE_PLASMA_OUTLINE_COLOR;
            break;
        case FieldType::VORTEX:
            newBaseColor = UPDATE_VORTEX_FIELD_COLOR;
            newOutlineColor = UPDATE_VORTEX_OUTLINE_COLOR;
            break;
        default:
            newBaseColor = UPDATE_STANDARD_FIELD_COLOR;
            newOutlineColor = UPDATE_STANDARD_OUTLINE_COLOR;
    }
    
    // Adjust based on field intensity
    newBaseColor.a = static_cast<sf::Uint8>(newBaseColor.a * (FIELD_COLOR_INTENSITY_MIN + FIELD_COLOR_INTENSITY_MAX * fieldIntensity));
    newOutlineColor.a = static_cast<sf::Uint8>(newOutlineColor.a * (FIELD_COLOR_INTENSITY_MIN + FIELD_COLOR_INTENSITY_MAX * fieldIntensity));
    
    // Apply updated colors
    fieldShape.setFillColor(newBaseColor);
    fieldShape.setOutlineColor(newOutlineColor);
    
    // Also update the field base color for future reference
    fieldColor = newBaseColor;
}