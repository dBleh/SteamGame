#include "Bullet.h"

Bullet::Bullet(const sf::Vector2f& position, const sf::Vector2f& direction, float speed, const std::string& shooterID)
    : lifetime(5.f), shooterID(shooterID) {
    shape.setSize(sf::Vector2f(10.f, 10.f));  // Small rectangle for bullet
    shape.setFillColor(sf::Color::Black);   // Distinct color
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

bool Bullet::CheckCollision(const sf::RectangleShape& playerShape, const std::string& playerID) const {
    // Don't collide with the shooter
    if (shooterID == playerID) return false;
    
    bool collision = shape.getGlobalBounds().intersects(playerShape.getGlobalBounds());
    if (collision) {
        std::cout << "[BULLET] Collision detected between bullet from " << shooterID 
                 << " and player " << playerID << std::endl;
    }
    return collision;
}
bool Bullet::BelongsToPlayer(const std::string& playerID) const {
    return shooterID == playerID;
}