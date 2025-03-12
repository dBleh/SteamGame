#include "PlayerManager.h"
#include "../Game.h"
#include <iostream>

PlayerManager::PlayerManager(Game* game, const std::string& localID)
    : game(game), localPlayerID(localID) {
}

PlayerManager::~PlayerManager() {
}

void PlayerManager::Update(float dt) {
    for (auto& pair : players) {
        RemotePlayer& rp = pair.second;
        sf::Vector2f pos;
        if (pair.first == localPlayerID) {
            // Local player uses input-based movement
            pos = rp.player.GetPosition();
        } else {
            // Interpolate remote players
            rp.interpTime += dt;
            float t = rp.interpTime / rp.interpDuration;
            if (t > 1.0f) t = 1.0f; // Clamp to avoid overshooting
            pos = rp.previousPosition + (rp.targetPosition - rp.previousPosition) * t;
            rp.player.SetPosition(pos); // Update Player position for rendering
        }
        // Update name text position
        rp.nameText.setPosition(pos.x, pos.y - 20.f);
    }
}

void PlayerManager::AddOrUpdatePlayer(const std::string& id, const RemotePlayer& player) {
    if (id.empty()) {
        std::cout << "[ERROR] Attempted to add player with empty ID!\n";
        return;
    }
    if (players.find(id) == players.end()) {
        // New player: initialize interpolation data
        players[id] = player;
        players[id].previousPosition = player.player.GetPosition();
        players[id].targetPosition = player.player.GetPosition();
        players[id].interpTime = 0.f;
        players[id].interpDuration = 0.5f; // Match host broadcast rate
        std::cout << "[DEBUG] Added player " << id << " at " << player.player.GetPosition().x << "," << player.player.GetPosition().y << "\n";
    } else if (id == localPlayerID) {
        // Local player: only update position from input
        players[id].player.SetPosition(player.player.GetPosition());
    } else {
        // Existing remote player: update target position for interpolation
        players[id].previousPosition = players[id].player.GetPosition();
        players[id].targetPosition = player.player.GetPosition();
        players[id].interpTime = 0.f; // Reset interpolation timer
        players[id].player = player.player; // Update shape, color, etc.
        players[id].nameText = player.nameText; // Update name
        std::cout << "[DEBUG] Updated player " << id << " to " << player.player.GetPosition().x << "," << player.player.GetPosition().y << "\n";
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
    rp.player = Player(position, color); // Use parameterized constructor
    rp.nameText.setFont(game->GetFont());
    rp.nameText.setString(name);
    rp.nameText.setCharacterSize(16);
    rp.nameText.setFillColor(sf::Color::Black);
    rp.previousPosition = position;
    rp.targetPosition = position;
    rp.interpTime = 0.f;
    rp.interpDuration = 0.5f; // Default, though local player won’t interpolate
    players[id] = rp;
    localPlayerID = id;
    std::cout << "[DEBUG] Added local player " << id << " at " << position.x << "," << position.y << "\n";
}

RemotePlayer& PlayerManager::GetLocalPlayer() {
    return players[localPlayerID];
}