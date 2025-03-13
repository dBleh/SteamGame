#ifndef PLAYER_RENDERER_H
#define PLAYER_RENDERER_H

#include <SFML/Graphics.hpp>
#include <iostream>
#include "../entities/PlayerManager.h"  // Update to include Bullet;
class PlayerManager; // Forward declaration

class PlayerRenderer {
public:
    explicit PlayerRenderer(PlayerManager* manager);
    ~PlayerRenderer();

    // Draw all players onto the provided window.
    void Render(sf::RenderWindow& window);

private:
    PlayerManager* playerManager;
};

#endif // PLAYER_RENDERER_H
