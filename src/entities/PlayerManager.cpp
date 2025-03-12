#include "PlayerManager.h"
#include "../Game.h"
#include <iostream>

PlayerManager::PlayerManager(Game* game)
    : game(game)
{
}

PlayerManager::~PlayerManager() {
}

void PlayerManager::Update(float dt) {
    // For each player, update additional logic if needed (animations, interpolation, etc.)
    for (auto& pair : players) {
        sf::Vector2f pos = pair.second.player.GetPosition();
        // Ensure the player's name is rendered above the player.
        pair.second.nameText.setPosition(pos.x, pos.y - 20.f);
    }
}

void PlayerManager::AddOrUpdatePlayer(const std::string& id, const RemotePlayer& player) {
    players[id] = player;
}

void PlayerManager::RemovePlayer(const std::string& id) {
    players.erase(id);
}

std::unordered_map<std::string, RemotePlayer>& PlayerManager::GetPlayers() {
    return players;
}
