#include "Bullet.h"

Bullet::Bullet(const sf::Vector2f& position, const sf::Vector2f& direction, float speed, const std::string& shooterID)
    : lifetime(5.f), shooterID(shooterID) {
    shape.setSize(sf::Vector2f(10.f, 4.f));  // Small rectangle for bullet
    shape.setFillColor(sf::Color::Yellow);   // Distinct color
    shape.setPosition(position);
    velocity = direction * speed;            // Normalized direction * speed
}

void Bullet::Update(float dt) {
    shape.move(velocity * dt);
    lifetime -= dt;
}

sf::RectangleShape& Bullet::GetShape() {
    return shape;
}

const sf::RectangleShape& Bullet::GetShape() const {
    return shape;
}