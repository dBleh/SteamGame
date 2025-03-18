#ifndef FORCE_FIELD_H
#define FORCE_FIELD_H

#include <SFML/Graphics.hpp>
#include <iostream>
#include <functional>
#include <memory>

// Forward declarations
class Player;
class PlayerManager;
class EnemyManager;
class Game;

class ForceField {
public:
    // Constants
    static constexpr float DEFAULT_RADIUS = 150.0f;
    static constexpr float DEFAULT_COOLDOWN = 1.5f; // seconds between zaps
    static constexpr float DEFAULT_DAMAGE = 25.0f;  // damage per zap
    
    // Callback type for zap events
    using ZapCallback = std::function<void(int enemyId, float damage, bool killed)>;
    
    ForceField(Player* player, float radius = DEFAULT_RADIUS);
    ~ForceField() = default;
    
    // Core functionality
    void Update(float dt, PlayerManager& playerManager, EnemyManager& enemyManager);
    void Render(sf::RenderWindow& window);
    
    // Getters/Setters
    float GetRadius() const { return radius; }
    void SetRadius(float newRadius) { radius = newRadius; fieldShape.setRadius(newRadius); }
    float GetCooldown() const { return zapCooldown; }
    void SetCooldown(float newCooldown) { zapCooldown = newCooldown; }
    float GetDamage() const { return zapDamage; }
    void SetDamage(float newDamage) { zapDamage = newDamage; }
    Player* GetPlayer() const { return player; }
    
    // Zap effect control
    void CreateZapEffect(const sf::Vector2f& start, const sf::Vector2f& end);
    void SetIsZapping(bool zapping) { isZapping = zapping; }
    void SetZapEffectTimer(float time) { zapEffectTimer = time; }
    
    // Event callback
    void SetZapCallback(ZapCallback callback) { zapCallback = callback; }
    
private:
    Player* player;             // The player this force field belongs to
    sf::CircleShape fieldShape; // Visual representation of the force field
    sf::VertexArray zapEffect;  // Visual representation of the zap lightning
    
    float radius;               // Radius of the force field
    float zapTimer;             // Current cooldown timer
    float zapCooldown;          // Time between zaps
    float zapDamage;            // Damage per zap
    
    int targetEnemyId;          // ID of the currently targeted enemy
    sf::Vector2f zapEndPosition; // End position of the zap effect
    bool isZapping;             // Whether a zap is currently active
    float zapEffectDuration;    // How long the zap effect lasts
    float zapEffectTimer;       // Timer for the zap effect
    
    ZapCallback zapCallback;    // Callback for zap events
    
    // Helper methods
    void FindAndZapEnemy(PlayerManager& playerManager, EnemyManager& enemyManager);
    void UpdateZapEffect(float dt);
};

#endif // FORCE_FIELD_H