#ifndef FORCE_FIELD_H
#define FORCE_FIELD_H

#include <SFML/Graphics.hpp>
#include <iostream>
#include <functional>
#include <memory>
#include <vector>
#include <array>

// Forward declarations
class Player;
class PlayerManager;
class EnemyManager;
class Game;

// Enum for different field types
enum class FieldType {
    STANDARD,  // Default blue field
    SHOCK,     // Electric blue field (higher chance of chain effects)
    PLASMA,    // Orange-red field (higher damage)
    VORTEX     // Purple field (faster cooldown and wider area)
};

// Particle types for visual effects
enum class ParticleType {
    AMBIENT,   // Ambient particles that drift around
    IMPACT,    // Impact particles created during zaps
    ORBIT      // Particles that orbit the field
};

// Particle structure for visual effects
struct Particle {
    bool active;               // Whether particle is currently active
    ParticleType type;         // Type of particle
    sf::Vector2f position;     // Current position
    sf::Vector2f velocity;     // Current velocity (for non-orbiting particles)
    sf::Color color;           // Particle color
    float size;                // Current size
    float baseSize;            // Base/starting size
    float lifetime;            // Remaining lifetime in seconds
    float maxLifetime;         // Maximum lifetime in seconds
    
    // For orbiting particles
    float orbitAngle;          // Current angle in degrees
    float orbitSpeed;          // Orbit speed in degrees per second
    float orbitDistance;       // Distance from center point
};

class ForceField {
public:
    // Constants
    static constexpr float DEFAULT_RADIUS = 150.0f;
    static constexpr float DEFAULT_COOLDOWN = .3f; // seconds between zaps
    static constexpr float DEFAULT_DAMAGE = 25.0f;  // damage per zap
    static constexpr int MAX_PARTICLES = 1000;       // Maximum particles for effects
    static constexpr int NUM_FIELD_RINGS = 3;       // Number of decorative rings
    static constexpr int NUM_ENERGY_ORBS = 12;       // Number of orbiting energy orbs
    
    // Callback type for zap events
    using ZapCallback = std::function<void(int enemyId, float damage, bool killed)>;
    
    ForceField(Player* player, float radius = DEFAULT_RADIUS);
    ~ForceField() = default;
    
    // Core functionality
    void Update(float dt, PlayerManager& playerManager, EnemyManager& enemyManager);
    void Render(sf::RenderWindow& window);
    
    // Enhanced zap functionality
    void FindAndZapEnemy(PlayerManager& playerManager, EnemyManager& enemyManager);
    void CreateZapEffect(const sf::Vector2f& start, const sf::Vector2f& end);
    void performChainLightning(EnemyManager& enemyManager, const sf::Vector2f& playerCenter, 
                             int primaryTargetId, const sf::Vector2f& primaryTargetPos,
                             const std::vector<std::pair<int, sf::Vector2f>>& enemiesInRange);
    void createChainLightningEffect(const sf::Vector2f& start, const sf::Vector2f& end);
    void createLightningBranch(const sf::Vector2f& branchStart, const sf::Vector2f& mainDirection, 
                             float mainDistance, int currentSegment, int totalSegments,
                             const sf::Color& baseColor, const sf::Color& brightColor);
    
    // Advanced visual effects
    void renderZapEffects(sf::RenderWindow& window);
    void renderPowerIndicator(sf::RenderWindow& window, const sf::Vector2f& playerCenter);
    void updateFieldColor();
    bool HasZapCallback() const { return zapCallback != nullptr; }
    // Particle system
    void initializeParticles();
    void updateParticles(float dt, const sf::Vector2f& playerCenter);
    void renderParticles(sf::RenderWindow& window);
    void createAmbientParticle(const sf::Vector2f& center);
    void createImpactParticles(const sf::Vector2f& impactPos);
    
    // Getters/Setters for shop integration
    float GetRadius() const { return radius; }
    void SetRadius(float newRadius) { 
        radius = newRadius; 
        fieldShape.setRadius(newRadius);
        fieldShape.setOrigin(newRadius, newRadius);
        
        // Update ring sizes
        for (int i = 0; i < NUM_FIELD_RINGS; i++) {
            fieldRings[i].setRadius(radius * (0.4f + 0.2f * i));
            fieldRings[i].setOrigin(radius * (0.4f + 0.2f * i), radius * (0.4f + 0.2f * i));
        }
    }
    
