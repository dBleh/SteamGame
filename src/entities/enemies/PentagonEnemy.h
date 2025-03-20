#ifndef PENTAGON_ENEMY_H
#define PENTAGON_ENEMY_H

#include "Enemy.h"
#include "../../utils/config/EnemyConfig.h"
#include <SFML/Graphics.hpp>

// Forward declarations
class PlayerManager;

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
};

#endif // PENTAGON_ENEMY_H