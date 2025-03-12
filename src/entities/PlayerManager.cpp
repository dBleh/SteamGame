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
            rp.player.Update(dt);
            pos = rp.player.GetPosition();
        } else {
            float elapsed = std::chrono::duration<float>(now - rp.lastUpdateTime).count();
            float t = elapsed / rp.interpDuration;
            if (t > 1.0f) t = 1.0f;
            pos = rp.previousPosition + (rp.targetPosition - rp.previousPosition) * t;
            rp.player.SetPosition(pos);
        }
        // Fix: Set nameText to base name + status, don't append
        std::string baseName = rp.nameText.getString().toAnsiString();
        // Strip any existing status to avoid duplication
        size_t statusPos = baseName.find_last_of(" X✓");
        if (statusPos != std::string::npos && statusPos >= baseName.length() - 2) {
            baseName = baseName.substr(0, statusPos - 1); // Remove old " X" or " ✓"
        }
        std::string status = rp.isReady ? " ✓" : " X";
        rp.nameText.setString(baseName + status);
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
        players[id].interpDuration = 0.1f;
    } else if (id != localPlayerID) {
        players[id].previousPosition = players[id].player.GetPosition();
        players[id].targetPosition = player.player.GetPosition();
        players[id].lastUpdateTime = now;
        players[id].player = player.player;
        players[id].nameText = player.nameText; // Preserve Steam name
        players[id].isReady = player.isReady; // Sync ready status
    }
}

void PlayerManager::AddLocalPlayer(const std::string& id, const std::string& name, const sf::Vector2f& position, const sf::Color& color) {
    RemotePlayer rp;
    rp.player = Player(position, color);
    rp.nameText.setFont(game->GetFont());
    rp.nameText.setString(name); // Use Steam name
    rp.nameText.setCharacterSize(16);
    rp.nameText.setFillColor(sf::Color::Black);
    rp.previousPosition = position;
    rp.targetPosition = position;
    rp.lastUpdateTime = std::chrono::steady_clock::now();
    rp.interpDuration = 0.1f;
    players[id] = rp;
    localPlayerID = id;
}

void PlayerManager::SetReadyStatus(const std::string& id, bool ready) {
    if (players.find(id) != players.end()) {
        players[id].isReady = ready;
        // Name will update in next Update() call
    }
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