#include "Player.h"

#include <iostream>
#include <cmath>

Player::Player()
    : movementSpeed(PLAYER_SPEED), shootCooldown(0.f), health(PLAYER_HEALTH), isDead(false), respawnPosition(0.f, 0.f),
      bulletSpeedMultiplier(1.0f), moveSpeedMultiplier(1.0f), maxHealth(PLAYER_HEALTH), bulletDamage(BULLET_DAMAGE),
      shootCooldownDuration(SHOOT_COOLDOWN_DURATION) {
    shape.setSize(sf::Vector2f(50.f, 50.f));
    shape.setFillColor(sf::Color::Blue);
    shape.setPosition(100.f, 100.f);
}

Player::Player(const sf::Vector2f& startPosition, const sf::Color& color)
    : movementSpeed(PLAYER_SPEED), shootCooldown(0.f), health(PLAYER_HEALTH), isDead(false), respawnPosition(startPosition),
      bulletSpeedMultiplier(1.0f), moveSpeedMultiplier(1.0f), maxHealth(PLAYER_HEALTH), bulletDamage(BULLET_DAMAGE),
      shootCooldownDuration(SHOOT_COOLDOWN_DURATION) {
    shape.setSize(sf::Vector2f(50.f, 50.f));
    shape.setFillColor(color);
    shape.setPosition(startPosition);
}

Player::Player(Player&& other) noexcept
    : shape(std::move(other.shape)),
      movementSpeed(other.movementSpeed),
      moveSpeedMultiplier(other.moveSpeedMultiplier),
      shootCooldown(other.shootCooldown),
      bulletSpeedMultiplier(other.bulletSpeedMultiplier),
      bulletDamage(other.bulletDamage),
      health(other.health),
      maxHealth(other.maxHealth),
      isDead(other.isDead),
      respawnPosition(other.respawnPosition),
      forceField(std::move(other.forceField)),
      forceFieldEnabled(other.forceFieldEnabled),
      shootCooldownDuration(other.shootCooldownDuration) {
}

Player& Player::operator=(Player&& other) noexcept {
    if (this != &other) {
        shape = std::move(other.shape);
        movementSpeed = other.movementSpeed;
        moveSpeedMultiplier = other.moveSpeedMultiplier;
        shootCooldown = other.shootCooldown;
        bulletSpeedMultiplier = other.bulletSpeedMultiplier;
        bulletDamage = other.bulletDamage;
        health = other.health;
        maxHealth = other.maxHealth;
        isDead = other.isDead;
        respawnPosition = other.respawnPosition;
        forceField = std::move(other.forceField);
        forceFieldEnabled = other.forceFieldEnabled;
        shootCooldownDuration = other.shootCooldownDuration;
    }
    return *this;
}

void Player::Update(float dt) {
    // Only handle cooldowns in the base update
    if (shootCooldown > 0.f) {
        shootCooldown -= dt;
    }
}

void Player::Update(float dt, const InputManager& inputManager) {
    // First run the base update to handle cooldowns
    Update(dt);
    
    // Then handle input-based movement
    sf::Vector2f movement(0.f, 0.f);
    
    // Skip movement if player is dead
    if (isDead) return;
    
    // Use the input manager to check for key presses using the configured bindings
    if (sf::Keyboard::isKeyPressed(inputManager.GetKeyBinding(GameAction::MoveUp)))
        movement.y -= movementSpeed * moveSpeedMultiplier * dt;
    if (sf::Keyboard::isKeyPressed(inputManager.GetKeyBinding(GameAction::MoveDown)))
        movement.y += movementSpeed * moveSpeedMultiplier * dt;
    if (sf::Keyboard::isKeyPressed(inputManager.GetKeyBinding(GameAction::MoveLeft)))
        movement.x -= movementSpeed * moveSpeedMultiplier * dt;
    if (sf::Keyboard::isKeyPressed(inputManager.GetKeyBinding(GameAction::MoveRight)))
        movement.x += movementSpeed * moveSpeedMultiplier * dt;
    
    shape.move(movement);
}

