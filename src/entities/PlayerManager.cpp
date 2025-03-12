#include "PlayerManager.h"
#include "../Game.h"
#include <iostream>

PlayerManager::PlayerManager(Game* game, const std::string& localID)
    : game(game), localPlayerID(localID), lastFrameTime(std::chrono::steady_clock::now()) {
}

PlayerManager::~PlayerManager() {
}

void PlayerManager::Update() {
    auto now = std::chrono::steady_clock::now();
    float dt = std::chrono::duration<float>(now - lastFrameTime).count();
    lastFrameTime = now;

    for (auto& pair : players) {
        RemotePlayer& rp = pair.second;
        sf::Vector2f pos;
        if (pair.first == localPlayerID) {
            // Local player updates immediately
            rp.player.Update(dt);
            pos = rp.player.GetPosition();
        } else {
            // Interpolate remote players based on time
            float elapsed = std::chrono::duration<float>(now - rp.lastUpdateTime).count();
            float t = elapsed / rp.interpDuration;
            if (t > 1.0f) t = 1.0f; // Clamp to target position
            pos = rp.previousPosition + (rp.targetPosition - rp.previousPosition) * t;
            rp.player.SetPosition(pos);
            //std::cout << "[DEBUG] Interpolating " << pair.first << " to " << pos.x << "," << pos.y << " (t=" << t << ")\n";
        }
        rp.nameText.setPosition(pos.x, pos.y - 20.f);
    }
}

void PlayerManager::AddOrUpdatePlayer(const std::string& id, const RemotePlayer& player) {
    if (id.empty()) {
        std::cout << "[ERROR] Attempted to add player with empty ID!\n";
        return;
    }
    auto now = std::chrono::steady_clock::now();
    if (players.find(id) == players.end()) {
        players[id] = player;
        players[id].previousPosition = player.player.GetPosition();
        players[id].targetPosition = player.player.GetPosition();
        players[id].lastUpdateTime = now;
        players[id].interpDuration = 0.1f; // Match broadcast rate
    } else if (id != localPlayerID) {
        // Remote player: update interpolation targets
        players[id].previousPosition = players[id].player.GetPosition();
        players[id].targetPosition = player.player.GetPosition();
        players[id].lastUpdateTime = now;
        players[id].player = player.player; // Update shape, color, etc.
        players[id].nameText = player.nameText;
    }
    // Local player position is updated locally, not overridden here
}

void PlayerManager::AddLocalPlayer(const std::string& id, const std::string& name, const sf::Vector2f& position, const sf::Color& color) {
    RemotePlayer rp;
    rp.player = Player(position, color);
    rp.nameText.setFont(game->GetFont());
    rp.nameText.setString(name);
    rp.nameText.setCharacterSize(16);
    rp.nameText.setFillColor(sf::Color::Black);
    rp.previousPosition = position;
    rp.targetPosition = position;
    rp.lastUpdateTime = std::chrono::steady_clock::now();
    rp.interpDuration = 0.1f;
    players[id] = rp;
    localPlayerID = id;
}

RemotePlayer& PlayerManager::GetLocalPlayer() {
    return players[localPlayerID];
}

void PlayerManager::RemovePlayer(const std::string& id) {
    players.erase(id);
}

std::unordered_map<std::string, RemotePlayer>& PlayerManager::GetPlayers() {
    return players;
}