#ifndef PLAYER_MANAGER_H
#define PLAYER_MANAGER_H

#include <unordered_map>
#include <string>
#include <SFML/Graphics.hpp>
#include "../utils/SteamHelpers.h"
#include <chrono> // For time-based updates

class Game;


class PlayerManager {
public:
    PlayerManager(Game* game, const std::string& localID);
    ~PlayerManager();
    void AddLocalPlayer(const std::string& id, const std::string& name, const sf::Vector2f& position, const sf::Color& color);
    RemotePlayer& GetLocalPlayer();
    void Update(); // Remove dt, use time-based logic
    void AddOrUpdatePlayer(const std::string& id, const RemotePlayer& player);
    void RemovePlayer(const std::string& id);
    std::unordered_map<std::string, RemotePlayer>& GetPlayers();

private:
    Game* game;
    std::unordered_map<std::string, RemotePlayer> players;
    std::string localPlayerID;
    std::chrono::steady_clock::time_point lastFrameTime; // Track frame timing
};

#endif // PLAYER_MANAGER_H