Player::BulletParams Player::Shoot(const sf::Vector2f& mouseWorldPos) {
    BulletParams params{};
    
    // Skip shooting if player is dead
    if (isDead) {
        params.success = false;
        return params;
    }
    
    if (shootCooldown <= 0.f) {
        shootCooldown = shootCooldownDuration;

        // Calculate the player's center position rather than top-left
        sf::Vector2f playerCenter = GetPosition() + sf::Vector2f(shape.getSize().x / 2.f, shape.getSize().y / 2.f);
        
        // Calculate direction from player center to mouse
        sf::Vector2f direction = mouseWorldPos - playerCenter;
        float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
        if (length > 0.f) {
            direction /= length;  // Normalize direction
        } else {
            direction = sf::Vector2f(1.f, 0.f);  // Default right if mouse is on player
        }

        // Set bullet start position at player center
        params.position = playerCenter;
        params.direction = direction;
        params.success = true; // Indicate successful shot

    } else {
        // If on cooldown, return default/invalid params with success = false
        params.position = GetPosition();
        params.direction = sf::Vector2f(0.f, 0.f);
        params.success = false;
    }

    return params;
}

Player::BulletParams Player::AttemptShoot(const sf::Vector2f& mouseWorldPos) {
    // Skip if player is dead
    if (isDead) {
        BulletParams params{};
        params.success = false;
        return params;
    }
    
    // Get the current player position
    sf::Vector2f playerPos = GetPosition();
    sf::Vector2f playerCenter = playerPos + sf::Vector2f(shape.getSize().x / 2.f, shape.getSize().y / 2.f);
    
    // Calculate direction vector from player to mouse position
    sf::Vector2f direction = mouseWorldPos - playerCenter;
    
    // Normalize the direction vector
    float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
    if (length > 0) {
        direction /= length;
    } else {
        direction = sf::Vector2f(1.f, 0.f);  // Default right if mouse is on player
    }
    
    // Call the Shoot method which handles cooldown checks
    shootCooldown = 0.f; // Force cooldown to be ready for this call
    return Shoot(mouseWorldPos);
}

sf::Vector2f Player::GetPosition() const {
    return shape.getPosition();
}

void Player::SetPosition(const sf::Vector2f& pos) {
    shape.setPosition(pos);
}

sf::RectangleShape& Player::GetShape() {
    return shape;
}

const sf::RectangleShape& Player::GetShape() const {
    return shape;
}

float Player::GetShootCooldown() const {
    return shootCooldown;
}

void Player::SetSpeed(float speed) {
    movementSpeed = speed;
}

float Player::GetSpeed() const {
    return movementSpeed;
}

void Player::TakeDamage(int amount) {
    // Skip if already dead
    if (isDead) return;
    
    health -= amount;
    if (health <= 0) {
        health = 0;
        isDead = true;
        // Only save respawn position if it's not already set
        if (respawnPosition.x == 0.f && respawnPosition.y == 0.f) {
            respawnPosition = shape.getPosition();
        }
    }
}

void Player::SetHealth(float newHealth) {
    health = std::min(newHealth, maxHealth);
    isDead = (health <= 0);
}

bool Player::IsDead() const {
    return isDead;
}

void Player::Respawn() {
    health = maxHealth; // Use maxHealth instead of constant
    isDead = false;
    // Move player to their respawn position
    shape.setPosition(respawnPosition);
}

void Player::SetRespawnPosition(const sf::Vector2f& position) {
    respawnPosition = position;
}

sf::Vector2f Player::GetRespawnPosition() const {
    return respawnPosition;
}

