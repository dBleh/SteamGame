#ifndef PLAYER_H
#define PLAYER_H

#include <SFML/Graphics.hpp>
#include <steam/steam_api.h>  // Optional: if you want to associate a SteamID with the player
#include <iostream>
class Player {
public:
    // Default constructor with preset starting position, color, and movement speed.
    Player();
    // Constructor allowing to set starting position and color.
    Player(const sf::Vector2f& startPosition, const sf::Color& color = sf::Color::Blue);
    
    // Update the player's state (handle movement).
    void Update(float dt);
    struct BulletParams {
        sf::Vector2f position;
        sf::Vector2f direction;
    };
    BulletParams Shoot(const sf::Vector2f& mouseWorldPos);
    // Get the player's current position.
    sf::Vector2f GetPosition() const;
    
    // Set the player's position.
    void SetPosition(const sf::Vector2f& pos);
    
    sf::RectangleShape& GetShape();
    // Add a const overload for read-only access (used when drawing, etc.).
    const sf::RectangleShape& GetShape() const;

    void Shoot();
    // Set the movement speed.
    void SetSpeed(float speed);
    // Get the movement speed.
    float GetSpeed() const;
    float shootCooldown;    // Time remaining until next shot allowed
    static constexpr float SHOOT_COOLDOWN_DURATION = 0.5f;

    void TakeDamage(int amount);
    bool IsDead() const;
    void Respawn();
    void SetRespawnPosition(const sf::Vector2f& position);

private:
    sf::RectangleShape shape;  // Represents the player as a cube.
    float movementSpeed;       // Movement speed in pixels per second.
    int health;
    bool isDead;
    sf::Vector2f respawnPosition;
    
};

#endif // PLAYER_H
