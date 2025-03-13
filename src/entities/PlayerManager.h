#ifndef PLAYER_MANAGER_H
#define PLAYER_MANAGER_H

#include <unordered_map>
#include <string>
#include <vector>
#include <SFML/Graphics.hpp>
#include "../utils/SteamHelpers.h"
#include <chrono>
#include "Bullet.h"

class Game;

class PlayerManager {
public:
    PlayerManager(Game* game, const std::string& localID);
    ~PlayerManager();
    void AddLocalPlayer(const std::string& id, const std::string& name, const sf::Vector2f& position, const sf::Color& color);
    RemotePlayer& GetLocalPlayer();
    void Update();
    void AddOrUpdatePlayer(const std::string& id, const RemotePlayer& player);
    void RemovePlayer(const std::string& id);
    std::unordered_map<std::string, RemotePlayer>& GetPlayers();
    void SetReadyStatus(const std::string& id, bool ready);
    void AddBullet(const std::string& shooterID, const sf::Vector2f& position, const sf::Vector2f& direction, float velocity);
    std::vector<Bullet>& GetBullets() { return bullets; }

private:
    Game* game;
    std::unordered_map<std::string, RemotePlayer> players;
    std::string localPlayerID;
    std::chrono::steady_clock::time_point lastFrameTime;
    std::vector<Bullet> bullets;  // List of active bullets
};

#endif // PLAYER_MANAGER_H