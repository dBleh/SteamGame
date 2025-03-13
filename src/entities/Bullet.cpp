#include "Bullet.h"

Bullet::Bullet(const sf::Vector2f& position, const sf::Vector2f& direction, float speed, const std::string& shooterID)
    : lifetime(5.f), shooterID(shooterID) {
    // Create smaller bullet for better precision
    shape.setSize(sf::Vector2f(8.f, 8.f));  
    shape.setFillColor(sf::Color::Black);
    
    // Center the bullet shape (important for accurate collision)
    shape.setOrigin(4.f, 4.f);
    
    // Set the bullet position 
    shape.setPosition(position);
    
    // Set the bullet velocity
    velocity = direction * speed;
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