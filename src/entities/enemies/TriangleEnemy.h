#ifndef TRIANGLE_ENEMY_H
#define TRIANGLE_ENEMY_H

#include "Enemy.h"
#include <SFML/Graphics.hpp>
#include "../../states/GameSettingsManager.h"

class TriangleEnemy : public Enemy {
public:
    TriangleEnemy(int id, const sf::Vector2f& position, float health = TRIANGLE_HEALTH, float speed = ENEMY_SPEED);
    TriangleEnemy(int id, const sf::Vector2f& position, float health = TRIANGLE_HEALTH, 
        float speed = ENEMY_SPEED, float size = TRIANGLE_SIZE);
    ~TriangleEnemy() override = default;
    
    void Render(sf::RenderWindow& window) override;
    EnemyType GetType() const override { return EnemyType::Triangle; }
    void ApplySettings(GameSettingsManager* settingsManager) override;
    void SetSize(float size) override {
        // First call the base class implementation to update radius
        Enemy::SetSize(size);
        
        // Then update the triangle-specific size property
        triangleSize = size;
        
        // Update the shape with the new size
        UpdateVisualRepresentation();
    }
protected:
    void UpdateVisualRepresentation() override;
    void UpdateMovement(float dt, PlayerManager& playerManager) override;
    
private:
    sf::ConvexShape shape;
    float rotationAngle;
    float rotationSpeed;
    float triangleSize;
};

#endif // TRIANGLE_ENEMY_H