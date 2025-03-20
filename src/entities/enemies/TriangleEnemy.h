#ifndef TRIANGLE_ENEMY_H
#define TRIANGLE_ENEMY_H

#include "Enemy.h"
#include "../../utils/config/EnemyConfig.h"
#include <SFML/Graphics.hpp>

// Forward declarations
class PlayerManager;

class TriangleEnemy : public Enemy {
public:
    TriangleEnemy(int id, const sf::Vector2f& position, float health = TRIANGLE_HEALTH, float speed = ENEMY_SPEED);
    ~TriangleEnemy() override = default;
    void FindTarget(PlayerManager& playerManager) override;
    void Render(sf::RenderWindow& window) override;
    EnemyType GetType() const override { return EnemyType::Triangle; }
    
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
    float axisChangeTimer;
    float axisChangeInterval; 
    sf::Vector2f lastIntersectionPoint;
    float lineLength;
    bool playerIntersectsLine;
    bool CheckPlayerIntersectsAnyLine(PlayerManager& playerManager);
    bool CheckLineIntersectsPlayer(const sf::Vector2f& lineStart, const sf::Vector2f& lineEnd, const sf::RectangleShape& playerShape);
    
    // New bounce-related properties
    float bounceTimer;
    float bounceDirection;
    float bounceAmplitude;
    float bounceFrequency;
};

#endif // TRIANGLE_ENEMY_H