void Player::InitializeForceField(GameSettingsManager* settingsManager) {
    try {
        // If we already have a force field, don't recreate it
        if (forceField) {
            std::cout << "[PLAYER] Force field already exists, not reinitializing" << std::endl;
            forceFieldEnabled = true;
            return;
        }
        
        // Start with a smaller radius based on settings or default
        float startingRadius = 100.0f;  // Smaller than the default 150.0f
        
        // Get radius from settings if available
        if (settingsManager) {
            GameSetting* radiusSetting = settingsManager->GetSetting("forcefield_radius");
            if (radiusSetting) {
                startingRadius = radiusSetting->GetFloatValue() * 0.7f; // 70% of default size
            }
        }
        
        // Create the force field with settings
        forceField = std::make_unique<ForceField>(this, startingRadius, settingsManager);
        
        // Set initial properties to be weaker than default
        if (forceField) {
            // Reduced damage (either from settings or default)
            float baseDamage = ForceField::GetDefaultDamage(settingsManager);
            forceField->SetDamage(baseDamage * 0.6f);  // 60% of the base damage
            
            // Slower firing rate
            float baseCooldown = ForceField::GetDefaultCooldown(settingsManager);
            forceField->SetCooldown(baseCooldown * 1.5f);  // 50% slower than default
            
            // Disable chain lightning initially (player will unlock with upgrades)
            forceField->SetChainLightningEnabled(false);
            forceField->SetChainLightningTargets(1);
            
            // Set to lowest power level
            forceField->SetPowerLevel(1);
            
            // Standard field type initially
            forceField->SetFieldType(FieldType::STANDARD);
        } else {
            std::cerr << "[PLAYER] Failed to create ForceField - null after initialization" << std::endl;
            return;
        }
        
        forceFieldEnabled = true;
        
    } catch (const std::exception& e) {
        std::cerr << "[PLAYER] Exception in InitializeForceField: " << e.what() << std::endl;
        forceFieldEnabled = false;
        forceField.reset(); // Clean up any partially initialized field
    }
}

void Player::EnableForceField(bool enable) {
    // Only change state if we have a force field
    if (forceField) {
        bool previousState = forceFieldEnabled;
        forceFieldEnabled = enable;
        
        // Create a visual pulse effect when toggling to enabled
        if (enable && !previousState) {
            // Make it pulse by temporarily increasing size
            float originalRadius = forceField->GetRadius();
            forceField->SetRadius(originalRadius * 1.2f); // 20% larger
            
            // Return to normal size after 0.2 seconds
            sf::Clock timer;
            sf::Time duration = sf::seconds(0.2f);
            
            // This is a simple approach; in a real implementation,
            // you'd want to add this to a queue of effects to apply during update
            while (timer.getElapsedTime() < duration) {
                // Wait for the pulse effect
                sf::sleep(sf::milliseconds(10));
            }
            
            // Return to normal size
            forceField->SetRadius(originalRadius);
        }
    }
}

void Player::ApplySettings(GameSettingsManager* settingsManager) {
    if (!settingsManager) return;
    
    // Apply player settings
    GameSetting* playerSpeedSetting = settingsManager->GetSetting("player_speed");
    if (playerSpeedSetting) {
        movementSpeed = playerSpeedSetting->GetFloatValue();
    }
    
    GameSetting* playerHealthSetting = settingsManager->GetSetting("player_health");
    if (playerHealthSetting) {
        // Only update max health if the player is at full health or hasn't taken damage
        if (health == maxHealth) {
            maxHealth = playerHealthSetting->GetFloatValue();
            health = maxHealth; // Update current health as well
        } else {
            // If player has taken damage, keep the same proportion of health
            float healthRatio = health / maxHealth;
            maxHealth = playerHealthSetting->GetFloatValue();
            health = maxHealth * healthRatio;
        }
    }
    
    
    // Bullet settings
    GameSetting* bulletDamageSetting = settingsManager->GetSetting("bullet_damage");
    if (bulletDamageSetting) {
        bulletDamage = bulletDamageSetting->GetFloatValue();
    }
    
    GameSetting* bulletSpeedSetting = settingsManager->GetSetting("bullet_speed");
    if (bulletSpeedSetting) {
        // Update the base bullet speed - this affects new bullets only
        // The actual speed will be this base value * bulletSpeedMultiplier
    }
    
    // Shop multipliers from settings
    GameSetting* moveSpeedMultiplierSetting = settingsManager->GetSetting("shop_move_speed_multiplier");
    if (moveSpeedMultiplierSetting) {
        // This would be applied when the player purchases an upgrade
        // Not directly setting it here as it's controlled by the shop system
    }
    
    GameSetting* bulletSpeedMultiplierSetting = settingsManager->GetSetting("shop_bullet_speed_multiplier");
    if (bulletSpeedMultiplierSetting) {
        // Similar to move speed multiplier, controlled by shop
    }
    
    // Collision settings
    GameSetting* collisionRadiusSetting = settingsManager->GetSetting("collision_radius");
    if (collisionRadiusSetting) {
        // Update the player's collision radius/hitbox if needed
        // This might affect the shape size or separate collision detection
    }
    
    // Apply force field settings if it exists
    if (forceField && forceFieldEnabled) {
        forceField->ApplySettings(settingsManager);
    }
}