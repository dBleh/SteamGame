#include "Player.h"
#include <iostream>
#include <cmath>

Player::Player()
    : movementSpeed(200.f), shootCooldown(0.f), health(PLAYER_HEALTH), isDead(false), respawnPosition(0.f, 0.f) {
    shape.setSize(sf::Vector2f(50.f, 50.f));
    shape.setFillColor(sf::Color::Blue);
    shape.setPosition(100.f, 100.f);
}

Player::Player(const sf::Vector2f& startPosition, const sf::Color& color)
    : movementSpeed(200.f), shootCooldown(0.f), health(PLAYER_HEALTH), isDead(false), respawnPosition(0.f, 0.f) {
    shape.setSize(sf::Vector2f(50.f, 50.f));
    shape.setFillColor(color);
    shape.setPosition(startPosition);
}
// In src/entities/Player.cpp, replace the Update() method with this version
// that properly uses InputManager instead of hardcoded keys

void Player::Update(float dt) {
    // This method is being replaced with the InputManager version
    // We'll forward to a default implementation that doesn't move
    // to prevent crashes when called without proper input handling
    
    // Only handle cooldowns
    if (shootCooldown > 0.f) {
        shootCooldown -= dt;
    }
}

void Player::Update(float dt, const InputManager& inputManager) {
    sf::Vector2f movement(0.f, 0.f);
    
    // Use the input manager to check for key presses using the configured bindings
    if (sf::Keyboard::isKeyPressed(inputManager.GetKeyBinding(GameAction::MoveUp)))
        movement.y -= movementSpeed * dt;
    if (sf::Keyboard::isKeyPressed(inputManager.GetKeyBinding(GameAction::MoveDown)))
        movement.y += movementSpeed * dt;
    if (sf::Keyboard::isKeyPressed(inputManager.GetKeyBinding(GameAction::MoveLeft)))
        movement.x -= movementSpeed * dt;
    if (sf::Keyboard::isKeyPressed(inputManager.GetKeyBinding(GameAction::MoveRight)))
        movement.x += movementSpeed * dt;
    
    shape.move(movement);

    if (shootCooldown > 0.f) {
        shootCooldown -= dt;
    }
}

Player::BulletParams Player::Shoot(const sf::Vector2f& mouseWorldPos) {
    BulletParams params{};
    
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

sf::Vector2f Player::GetPosition() const {
    return shape.getPosition();
}

void Player::SetPosition(const sf::Vector2f& pos) {
    shape.setPosition(pos);
}

sf::RectangleShape& Player::GetShape() {
    return shape;
}
float Player::GetShootCooldown() const {
    return shootCooldown;
}
const sf::RectangleShape& Player::GetShape() const {
    return shape;
}

void Player::SetSpeed(float speed) {
    movementSpeed = speed;
}

float Player::GetSpeed() const {
    return movementSpeed;
}

void Player::TakeDamage(int amount) {
    health -= amount;
    if (health <= 0) {
        health = 0;
        isDead = true;
        // Save current position as respawn position
        respawnPosition = shape.getPosition();
    }
}

bool Player::IsDead() const {
    return isDead;
}

void Player::Respawn() {
    health = PLAYER_HEALTH;
    isDead = false;
    shape.setPosition(respawnPosition);
}

void Player::SetRespawnPosition(const sf::Vector2f& position) {
    respawnPosition = position;
}