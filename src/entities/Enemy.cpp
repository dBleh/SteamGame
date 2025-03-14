#include "Enemy.h"
#include <sstream>
#include <iostream>

Enemy::Enemy(int id, const sf::Vector2f& position, float speed, int health)
    : id(id), movementSpeed(speed), health(health), isDead(false) {
    // Create enemy shape (red square)
    shape.setSize(sf::Vector2f(40.f, 40.f));
    shape.setFillColor(sf::Color::Red);
    shape.setOutlineThickness(2.f);
    shape.setOutlineColor(sf::Color::Black);
    
    // Center the enemy shape for accurate collision
    shape.setOrigin(20.f, 20.f);
    
    // Set the initial position
    shape.setPosition(position);
}

void Enemy::Update(float dt, const sf::Vector2f& targetPosition) {
    if (isDead) return;
    
    // Calculate direction vector to target
    sf::Vector2f currentPos = shape.getPosition();
    sf::Vector2f direction = targetPosition - currentPos;
    
    // Normalize direction vector
    float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
    if (length > 0) {
        direction /= length;
    }
    
    // Move towards target
    shape.move(direction * movementSpeed * dt);
}

bool Enemy::TakeDamage(int amount) {
    health -= amount;
    std::cout << "Enemy took damage" << std::endl;
    // Update visuals
    UpdateVisuals();
    
    if (health <= 0) {
        isDead = true;
        return true; // Enemy died from this damage
    }
    return false; // Enemy still alive
}
void Enemy::ProcessHit(int damage, const std::string& bulletID) {
    // Current timestamp (could use game time instead)
    float currentTime = static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count()) / 1000.0f;
    
    // Check if we've recently processed a hit from this bullet
    auto it = recentHits.find(bulletID);
    if (it != recentHits.end()) {
        // If this bullet hit recently (within 0.5 seconds), ignore it
        if (currentTime - it->second < 0.5f) {
            std::cout << "[ENEMY] Ignoring duplicate hit from bullet " << bulletID << std::endl;
            return;
        }
    }
    
    // Record this hit with current timestamp
    recentHits[bulletID] = currentTime;
    
    // Clean up old entries (optional, for memory management)
    for (auto it = recentHits.begin(); it != recentHits.end();) {
        if (currentTime - it->second > 1.0f) {
            it = recentHits.erase(it);
        } else {
            ++it;
        }
    }
    
    // Now actually apply the damage
    TakeDamage(damage);
    std::cout << "[ENEMY] Processing hit from bullet " << bulletID << std::endl;
}
bool Enemy::CheckCollision(const sf::RectangleShape& playerShape) {
    if (isDead) return false;
    
    // Simple AABB collision
    return shape.getGlobalBounds().intersects(playerShape.getGlobalBounds());
}

sf::RectangleShape& Enemy::GetShape() {
    return shape;
}

const sf::RectangleShape& Enemy::GetShape() const {
    return shape;
}

sf::Vector2f Enemy::GetPosition() const {
    return shape.getPosition();
}

bool Enemy::IsAlive() const {
    return !isDead;
}

int Enemy::GetID() const {
    return id;
}

std::string Enemy::Serialize() const {
    std::ostringstream oss;
    sf::Vector2f pos = shape.getPosition();
    oss << id << "," << pos.x << "," << pos.y << "," << health << "," << (isDead ? "1" : "0");
    return oss.str();
}

Enemy Enemy::Deserialize(const std::string& data) {
    std::istringstream iss(data);
    int id;
    float x, y;
    int health;
    std::string deadStr;
    char comma;
    
    iss >> id >> comma >> x >> comma >> y >> comma >> health >> comma >> deadStr;
    
    Enemy enemy(id, sf::Vector2f(x, y), 60.f, health);
    if (deadStr == "1") {
        enemy.isDead = true;
    }
    
    return enemy;
}



void Enemy::UpdateVisuals() {
    // Update color based on health percentage
    float healthPercent = static_cast<float>(health) / 40.0f; // Based on initial health
    sf::Color newColor(
        255,  // Full red
        static_cast<sf::Uint8>(255 * healthPercent),  // Less green as damage increases
        static_cast<sf::Uint8>(255 * healthPercent)   // Less blue as damage increases
    );
    shape.setFillColor(newColor);
}