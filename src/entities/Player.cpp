#include "Player.h"
#include <iostream>
#include <cmath>

Player::Player()
    : movementSpeed(200.f), shootCooldown(0.f), health(PLAYER_HEALTH), isDead(false), respawnPosition(0.f, 0.f),
      bulletSpeedMultiplier(1.0f), moveSpeedMultiplier(1.0f), maxHealth(PLAYER_HEALTH) {
    shape.setSize(sf::Vector2f(50.f, 50.f));
    shape.setFillColor(sf::Color::Blue);
    shape.setPosition(100.f, 100.f);
}

Player::Player(const sf::Vector2f& startPosition, const sf::Color& color)
    : movementSpeed(200.f), shootCooldown(0.f), health(PLAYER_HEALTH), isDead(false), respawnPosition(startPosition),
      bulletSpeedMultiplier(1.0f), moveSpeedMultiplier(1.0f), maxHealth(PLAYER_HEALTH) {
    shape.setSize(sf::Vector2f(50.f, 50.f));
    shape.setFillColor(color);
    shape.setPosition(startPosition);
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
    health = PLAYER_HEALTH;
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