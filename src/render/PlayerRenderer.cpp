#include "PlayerRenderer.h"
#include "../entities/PlayerManager.h"

PlayerRenderer::PlayerRenderer(PlayerManager* manager)
    : playerManager(manager)
{
}

PlayerRenderer::~PlayerRenderer() {
}

void PlayerRenderer::Render(sf::RenderWindow& window) {
    auto& players = playerManager->GetPlayers();

    for (auto& pair : players) {
        
        window.draw(pair.second.player.GetShape());
        window.draw(pair.second.nameText);
    }
}