#ifndef SQUARE_ENEMY_H
#define SQUARE_ENEMY_H

#include "Enemy.h"
#include "../../utils/config/EnemyConfig.h"
#include <SFML/Graphics.hpp>

// Forward declarations
class PlayerManager;

// Define movement phases for the revamped square enemy behavior
enum class MovementPhase {
    Seeking,    // Approaching the player
    FlyBy,      // Fast direct movement past the player
    Orbiting,   // Circling around the player
    Retreating  // Moving away from the player
};

class SquareEnemy : public Enemy {
public:
    // Constructor with all parameters
    SquareEnemy(int id, const sf::Vector2f& position, float health, float speed);
    
    // Constructor with default values for health and speed
    SquareEnemy(int id, const sf::Vector2f& position);
    
    ~SquareEnemy() override = default;
    
    void FindTarget(PlayerManager& playerManager) override;
    void Render(sf::RenderWindow& window) override;
    EnemyType GetType() const override { return EnemyType::Square; }
    
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
    
    // Store reference to the target player's shape
    const sf::RectangleShape* lastTargetShape;
    
    // Helper function to find the closest point on a rectangle
    sf::Vector2f FindClosestPointOnRect(const sf::RectangleShape& rect);
    
    bool CheckPlayerIntersectsAnyLine(PlayerManager& playerManager);
    bool CheckLineIntersectsPlayer(const sf::Vector2f& lineStart, const sf::Vector2f& lineEnd, const sf::RectangleShape& playerShape);
    
    // New movement behavior properties
    float flyByTimer;
    bool flyByActive;
    float flyByDuration;
    float flyBySpeedMultiplier;
    sf::Vector2f flyByDirection;
    
    float orbitDistance;
    float orbitSpeedMultiplier;
    
    MovementPhase movementPhase;
    MovementPhase lastState;
    float phaseTimer;
    float targetPlayerDistance;
    float directionChangeTimer;
    
    // New movement behavior methods
    void UpdateMovementPhase(float dt);
    void HandleSeekingMovement(float dt);
    void HandleFlyByMovement(float dt);
    void HandleOrbitingMovement(float dt);
    void HandleRetreatMovement(float dt);
};

#endif // SQUARE_ENEMY_H