#include "PlayerManager.h"
#include "../Game.h"
#include <iostream>

PlayerManager::PlayerManager(Game* game, const std::string& localID)
    : game(game), localPlayerID(localID) {
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
    if (id.empty()) {
        std::cout << "[ERROR] Attempted to add player with empty ID!\n";
        return; // Skip adding
    }
    if (id == localPlayerID) {
        players[id].player.SetPosition(player.player.GetPosition());
    } else {
        players[id] = player;
        
    }
}
void PlayerManager::RemovePlayer(const std::string& id) {
    players.erase(id);
}

std::unordered_map<std::string, RemotePlayer>& PlayerManager::GetPlayers() {
    return players;
}

void PlayerManager::AddLocalPlayer(const std::string& id, const std::string& name, const sf::Vector2f& position, const sf::Color& color) {
    RemotePlayer rp;
    rp.player.SetPosition(position);
    rp.player.GetShape().setFillColor(color);
    rp.nameText.setFont(game->GetFont());
    rp.nameText.setString(name);
    rp.nameText.setCharacterSize(16);
    rp.nameText.setFillColor(sf::Color::Black);
    players[id] = rp;
    localPlayerID = id;
}

RemotePlayer& PlayerManager::GetLocalPlayer() {
    return players[localPlayerID];
}