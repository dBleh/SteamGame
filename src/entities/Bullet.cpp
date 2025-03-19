#include "Bullet.h"

Bullet::Bullet(const sf::Vector2f& position, const sf::Vector2f& direction, float speed, const std::string& shooterID)
    : lifetime(5.f), 
      shooterID(shooterID),
      radius(BULLET_RADIUS),
      damage(BULLET_DAMAGE) {
    // Create smaller bullet for better precision
    shape.setSize(sf::Vector2f(radius * 2, radius * 2));  
    shape.setFillColor(sf::Color::Black);
    
    // Center the bullet shape (important for accurate collision)
    shape.setOrigin(radius, radius);
    
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
        return false;
    }
    
    bool collision = shape.getGlobalBounds().intersects(playerShape.getGlobalBounds());
    return collision;
}

bool Bullet::BelongsToPlayer(const std::string& playerID) const {
    return shooterID == playerID;
}

void Bullet::ApplySettings(GameSettingsManager* settingsManager) {
    if (!settingsManager) return;
    
    // Apply bullet settings
    GameSetting* bulletDamageSetting = settingsManager->GetSetting("bullet_damage");
    if (bulletDamageSetting) {
        damage = bulletDamageSetting->GetFloatValue();
    }
    
    GameSetting* bulletRadiusSetting = settingsManager->GetSetting("bullet_radius");
    if (bulletRadiusSetting) {
        radius = bulletRadiusSetting->GetFloatValue();
        
        // Update the shape size to match the new radius
        shape.setSize(sf::Vector2f(radius * 2, radius * 2));
        shape.setOrigin(radius, radius);
    }
    
    // Note: We don't update the speed of existing bullets as that would be
    // confusing for players. Only new bullets get the updated speed.
}