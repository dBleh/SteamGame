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
    std::cout << "[DEBUG] Rendering " << players.size() << " players:\n";
    for (auto& pair : players) {
        std::cout << "[DEBUG] Player ID: " << pair.first 
                  << ", Position: (" << pair.second.player.GetPosition().x << ", " << pair.second.player.GetPosition().y 
                  << "), Color: (" << (int)pair.second.player.GetShape().getFillColor().r << ", " 
                  << (int)pair.second.player.GetShape().getFillColor().g << ", " 
                  << (int)pair.second.player.GetShape().getFillColor().b << ")\n";
        window.draw(pair.second.player.GetShape());
        window.draw(pair.second.nameText);
    }
}