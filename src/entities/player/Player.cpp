#include "Player.h"

#include <iostream>
#include <cmath>

Player::Player()
    : movementSpeed(PLAYER_DEFAULT_MOVE_SPEED), shootCooldown(0.f), health(PLAYER_HEALTH), isDead(false), respawnPosition(0.f, 0.f),
      bulletSpeedMultiplier(1.0f), moveSpeedMultiplier(1.0f), maxHealth(PLAYER_HEALTH) {
    shape.setSize(sf::Vector2f(PLAYER_WIDTH, PLAYER_HEIGHT));
    shape.setFillColor(PLAYER_DEFAULT_COLOR);
    shape.setPosition(PLAYER_DEFAULT_START_X, PLAYER_DEFAULT_START_Y);
}

Player::Player(const sf::Vector2f& startPosition, const sf::Color& color)
    : movementSpeed(PLAYER_DEFAULT_MOVE_SPEED), shootCooldown(0.f), health(PLAYER_HEALTH), isDead(false), respawnPosition(startPosition),
      bulletSpeedMultiplier(1.0f), moveSpeedMultiplier(1.0f), maxHealth(PLAYER_HEALTH) {
    shape.setSize(sf::Vector2f(PLAYER_WIDTH, PLAYER_HEIGHT));
    shape.setFillColor(color);
    shape.setPosition(startPosition);
}

Player::Player(Player&& other) noexcept
    : shape(std::move(other.shape)),
      movementSpeed(other.movementSpeed),
      moveSpeedMultiplier(other.moveSpeedMultiplier),
      shootCooldown(other.shootCooldown),
      bulletSpeedMultiplier(other.bulletSpeedMultiplier),
      health(other.health),
      maxHealth(other.maxHealth),
      isDead(other.isDead),
      respawnPosition(other.respawnPosition),
      forceField(std::move(other.forceField)),
      forceFieldEnabled(other.forceFieldEnabled) {
}

Player& Player::operator=(Player&& other) noexcept {
    if (this != &other) {
        shape = std::move(other.shape);
        movementSpeed = other.movementSpeed;
        moveSpeedMultiplier = other.moveSpeedMultiplier;
        shootCooldown = other.shootCooldown;
        bulletSpeedMultiplier = other.bulletSpeedMultiplier;
        health = other.health;
        maxHealth = other.maxHealth;
        isDead = other.isDead;
        respawnPosition = other.respawnPosition;
        forceField = std::move(other.forceField);
        forceFieldEnabled = other.forceFieldEnabled;
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
        shootCooldown = SHOOT_COOLDOWN_DURATION;

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
    // Call the overloaded version with empty attacker ID
    TakeDamage(amount, "");
}

void Player::TakeDamage(int amount, const std::string& attackerID) {
    // Skip if already dead
    if (isDead) return;
    
    // Store the attacker ID for death callback
    if (!attackerID.empty()) {
        lastAttackerID = attackerID;
    }
    
    int oldHealth = health;
    health -= amount;
    
    // Call damage callback if set
    if (onDamage && amount > 0) {
        onDamage(playerID, amount, oldHealth - health);
    }
    
    if (health <= 0) {
        health = 0;
        isDead = true;
        
        // Only save respawn position if it's not already set
        if (respawnPosition.x == 0.f && respawnPosition.y == 0.f) {
            respawnPosition = shape.getPosition();
        }
        
        // Call death callback if set, passing the lastAttackerID
        if (onDeath) {
            onDeath(playerID, shape.getPosition(), lastAttackerID);
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
    health = PLAYER_HEALTH;
    isDead = false;
    // Move player to their respawn position
    shape.setPosition(respawnPosition);
    
    // Call respawn callback if set
    if (onRespawn) {
        onRespawn(playerID, respawnPosition);
    }
}

void Player::SetRespawnPosition(const sf::Vector2f& position) {
    respawnPosition = position;
}

sf::Vector2f Player::GetRespawnPosition() const {
    return respawnPosition;
}

void Player::InitializeForceField() {
    // Initialize with default values from config
    forceField = std::make_unique<ForceField>(this, DEFAULT_RADIUS);
    
    // Set initial properties appropriate for starting level
    if (forceField) {
        forceField->SetDamage(DEFAULT_DAMAGE);
        forceField->SetCooldown(DEFAULT_COOLDOWN);
        forceField->SetChainLightningEnabled(false);
        forceField->SetChainLightningTargets(FIELD_DEFAULT_CHAIN_TARGETS);
        forceField->SetPowerLevel(DEFAULT_POWER_LEVEL);
        forceField->SetFieldType(FieldType::STANDARD);
    }
    
    forceFieldEnabled = true;
}

void Player::SetForceFieldZapCallback(const std::function<void(int, float, bool)>& callback) {
    if (forceField) {
        forceField->SetZapCallback(callback);
    }
}

bool Player::CheckBulletCollision(const sf::Vector2f& bulletPos, float bulletRadius) const {
    if (isDead) return false;
    
    // Get player bounds
    sf::FloatRect playerBounds = shape.getGlobalBounds();
    
    // Calculate distance between bullet and player center
    sf::Vector2f playerCenter(
        playerBounds.left + playerBounds.width / 2.0f,
        playerBounds.top + playerBounds.height / 2.0f
    );
    
    float distX = bulletPos.x - playerCenter.x;
    float distY = bulletPos.y - playerCenter.y;
    float distSquared = distX * distX + distY * distY;
    
    // Use approximate circle-rectangle collision
    float combinedRadius = bulletRadius + std::min(playerBounds.width, playerBounds.height) / 2.0f;
    return distSquared <= (combinedRadius * combinedRadius);
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
            forceField->SetRadius(originalRadius * FIELD_PULSE_FACTOR);
            
            // Return to normal size after pulse duration
            sf::Clock timer;
            sf::Time duration = sf::seconds(FIELD_PULSE_DURATION);
            
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

void Player::Die(const sf::Vector2f& deathPosition) {
    // Only process if not already dead
    if (!isDead) {
        health = 0;
        isDead = true;
        respawnTimer = RESPAWN_TIME;
        isRespawning = true;
        
        // Save respawn position if not already set
        if (respawnPosition.x == 0.f && respawnPosition.y == 0.f) {
            respawnPosition = deathPosition;
        }
    }
}

void Player::UpdateRespawn(float dt) {
    // Only update timer if player is dead and respawning
    if (isDead && isRespawning) {
        respawnTimer -= dt;
        
        if (respawnTimer <= 0.0f) {
            // Time to respawn
            Respawn();
            isRespawning = false;
        }
    }
}