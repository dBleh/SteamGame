#include "ForceField.h"
#include "Player.h"
#include "PlayerManager.h"
#include "enemies/EnemyManager.h"
#include "enemies/Enemy.h"
#include "../core/Game.h"
#include <cmath>
#include <iostream>
#include <random>

ForceField::ForceField(Player* player, float radius)
    : player(player),
      radius(radius),
      zapTimer(0.0f),
      zapCooldown(DEFAULT_COOLDOWN),
      zapDamage(DEFAULT_DAMAGE),
      targetEnemyId(-1),
      isZapping(false),
      zapEffectDuration(0.3f),  // Zap effect lasts 0.3 seconds
      zapEffectTimer(0.0f) {
    
    // Setup field shape with more visible effects
    fieldShape.setRadius(radius);
    fieldShape.setFillColor(sf::Color(100, 100, 255, 50)); // Light blue, semi-transparent
    fieldShape.setOutlineColor(sf::Color(150, 150, 255, 150)); // More visible blue outline
    fieldShape.setOutlineThickness(3.0f); // Thicker outline
    fieldShape.setOrigin(radius, radius); // Center origin
    
    // Setup zap effect as a line strip
    zapEffect.setPrimitiveType(sf::Lines);
    zapEffect.resize(0); // Start with no lines
    
    // Fire initial zap quickly
    zapTimer = 0.5f; // Start with a short cooldown
}

void ForceField::Update(float dt, PlayerManager& playerManager, EnemyManager& enemyManager) {
    // Skip if player is dead
    if (player->IsDead()) {
        isZapping = false;
        return;
    }
    
    // Update the force field position to follow the player
    sf::Vector2f playerCenter = player->GetPosition() + sf::Vector2f(25.0f, 25.0f); // Assuming player is 50x50
    fieldShape.setPosition(playerCenter);
    
    // Update zap effect timer
    if (isZapping) {
        zapEffectTimer -= dt;
        if (zapEffectTimer <= 0.0f) {
            isZapping = false;
            zapEffect.resize(0); // Clear zap effect
        }
    }
    
    // Update cooldown timer
    zapTimer -= dt;
    if (zapTimer <= 0.0f) {
        // Time to zap a new enemy
        FindAndZapEnemy(playerManager, enemyManager);
        zapTimer = zapCooldown; // Reset cooldown
    }
    
    // Quick debug check to ensure method is being called
    static bool debugPrinted = false;
    static int updateCount = 0;
    updateCount++;
    if (!debugPrinted && updateCount > 50) {
        std::cout << "[ForceField] Update method is being called, enemyCount: " 
                  << enemyManager.GetEnemyCount() << std::endl;
        debugPrinted = true;
    }
}

void ForceField::Render(sf::RenderWindow& window) {
    // Skip if player is dead
    if (player->IsDead()) return;
    
    // Get player position and center the force field on the player
    sf::Vector2f playerCenter = player->GetPosition() + sf::Vector2f(25.0f, 25.0f); // Assuming player is 50x50
    fieldShape.setPosition(playerCenter);
    
    // Debugging - print each second 
    static int frameCount = 0;
    if (frameCount++ % 60 == 0) {  // Assuming ~60 fps, print once per second
        std::cout << "[ForceField] DEBUG: Render called at position: (" 
                << playerCenter.x << ", " << playerCenter.y 
                << ") with radius: " << radius << std::endl;
    }
    
    // Add pulsing effect to make force field more visible
    static float pulseTimer = 0.0f;
    static float originalAlpha = fieldShape.getFillColor().a;
    pulseTimer += 0.05f;  // Increment timer
    
    // Calculate pulsing alpha (oscillate between 50% and 100% of original alpha)
    float pulseRatio = 0.5f + 0.5f * std::sin(pulseTimer * 2.0f);
    sf::Color pulseColor = fieldShape.getFillColor();
    pulseColor.a = static_cast<sf::Uint8>(originalAlpha * (0.5f + 0.5f * pulseRatio));
    fieldShape.setFillColor(pulseColor);
    
    // Render force field
    window.draw(fieldShape);
    
    // Render zap effect if active
    if (isZapping && zapEffect.getVertexCount() > 0) {
        // Add some glow effect to make zaps more visible
        sf::CircleShape zapGlow;
        zapGlow.setRadius(10.0f);
        zapGlow.setFillColor(sf::Color(150, 200, 255, 150));
        zapGlow.setOrigin(10.0f, 10.0f);
        
        // Place glows at each vertex of the zap effect
        for (size_t i = 0; i < zapEffect.getVertexCount(); i++) {
            zapGlow.setPosition(zapEffect[i].position);
            window.draw(zapGlow);
        }
        
        // Draw the actual zap effect
        window.draw(zapEffect);
    }
}

