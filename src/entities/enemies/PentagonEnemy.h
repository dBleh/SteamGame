#ifndef PENTAGON_ENEMY_H
#define PENTAGON_ENEMY_H

#include "Enemy.h"
#include "../../utils/config/EnemyConfig.h"
#include <SFML/Graphics.hpp>
#include <deque>

// Forward declarations
class PlayerManager;

// Different behavior patterns for the pentagon
enum class PentagonBehavior {
    Stalking,       // Follows player cautiously, keeping distance
    Charging,       // Builds up energy and charges toward player
    Pulsating,      // Expands influence in waves to trap player
    Encircling,     // Creates a complex pentagon formation around player
    Teleporting     // Disappears and reappears in strategic positions
};

class PentagonEnemy : public Enemy {
public:
    // Constructor with all parameters
    PentagonEnemy(int id, const sf::Vector2f& position, float health, float speed);
    
    // Constructor with default values for health and speed
    PentagonEnemy(int id, const sf::Vector2f& position);
    
    ~PentagonEnemy() override = default;
    
    void FindTarget(PlayerManager& playerManager) override;
    void Render(sf::RenderWindow& window) override;
    EnemyType GetType() const override { return EnemyType::Pentagon; }
    
    std::vector<sf::Vector2f> GetAxes() const;
    
protected:
    void UpdateVisualRepresentation() override;
    void UpdateMovement(float dt, PlayerManager& playerManager) override;
    
private:
    void InitializeAxes();
    sf::ConvexShape shape;
    std::vector<sf::Vector2f> axes;
    void UpdateAxes();
    float rotationAngle;
    float rotationSpeed;
    int currentAxisIndex;
    float lineLength;
    bool playerIntersectsLine;
    sf::Vector2f lastIntersectionPoint;
    
    bool CheckPlayerIntersectsAnyLine(PlayerManager& playerManager);
    bool CheckLineIntersectsPlayer(const sf::Vector2f& lineStart, const sf::Vector2f& lineEnd, const sf::RectangleShape& playerShape);

    const sf::RectangleShape* lastTargetShape;
    
    // Helper function to find the closest point on a rectangle
    sf::Vector2f FindClosestPointOnRect(const sf::RectangleShape& rect);
    
    // Advanced pentagon movement system
    PentagonBehavior currentBehavior;
    PentagonBehavior lastBehavior;
    float behaviorTimer;
    float stateTransitionTimer;
    float targetPlayerDistance;
    float chargeEnergy;
    float maxChargeEnergy;
    bool chargingUp;
    bool isCharging;
    
    // Afterimage and teleportation system
    struct AfterImage {
        sf::Vector2f position;
        float lifetime;
        float alpha;
    };
    std::deque<AfterImage> afterImages;
    bool isTeleporting;
    sf::Vector2f teleportDestination;
    float teleportProgress;
    float teleportDuration;
    
    // Pulsating system
    float pulsePhase;
    float pulseFrequency;
    float pulseAmplitude;
    int pulseCount;
    int maxPulseCount;
    
    // Encircling system
    std::vector<sf::Vector2f> formationPositions;
    int currentFormationIndex;
    float formationRadius;
    float formationAngle;
    
    // Pattern creation
    void GenerateEncirclingFormation();
    void ClearAfterImages();
    void AddAfterImage(float lifetime = 1.0f);
    
    // Behavior methods
    void UpdateBehavior(float dt);
    void HandleStalkingBehavior(float dt);
    void HandleChargingBehavior(float dt);
    void HandlePulsatingBehavior(float dt);
    void HandleEncirclingBehavior(float dt);
    void HandleTeleportingBehavior(float dt);
    
    // Helper methods
    sf::Vector2f GetBestAxisForDirection(const sf::Vector2f& direction, bool allowReverse = true);
    sf::Vector2f GetVectorAlongBestAxis(const sf::Vector2f& direction, float speedMultiplier = 1.0f);
};

#endif // PENTAGON_ENEMY_H