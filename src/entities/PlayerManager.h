#ifndef PLAYER_MANAGER_H
#define PLAYER_MANAGER_H

#include <unordered_map>
#include <string>
#include <SFML/Graphics.hpp>
#include "../utils/SteamHelpers.h" // Assuming RemotePlayer is defined here

class Game; // Forward declaration

class PlayerManager {
    public:
        PlayerManager(Game* game, const std::string& localID);
        ~PlayerManager();
        void AddLocalPlayer(const std::string& id, const std::string& name, const sf::Vector2f& position, const sf::Color& color);
        RemotePlayer& GetLocalPlayer();
        void Update(float dt);
        void AddOrUpdatePlayer(const std::string& id, const RemotePlayer& player);
        void RemovePlayer(const std::string& id);
        std::unordered_map<std::string, RemotePlayer>& GetPlayers();
    private:
        Game* game;
        std::unordered_map<std::string, RemotePlayer> players;
        std::string localPlayerID;
    };

#endif // PLAYER_MANAGER_H