void ForceField::FindAndZapEnemy(PlayerManager& playerManager, EnemyManager& enemyManager) {
    // Get player position
    sf::Vector2f playerCenter = player->GetPosition() + sf::Vector2f(25.0f, 25.0f); // Assuming player is 50x50
    
    // Find closest enemy within range
    float closestDistanceSquared = radius * radius;
    int closestEnemyId = -1;
    sf::Vector2f closestEnemyPos;
    
    // Log enemy count
    int enemyCount = enemyManager.GetEnemyCount();
    std::cout << "[ForceField] Looking for enemies to zap. Enemy count: " << enemyCount << std::endl;
    
    // Direct method: Loop through enemies and check distance
    // This requires EnemyManager to expose a way to iterate through enemies
    
    // Alternative: Use collision detection
    // This approach uses a more direct method of checking enemy positions within the radius
    
    for (int i = 0; i < 200; i++) {  // Try a reasonable number of random points
        // Generate random point within field radius
        float angle = (float)(rand() % 360) * 3.14159f / 180.0f;
        float distance = (float)(rand() % (int)radius);
        
        sf::Vector2f checkPos = playerCenter + sf::Vector2f(
            cosf(angle) * distance,
            sinf(angle) * distance
        );
        
        int enemyId = -1;
        // Use the existing bullet collision check to find enemies
        if (enemyManager.CheckBulletCollision(checkPos, 10.0f, enemyId)) {
            if (enemyId != -1) {
                sf::Vector2f enemyPos;
                
                // Get enemy position
                Enemy* enemy = enemyManager.FindEnemy(enemyId);
                if (enemy && !enemy->IsDead()) {
                    enemyPos = enemy->GetPosition();
                    float distSquared = std::pow(enemyPos.x - playerCenter.x, 2) + 
                                      std::pow(enemyPos.y - playerCenter.y, 2);
                    
                    if (distSquared < closestDistanceSquared) {
                        closestDistanceSquared = distSquared;
                        closestEnemyId = enemyId;
                        closestEnemyPos = enemyPos;
                    }
                }
            }
        }
    }
    
    // If we found an enemy, zap it
    if (closestEnemyId != -1) {
        std::cout << "[ForceField] Found enemy to zap with ID: " << closestEnemyId << std::endl;
        
        // Apply damage
        bool killed = enemyManager.InflictDamage(closestEnemyId, zapDamage);
        
        // Notify through callback
        if (zapCallback) {
            zapCallback(closestEnemyId, zapDamage, killed);
        }
        
        // Show zap effect
        CreateZapEffect(playerCenter, closestEnemyPos);
        
        // Save target info
        targetEnemyId = closestEnemyId;
        zapEndPosition = closestEnemyPos;
        isZapping = true;
        zapEffectTimer = zapEffectDuration;
    } else {
        std::cout << "[ForceField] No enemy found within range to zap" << std::endl;
    }
}

void ForceField::CreateZapEffect(const sf::Vector2f& start, const sf::Vector2f& end) {
    // Clear existing effect
    zapEffect.clear();
    
    // Number of line segments for the zap
    const int segments = 12;  // Increased from 8 for more detail
    
    // Direction vector
    sf::Vector2f direction = end - start;
    float distance = std::hypot(direction.x, direction.y);
    
    if (distance < 0.001f) return; // Prevent division by zero
    
    // Normalized perpendicular vector for zig-zag
    sf::Vector2f perpendicular(-direction.y / distance, direction.x / distance);
    
    // Create a jagged lightning effect
    sf::Vector2f currentPos = start;
    for (int i = 0; i < segments; i++) {
        // Calculate next position along the line with random offset
        float t = (i + 1.0f) / segments;
        sf::Vector2f nextPos = start + (direction * t);
        
        // Add some randomness for zig-zag effect (more pronounced)
        if (i < segments - 1) {  // Don't offset the final segment
            float offset = (rand() % 120 - 60) / 2.0f;  // Increased range for more dramatic zag
            nextPos += perpendicular * offset;
        }
        
        // Add two parallel lines for a thicker effect
        float thickness = 2.0f;
        sf::Vector2f offsetPerp = perpendicular * thickness;
        
        // Add line segment to the zap effect
        // Bright electric blue color with fade-out
        sf::Color startColor(150, 220, 255, 255 - (i * 15));  // Brighter blue
        sf::Color endColor(200, 240, 255, 255 - (i * 15));    // Near white-blue
        
        // Main line
        zapEffect.append(sf::Vertex(currentPos, startColor));
        zapEffect.append(sf::Vertex(nextPos, endColor));
        
        // Add occasional branch for a more realistic lightning effect
        if (i > 0 && i < segments - 2 && rand() % 3 == 0) {  // 33% chance of branch
            sf::Vector2f branchStart = currentPos;
            sf::Vector2f branchDir(direction.y, -direction.x);  // Perpendicular to main direction
            // Randomize branch direction
            if (rand() % 2 == 0) {
                branchDir = -branchDir;
            }
            
            // Normalize and scale branch direction
            float branchLen = distance * (0.2f + (rand() % 100) / 500.0f);  // 20-40% of main length
            branchDir /= std::hypot(branchDir.x, branchDir.y);
            branchDir *= branchLen;
            
            // Create the branch using 3-4 segments
            sf::Vector2f branchPos = branchStart;
            int branchSegments = 3 + rand() % 2;
            for (int j = 0; j < branchSegments; j++) {
                float bt = (j + 1.0f) / branchSegments;
                sf::Vector2f nextBranchPos = branchStart + (branchDir * bt);
                
                // Add randomness to branch
                nextBranchPos += sf::Vector2f(
                    (rand() % 40 - 20),
                    (rand() % 40 - 20)
                );
                
                // Add branch segment
                sf::Color branchColor(180, 230, 255, 200 - (j * 40));  // Brighter, more transparent
                zapEffect.append(sf::Vertex(branchPos, branchColor));
                zapEffect.append(sf::Vertex(nextBranchPos, sf::Color(200, 240, 255, 150 - (j * 40))));
                
                branchPos = nextBranchPos;
            }
        }
        
        currentPos = nextPos;
    }
    
    // Debug output
    std::cout << "[ForceField] Created zap effect with " << zapEffect.getVertexCount() 
              << " vertices from (" << start.x << "," << start.y << ") to (" 
              << end.x << "," << end.y << ")" << std::endl;
}