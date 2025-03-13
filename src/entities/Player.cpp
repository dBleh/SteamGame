#include "Player.h"
#include <iostream>
#include <cmath>

Player::Player()
    : movementSpeed(200.f), shootCooldown(0.f) {
    shape.setSize(sf::Vector2f(50.f, 50.f));
    shape.setFillColor(sf::Color::Blue);
    shape.setPosition(50.f, 50.f);
}

Player::Player(const sf::Vector2f& startPosition, const sf::Color& color)
    : movementSpeed(200.f), shootCooldown(0.f) {
    shape.setSize(sf::Vector2f(50.f, 50.f));
    shape.setFillColor(color);
    shape.setPosition(startPosition);
}

void Player::Update(float dt) {
    sf::Vector2f movement(0.f, 0.f);
    
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::W))
        movement.y -= movementSpeed * dt;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::S))
        movement.y += movementSpeed * dt;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::A))
        movement.x -= movementSpeed * dt;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::D))
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

        // Calculate direction from player to mouse
        sf::Vector2f playerPos = GetPosition();
        sf::Vector2f direction = mouseWorldPos - playerPos;
        float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
        if (length > 0.f) {
            direction /= length;  // Normalize direction
        } else {
            direction = sf::Vector2f(1.f, 0.f);  // Default right if mouse is on player
        }

        // Set bullet start position (offset from player)
        params.position = playerPos + direction * 50.f;  // 50 units offset
        params.direction = direction;

        std::cout << "[PLAYER] Bullet fired toward (" << direction.x << ", " << direction.y << ")\n";
    } else {
        // If on cooldown, return default/invalid params (won’t be used)
        params.position = GetPosition();
        params.direction = sf::Vector2f(0.f, 0.f);
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

const sf::RectangleShape& Player::GetShape() const {
    return shape;
}

void Player::SetSpeed(float speed) {
    movementSpeed = speed;
}

float Player::GetSpeed() const {
    return movementSpeed;
}