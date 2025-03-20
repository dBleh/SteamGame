#include "ForceField.h"
#include "Player.h"
#include "PlayerManager.h"
#include "enemies/EnemyManager.h"
#include "enemies/Enemy.h"
#include "../core/Game.h"
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
      zapEffectDuration(0.3f),
      zapEffectTimer(0.0f),
      fieldRotation(0.0f),
      fieldPulsePhase(0.0f),
      fieldIntensity(1.0f),
      chargeLevel(0.0f),
      powerLevel(5),
      consecutiveHits(0),
      comboTimer(0.0f),
      fieldColor(sf::Color(100, 100, 255, 50)),
      chainLightningEnabled(true),
      chainLightningTargets(3),
      fieldType(FieldType::STANDARD), 
      zapCallback(nullptr) {
    
    // Add more field types that can be selected
    const int randomFieldType = rand() % 100;
    if (randomFieldType < 25) {
        fieldType = FieldType::SHOCK;
        fieldColor = sf::Color(70, 130, 255, 50);
    } else if (randomFieldType < 50) {
        fieldType = FieldType::PLASMA;
        fieldColor = sf::Color(255, 90, 40, 50);
    } else if (randomFieldType < 75) {
        fieldType = FieldType::VORTEX;
        fieldColor = sf::Color(180, 70, 255, 50);
    } else {
        // For the standard type, create a more iridescent base color
        fieldColor = sf::Color(120, 140, 255, 50);
    }
    
    // Setup field shape with more dynamic effects
    fieldShape.setRadius(radius);
    fieldShape.setFillColor(fieldColor);
    
    // Create a more vibrant outline
    sf::Color outlineColor;
    switch(fieldType) {
        case FieldType::SHOCK:
            outlineColor = sf::Color(100, 210, 255, 180);
            break;
        case FieldType::PLASMA:
            outlineColor = sf::Color(255, 170, 90, 180);
            break;
        case FieldType::VORTEX:
            outlineColor = sf::Color(220, 130, 255, 180);
            break;
        default:
            outlineColor = sf::Color(160, 200, 255, 180);
    }
    
    fieldShape.setOutlineColor(outlineColor);
    fieldShape.setOutlineThickness(4.0f);
    fieldShape.setOrigin(radius, radius);
    
    // Setup secondary field effects with colorful variations
    for (int i = 0; i < NUM_FIELD_RINGS; i++) {
        fieldRings[i].setRadius(radius * (0.4f + 0.2f * i));
        fieldRings[i].setFillColor(sf::Color::Transparent);
        
        // Create colorful ring variations
        sf::Color ringColor;
        switch(fieldType) {
            case FieldType::SHOCK:
                // Electric blue with cyan accents
                ringColor = sf::Color(70 + i * 30, 170 + i * 20, 255, 120 - i * 20);
                break;
            case FieldType::PLASMA:
                // Fiery gradient from red to orange to yellow
                ringColor = sf::Color(255, 100 + i * 50, 50 + i * 40, 120 - i * 20);
                break;
            case FieldType::VORTEX:
                // Violet to magenta spectrum
                ringColor = sf::Color(180 + i * 20, 70 + i * 10, 255 - i * 30, 120 - i * 20);
                break;
            default:
                // Iridescent blue-green-cyan
                ringColor = sf::Color(120 + i * 10, 170 + i * 20, 255 - i * 10, 120 - i * 20);
        }
        
        fieldRings[i].setOutlineColor(ringColor);
        fieldRings[i].setOutlineThickness(2.0f + (NUM_FIELD_RINGS - i) * 0.5f); // Thicker inner rings
        fieldRings[i].setOrigin(radius * (0.4f + 0.2f * i), radius * (0.4f + 0.2f * i));
    }
    
    // Setup energy orbs that orbit the force field with more variety
    for (int i = 0; i < NUM_ENERGY_ORBS; i++) {
        float orbSize = 5.0f + (rand() % 10);
        energyOrbs[i].setRadius(orbSize);
        
        // Create varied orb colors based on position in sequence and field type
        sf::Color orbColor;
        int orbGroup = i % 3; // Create 3 different color groups for orbs
        
        switch(fieldType) {
            case FieldType::SHOCK:
                // Electric blue with white and cyan variations
                if (orbGroup == 0)
                    orbColor = sf::Color(220, 240, 255, 180); // Near white
                else if (orbGroup == 1)
                    orbColor = sf::Color(80, 180, 255, 180);  // Electric blue
                else
                    orbColor = sf::Color(0, 220, 230, 180);   // Cyan
                break;
                
            case FieldType::PLASMA:
                // Fire colors: yellow, orange, red
                if (orbGroup == 0)
                    orbColor = sf::Color(255, 230, 120, 180); // Yellow
                else if (orbGroup == 1)
                    orbColor = sf::Color(255, 140, 50, 180);  // Orange
                else
                    orbColor = sf::Color(255, 60, 30, 180);   // Red
                break;
                
            case FieldType::VORTEX:
                // Purple spectrum
                if (orbGroup == 0)
                    orbColor = sf::Color(230, 140, 255, 180); // Light purple
                else if (orbGroup == 1)
                    orbColor = sf::Color(180, 70, 255, 180);  // Medium purple
                else
                    orbColor = sf::Color(140, 0, 230, 180);   // Deep purple
                break;
                
            default:
                // Iridescent colors
                if (orbGroup == 0)
                    orbColor = sf::Color(180, 220, 255, 180); // Light blue
                else if (orbGroup == 1)
                    orbColor = sf::Color(130, 200, 220, 180); // Teal
                else
                    orbColor = sf::Color(180, 255, 220, 180); // Aqua
        }
        
        energyOrbs[i].setFillColor(orbColor);
        energyOrbs[i].setOrigin(orbSize, orbSize);
        
        // Varied orbit parameters
        orbAngles[i] = (float)(rand() % 360);
        orbSpeeds[i] = 1.0f + (rand() % 100) / 50.0f;
        
        // Create layered orbit distances
        if (i < NUM_ENERGY_ORBS / 3)
            orbDistances[i] = radius * 0.7f + (rand() % 20); // Inner orbit
        else if (i < 2 * NUM_ENERGY_ORBS / 3)
            orbDistances[i] = radius * 0.85f + (rand() % 20); // Middle orbit
        else
            orbDistances[i] = radius * 1.0f + (rand() % 20); // Outer orbit
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
    zapTimer = 0.5f;
    
    // Initialize with correct field color based on type
    updateFieldColor();
}

void ForceField::Update(float dt, PlayerManager& playerManager, EnemyManager& enemyManager) {
    // Skip if player is null or dead
    if (!player) {
        std::cerr << "[FORCEFIELD] Null player reference in Update" << std::endl;
        return;
    }
    
    if (player->IsDead()) {
        isZapping = false;
        return;
    }
    
    // Update the force field position to follow the player
    sf::Vector2f playerCenter = player->GetPosition() + sf::Vector2f(25.0f, 25.0f);
    fieldShape.setPosition(playerCenter);
    
    // Update field rotation for dynamic effect
    fieldRotation += dt * 15.0f * powf(fieldIntensity, 0.5f); // Rotation speed increases with intensity
    fieldPulsePhase += dt * 3.0f;
    
    // Rest of the update code...
    // ... 
    
    // Update cooldown timer - adjust for power level and charge
    float adjustedCooldown = zapCooldown * (1.0f - 0.1f * (powerLevel - 1)) * (1.0f - chargeLevel * 0.3f);
    zapTimer -= dt;
    if (zapTimer <= 0.0f) {
        try {
            // Time to zap a new enemy - wrapped in try-catch for safety
            FindAndZapEnemy(playerManager, enemyManager);
        } catch (const std::exception& e) {
            std::cerr << "[FORCEFIELD] Exception in FindAndZapEnemy: " << e.what() << std::endl;
        }
        zapTimer = adjustedCooldown;
    }
    
    // Field intensity increases during zapping and with charge level
    fieldIntensity = 1.0f + (isZapping ? 0.5f : 0.0f) + chargeLevel * 0.5f;
    
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
    if (!player) {
        std::cerr << "[ERROR] ForceField::FindAndZapEnemy called with null player pointer" << std::endl;
        return;
    }
    
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
        comboTimer = 3.0f; // 3 seconds to maintain combo
        
        // Increase charge level
        chargeLevel = std::min(1.0f, chargeLevel + 0.1f);
        
        // Chain lightning if enabled and we have multiple enemies
        if (chainLightningEnabled && enemiesInRange.size() > 1) {
            performChainLightning(enemyManager, playerCenter, closestEnemyId, closestEnemyPos, enemiesInRange);
        }
        
        // Notify through callback if it exists
        if (zapCallback) {
            try {
                zapCallback(closestEnemyId, effectiveDamage, killed);
            } catch (const std::exception& e) {
                std::cerr << "[ERROR] Exception in zap callback: " << e.what() << std::endl;
            }
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
    const int segments = 12 + powerLevel * 2;
    
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
            zapBaseColor = sf::Color(80, 180, 255, 255);
            zapBrightColor = sf::Color(180, 230, 255, 255);
            break;
        case FieldType::PLASMA:
            zapBaseColor = sf::Color(255, 150, 80, 255);
            zapBrightColor = sf::Color(255, 220, 180, 255);
            break;
        case FieldType::VORTEX:
            zapBaseColor = sf::Color(180, 80, 255, 255);
            zapBrightColor = sf::Color(220, 180, 255, 255);
            break;
        default:
            zapBaseColor = sf::Color(150, 220, 255, 255);
            zapBrightColor = sf::Color(200, 240, 255, 255);
    }
    
    // Create a jagged lightning effect with power level influencing complexity
    sf::Vector2f currentPos = start;
    for (int i = 0; i < segments; i++) {
        // Calculate next position along the line with random offset
        float t = (i + 1.0f) / segments;
        sf::Vector2f nextPos = start + (direction * t);
        
        // Add some randomness for zig-zag effect (more pronounced at higher power)
        if (i < segments - 1) {
            float offset = (rand() % 120 - 60) * (1.0f + powerLevel * 0.2f) / 2.0f;
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
        float thickness = 2.0f * (1.0f + 0.2f * powerLevel);
        sf::Vector2f offsetPerp = perpendicular * thickness;
        
        zapEffect.append(sf::Vertex(currentPos + offsetPerp, startColor));
        zapEffect.append(sf::Vertex(nextPos + offsetPerp, endColor));
        
        zapEffect.append(sf::Vertex(currentPos - offsetPerp, startColor));
        zapEffect.append(sf::Vertex(nextPos - offsetPerp, endColor));
        
        // Add branches based on power level
        int branchChance = 20 + powerLevel * 10; // 30-70% chance based on power
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
    float angleAdjust = (rand() % 60 - 30) * 3.14159f / 180.0f;
    float cosA = cos(angleAdjust);
    float sinA = sin(angleAdjust);
    branchDir = sf::Vector2f(
        branchDir.x * cosA - branchDir.y * sinA,
        branchDir.x * sinA + branchDir.y * cosA
    );
    
    // Normalize and scale branch direction
    float branchLen = mainDistance * (0.2f + (rand() % 100) / 500.0f) * (1.0f + 0.1f * powerLevel);
    float branchDirMag = std::hypot(branchDir.x, branchDir.y);
    branchDir = branchDir * (branchLen / branchDirMag);
    
    // Create the branch using 3-5 segments (more at higher power)
    sf::Vector2f branchPos = branchStart;
    int branchSegments = 3 + rand() % (1 + powerLevel);
    
    for (int j = 0; j < branchSegments; j++) {
        float bt = (j + 1.0f) / branchSegments;
        sf::Vector2f nextBranchPos = branchStart + (branchDir * bt);
        
        // Add randomness to branch
        nextBranchPos += sf::Vector2f(
            (rand() % 40 - 20) * (1.0f + 0.1f * powerLevel),
            (rand() % 40 - 20) * (1.0f + 0.1f * powerLevel)
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
        if (powerLevel >= 3 && j < branchSegments - 1 && rand() % 100 < 30) {
            sf::Vector2f subBranchDir(branchDir.y, -branchDir.x);
            if (rand() % 2 == 0) subBranchDir = -subBranchDir;
            
            float subLen = branchLen * 0.4f;
            subBranchDir = subBranchDir * (subLen / std::hypot(subBranchDir.x, subBranchDir.y));
            
            sf::Vector2f subBranchEnd = branchPos + subBranchDir;
            
            // Add randomness
            subBranchEnd += sf::Vector2f((rand() % 30 - 15), (rand() % 30 - 15));
            
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
    float damageMultiplier = 1.0f + 0.1f * (powerLevel - 1);
    float chainDamage = zapDamage * 0.6f * damageMultiplier; // Chain does 60% of primary damage
    
    for (size_t i = 0; i < chainTargets.size(); i++) {
        int enemyId = chainTargets[i].first;
        sf::Vector2f enemyPos = chainTargets[i].second;
        
        // Create lightning effect between previous target and this one
        createChainLightningEffect(prevPos, enemyPos);
        
        // Apply reduced damage to chained targets
        float targetDamage = chainDamage * (1.0f - 0.1f * i); // Damage falls off with each jump
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
            chainBaseColor = sf::Color(80, 180, 255, 200);
            chainBrightColor = sf::Color(180, 230, 255, 180);
            break;
        case FieldType::PLASMA:
            chainBaseColor = sf::Color(255, 150, 80, 200);
            chainBrightColor = sf::Color(255, 220, 180, 180);
            break;
        case FieldType::VORTEX:
            chainBaseColor = sf::Color(180, 80, 255, 200);
            chainBrightColor = sf::Color(220, 180, 255, 180);
            break;
        default:
            chainBaseColor = sf::Color(150, 220, 255, 200);
            chainBrightColor = sf::Color(200, 240, 255, 180);
    }
    
    // Create chain lightning with fewer segments than primary
    const int segments = 8 + powerLevel;
    sf::Vector2f currentPos = start;
    
    for (int i = 0; i < segments; i++) {
        // Calculate next position along the line with random offset
        float t = (i + 1.0f) / segments;
        sf::Vector2f nextPos = start + (direction * t);
        
        // Add some randomness for zig-zag effect
        if (i < segments - 1) {
            float offset = (rand() % 80 - 40) * (1.0f + powerLevel * 0.1f) / 2.0f;
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
        backgroundGlow.setRadius(20.0f + 5.0f * powerLevel);
        
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
        zapGlow.setRadius(8.0f + 2.0f * powerLevel);
        
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
        if (powerLevel >= 2) {
            sf::CircleShape sparkle;
            sparkle.setRadius(2.0f);
            sparkle.setOrigin(1.0f, 1.0f);
            
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
            int numSparkles = 10 + 5 * powerLevel;
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
                        (rand() % 20 - 10) * (1.0f + 0.2f * powerLevel),
                        (rand() % 20 - 10) * (1.0f + 0.2f * powerLevel)
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
        impactFlash.setRadius(15.0f + 5.0f * powerLevel);
        impactFlash.setOrigin(impactFlash.getRadius(), impactFlash.getRadius());
        impactFlash.setPosition(zapEndPosition);
        
        // Pulsing impact flash
        float flashPulse = 0.7f + 0.3f * std::sin(fieldPulsePhase * 8.0f);
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
        if (zapEffectTimer > zapEffectDuration * 0.7f) { // Only during initial part of effect
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
        if (consecutiveHits >= 3 || chargeLevel > 0.8f) {
            sf::CircleShape shockwave;
            float shockwaveProgress = 1.0f - (zapEffectTimer / zapEffectDuration);
            float shockwaveSize = 60.0f * shockwaveProgress * (1.0f + 0.2f * powerLevel);
            
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
        chainGlow.setRadius(5.0f + powerLevel);
        chainGlow.setOrigin(chainGlow.getRadius(), chainGlow.getRadius());
        
        sf::Color chainGlowColor;
        switch (fieldType) {
            case FieldType::SHOCK:
                chainGlowColor = sf::Color(100, 200, 255, 60);
                break;
            case FieldType::PLASMA:
                chainGlowColor = sf::Color(255, 150, 100, 60);
                break;
            case FieldType::VORTEX:
                chainGlowColor = sf::Color(200, 100, 255, 60);
                break;
            default:
                chainGlowColor = sf::Color(150, 200, 255, 60);
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

void ForceField::createAmbientParticle(const sf::Vector2f& center) {
    // Find an inactive particle
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!particles[i].active) {
            // Random angle and distance from center
            float angle = (rand() % 360) * 3.14159f / 180.0f;
            float distance = radius * (0.2f + 0.8f * (rand() % 100) / 100.0f);
            
            particles[i].active = true;
            particles[i].position = center + sf::Vector2f(
                cosf(angle) * distance,
                sinf(angle) * distance
            );
            
            // Random type with weights based on field type
            int typeRoll = rand() % 100;
            if (typeRoll < 70) { // 70% chance for ambient
                particles[i].type = ParticleType::AMBIENT;
                particles[i].velocity = sf::Vector2f(
                    (rand() % 20 - 10) * 5.0f,
                    (rand() % 20 - 10) * 5.0f
                );
                particles[i].baseSize = 1.0f + (rand() % 3);
                particles[i].size = particles[i].baseSize;
                particles[i].maxLifetime = 0.5f + (rand() % 100) / 100.0f;
            } else { // 30% chance for orbiting
                particles[i].type = ParticleType::ORBIT;
                particles[i].orbitAngle = angle;
                particles[i].orbitSpeed = (60.0f + rand() % 60) * (rand() % 2 == 0 ? 1.0f : -1.0f);
                particles[i].orbitDistance = distance;
                particles[i].baseSize = 2.0f + (rand() % 4);
                particles[i].size = particles[i].baseSize;
                particles[i].maxLifetime = 1.0f + (rand() % 200) / 100.0f;
            }
            
            particles[i].lifetime = particles[i].maxLifetime;
            
            // Color based on field type
            switch (fieldType) {
                case FieldType::SHOCK:
                    particles[i].color = sf::Color(100 + rand() % 100, 180 + rand() % 75, 255, 200);
                    break;
                case FieldType::PLASMA:
                    particles[i].color = sf::Color(255, 100 + rand() % 100, 50 + rand() % 100, 200);
                    break;
                case FieldType::VORTEX:
                    particles[i].color = sf::Color(150 + rand() % 100, 50 + rand() % 100, 255, 200);
                    break;
                default:
                    particles[i].color = sf::Color(150 + rand() % 100, 150 + rand() % 100, 255, 200);
            }
            
            // Only create one particle per call
            break;
        }
    }
}

void ForceField::createImpactParticles(const sf::Vector2f& impactPos) {
    // Create a burst of particles at impact position
    const int numParticles = 15 + powerLevel * 5;
    
    for (int j = 0; j < numParticles; j++) {
        // Find an inactive particle
        for (int i = 0; i < MAX_PARTICLES; i++) {
            if (!particles[i].active) {
                // Random angle
                float angle = (rand() % 360) * 3.14159f / 180.0f;
                float speed = 50.0f + (rand() % 150);
                
                particles[i].active = true;
                particles[i].type = ParticleType::IMPACT;
                particles[i].position = impactPos;
                particles[i].velocity = sf::Vector2f(
                    cosf(angle) * speed,
                    sinf(angle) * speed
                );
                
                particles[i].baseSize = 1.0f + (rand() % 4);
                particles[i].size = particles[i].baseSize;
                particles[i].maxLifetime = 0.3f + (rand() % 100) / 200.0f;
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

void ForceField::renderPowerIndicator(sf::RenderWindow& window, const sf::Vector2f& playerCenter) {
    // Only show indicator when charged or at higher power levels
    if (powerLevel <= 1 && chargeLevel < 0.1f) return;
    
    // Power level indicator as orbiting diamonds
    for (int i = 0; i < powerLevel; i++) {
        sf::CircleShape powerMarker(5.0f, 4); // Diamond shape
        powerMarker.setOrigin(5.0f, 5.0f);
        
        // Position in orbit around player
        float markerAngle = fieldRotation * 0.5f + (i * 360.0f / powerLevel);
        float markerDist = radius * 1.2f;
        sf::Vector2f markerPos = playerCenter + sf::Vector2f(
            cosf(markerAngle * 3.14159f / 180.0f) * markerDist,
            sinf(markerAngle * 3.14159f / 180.0f) * markerDist
        );
        
        powerMarker.setPosition(markerPos);
        
        // Color based on field type
        sf::Color markerColor;
        switch (fieldType) {
            case FieldType::SHOCK:
                markerColor = sf::Color(100, 200, 255, 200);
                break;
            case FieldType::PLASMA:
                markerColor = sf::Color(255, 150, 100, 200);
                break;
            case FieldType::VORTEX:
                markerColor = sf::Color(200, 100, 255, 200);
                break;
            default:
                markerColor = sf::Color(150, 220, 255, 200);
        }
        powerMarker.setFillColor(markerColor);
        
        // Pulsing size
        float pulseFactor = 1.0f + 0.3f * std::sin(fieldPulsePhase * 3.0f + i * 0.5f);
        powerMarker.setScale(pulseFactor, pulseFactor);
        
        window.draw(powerMarker);
    }
    
    // Charge level indicator
    if (chargeLevel > 0.05f) {
        sf::RectangleShape chargeBar;
        float chargeWidth = 50.0f * chargeLevel;
        chargeBar.setSize(sf::Vector2f(chargeWidth, 4.0f));
        chargeBar.setPosition(playerCenter.x - 25.0f, playerCenter.y - radius * 1.3f);
        
        // Color based on charge level and field type
        sf::Color chargeColor;
        switch (fieldType) {
            case FieldType::SHOCK:
                chargeColor = sf::Color(100, 150 + static_cast<int>(105 * chargeLevel), 255, 200);
                break;
            case FieldType::PLASMA:
                chargeColor = sf::Color(255, 100 + static_cast<int>(155 * chargeLevel), 50, 200);
                break;
            case FieldType::VORTEX:
                chargeColor = sf::Color(150 + static_cast<int>(105 * chargeLevel), 50, 255, 200);
                break;
            default:
                chargeColor = sf::Color(100 + static_cast<int>(155 * chargeLevel), 200, 255, 200);
        }
        chargeBar.setFillColor(chargeColor);
        
        // Pulsing opacity for high charge
        if (chargeLevel > 0.8f) {
            float pulseAlpha = 150 + 105 * std::sin(fieldPulsePhase * 5.0f);
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
            newBaseColor = sf::Color(50, 150, 255, 50);
            newOutlineColor = sf::Color(100, 200, 255, 180);
            break;
        case FieldType::PLASMA:
            newBaseColor = sf::Color(255, 100, 50, 50);
            newOutlineColor = sf::Color(255, 150, 100, 180);
            break;
        case FieldType::VORTEX:
            newBaseColor = sf::Color(150, 50, 255, 50);
            newOutlineColor = sf::Color(200, 100, 255, 180);
            break;
        default:
            newBaseColor = sf::Color(100, 100, 255, 50);
            newOutlineColor = sf::Color(150, 150, 255, 180);
    }
    
    // Adjust based on field intensity
    newBaseColor.a = static_cast<sf::Uint8>(newBaseColor.a * (0.7f + 0.3f * fieldIntensity));
    newOutlineColor.a = static_cast<sf::Uint8>(newOutlineColor.a * (0.7f + 0.3f * fieldIntensity));
    
    // Apply updated colors
    fieldShape.setFillColor(newBaseColor);
    fieldShape.setOutlineColor(newOutlineColor);
    
    // Also update the field base color for future reference
    fieldColor = newBaseColor;
}