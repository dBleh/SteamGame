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
        // Rebuild nameText from baseName + status
        std::string status = rp.isReady ? " ✓" : " X";
        rp.nameText.setString(rp.baseName + status);
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
            // New player
            players[id] = player;
            players[id].playerID = id;
            players[id].previousPosition = player.player.GetPosition();
            players[id].targetPosition = player.player.GetPosition();
            players[id].lastUpdateTime = now;
            players[id].baseName = player.baseName.empty() ? player.nameText.getString().toAnsiString() : player.baseName;
        } else if (id != localPlayerID) {
            // Update existing player
            players[id].previousPosition = players[id].player.GetPosition();
            players[id].targetPosition = player.player.GetPosition();
            players[id].lastUpdateTime = now;
            players[id].player = player.player;
            players[id].cubeColor = player.cubeColor;
            players[id].isReady = player.isReady;
            players[id].isHost = player.isHost;
            players[id].nameText = player.nameText;
        }
    }

void PlayerManager::AddLocalPlayer(const std::string& id, const std::string& name, const sf::Vector2f& position, const sf::Color& color) {
    RemotePlayer rp;
    rp.player = Player(position, color);
    rp.nameText.setFont(game->GetFont());
    rp.nameText.setString(name);
    rp.baseName = name; // Set baseName here
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
        std::cout << "[PM] Set ready status for " << id << " to " << (ready ? "true" : "false") << "\n";
        std::string status = ready ? " ✓" : " X";
        players[id].nameText.setString(players[id].baseName + status);
    } else {
        std::cout << "[PM] Player " << id << " not found for ready status update\n";
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