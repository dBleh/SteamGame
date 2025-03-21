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

protected:
    void UpdateVisualRepresentation() override;
    void UpdateMovement(float dt, PlayerManager& playerManager) override;

private:
    sf::ConvexShape shape;
    float rotationAngle;
    float rotationSpeed;
    float bounceTimer;
    float bounceAmplitude;
    float bounceFrequency;
};

#endif // TRIANGLE_ENEMY_H