    float GetCooldown() const { return zapCooldown; }
    void SetCooldown(float newCooldown) { zapCooldown = newCooldown; }
    
    float GetDamage() const { return zapDamage; }
    void SetDamage(float newDamage) { zapDamage = newDamage; }
    
    Player* GetPlayer() const { return player; }
    
    // Field type control
    FieldType GetFieldType() const { return fieldType; }
    void SetFieldType(FieldType type) { 
        fieldType = type;
        updateFieldColor();
    }
    
    // Power level control
    int GetPowerLevel() const { return powerLevel; }
    void SetPowerLevel(int level) { powerLevel = std::max(1, std::min(5, level)); }
    void IncreasePowerLevel() { powerLevel = std::min(powerLevel + 1, 5); }
    
    // Chain lightning control
    bool IsChainLightningEnabled() const { return chainLightningEnabled; }
    void SetChainLightningEnabled(bool enabled) { chainLightningEnabled = enabled; }
    int GetChainLightningTargets() const { return chainLightningTargets; }
    void SetChainLightningTargets(int targets) { chainLightningTargets = targets; }
    
    // Charge level
    float GetChargeLevel() const { return chargeLevel; }
    void SetChargeLevel(float charge) { chargeLevel = std::max(0.0f, std::min(1.0f, charge)); }
    void AddCharge(float amount) { chargeLevel = std::max(0.0f, std::min(1.0f, chargeLevel + amount)); }
    
    // Combo system
    int GetConsecutiveHits() const { return consecutiveHits; }
    void ResetCombo() { consecutiveHits = 0; comboTimer = 0.0f; }
    
    // Zap effect control
    void SetIsZapping(bool zapping) { isZapping = zapping; }
    void SetZapEffectTimer(float time) { zapEffectTimer = time; }
    
    // Event callback
    void SetZapCallback(ZapCallback callback) { zapCallback = callback; }
    
private:
    Player* player;                       // The player this force field belongs to
    sf::CircleShape fieldShape;           // Visual representation of the force field
    std::array<sf::CircleShape, NUM_FIELD_RINGS> fieldRings; // Decorative rings around the field
    std::array<sf::CircleShape, NUM_ENERGY_ORBS> energyOrbs; // Orbiting energy orbs
    std::array<float, NUM_ENERGY_ORBS> orbAngles;            // Current angle of each orb
    std::array<float, NUM_ENERGY_ORBS> orbSpeeds;            // Speed of each orb
    std::array<float, NUM_ENERGY_ORBS> orbDistances;         // Distance from center for each orb
    
    sf::VertexArray zapEffect;            // Visual representation of the zap lightning
    sf::VertexArray chainEffect;          // Visual representation of chain lightning
    
    // Particle system
    std::array<Particle, MAX_PARTICLES> particles;
    
    float radius;                         // Radius of the force field
    float zapTimer;                       // Current cooldown timer
    float zapCooldown;                    // Time between zaps
    float zapDamage;                      // Damage per zap
    
    int targetEnemyId;                    // ID of the currently targeted enemy
    sf::Vector2f zapEndPosition;          // End position of the zap effect
    bool isZapping;                       // Whether a zap is currently active
    float zapEffectDuration;              // How long the zap effect lasts
    float zapEffectTimer;                 // Timer for the zap effect
    
    float fieldRotation;                  // Current rotation angle for field effects
    float fieldPulsePhase;                // Phase for pulsing effects
    float fieldIntensity;                 // Current intensity multiplier (1.0 = normal)
    sf::Color fieldColor;                 // Base color of the field
    
    float chargeLevel;                    // Current charge level (0.0 to 1.0)
    int powerLevel;                       // Current power level (1-5)
    int consecutiveHits;                  // Number of consecutive successful zaps
    float comboTimer;                     // Timer for combo system
    
    bool chainLightningEnabled;           // Whether chain lightning is enabled
    int chainLightningTargets;            // Maximum number of chain targets
    
    FieldType fieldType;                  // Type of force field
    
    ZapCallback zapCallback;              // Callback for zap events
};

#endif // FORCE_FIELD_H