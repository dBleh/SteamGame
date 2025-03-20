#include "Bullet.h"

Bullet::Bullet(const sf::Vector2f& position, const sf::Vector2f& direction, float speed, const std::string& shooterID)
    : lifetime(BULLET_LIFETIME), shooterID(shooterID) {
    // Create smaller bullet for better precision
    shape.setSize(sf::Vector2f(BULLET_RADIUS, BULLET_RADIUS));  
    shape.setFillColor(BULLET_COLOR);
    
    // Center the bullet shape (important for accurate collision)
    shape.setOrigin(BULLET_RADIUS / 2.0f, BULLET_RADIUS / 2.0f);
    
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
    // Normalize both IDs before comparison for consistency
    std::string normalizedShooterID = shooterID;
    std::string normalizedPlayerID = playerID;
    
    try {
        if (!shooterID.empty()) {
            uint64_t shooterNum = std::stoull(shooterID);
            normalizedShooterID = std::to_string(shooterNum);
        }
        
        if (!playerID.empty()) {
            uint64_t playerNum = std::stoull(playerID);
            normalizedPlayerID = std::to_string(playerNum);
        }
    } catch (const std::exception& e) {
        std::cout << "[BULLET] Error normalizing IDs for collision: " << e.what() << "\n";
    }
    
    // Don't collide with the shooter (using normalized IDs)
    if (normalizedShooterID == normalizedPlayerID) {
        std::cout << "[BULLET] Self collision prevented\n";
        return false;
    }
    
    bool collision = shape.getGlobalBounds().intersects(playerShape.getGlobalBounds());
    return collision;
}
bool Bullet::BelongsToPlayer(const std::string& playerID) const {
    return shooterID == playerID;
}