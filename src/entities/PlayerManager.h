#ifndef PLAYER_MANAGER_H
#define PLAYER_MANAGER_H

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <chrono>
#include <SFML/Graphics.hpp>
#include "../utils/MessageHandler.h"
#include "../entities/Player.h"
#include "../entities/Bullet.h"
#include "../utils/SteamHelpers.h"

class Game;

class PlayerManager {
public:
    PlayerManager(Game* game, const std::string& localPlayerID);
    ~PlayerManager();

    const std::vector<Bullet>& GetBullets() const { return bullets; }  
    void CheckBulletCollisions();
    void Update();
    void AddLocalPlayer(const std::string& id, const std::string& name, const sf::Vector2f& position, const sf::Color& color);
    void RemovePlayer(const std::string& id);
    void Update(Game* game);
    // Player management
    void AddOrUpdatePlayer(const std::string& playerID, const RemotePlayer& player);
    RemotePlayer& GetLocalPlayer();
    std::unordered_map<std::string, RemotePlayer>& GetPlayers();
    void SetReadyStatus(const std::string& playerID, bool isReady);
    bool AreAllPlayersReady() const;
    void IncrementPlayerKills(const std::string& playerID);
    void RespawnPlayer(const std::string& playerID);
    // Bullet management
    void AddBullet(const std::string& playerID, const sf::Vector2f& position, 
                 const sf::Vector2f& direction, float velocity);
    const std::vector<Bullet>& GetAllBullets() const;
    void UpdateBullets(float dt);
    void PlayerDied(const std::string& playerID, const std::string& killerID);
    void RemoveBullets(const std::vector<size_t>& indicesToRemove);

private:
    Game* game;
    std::string localPlayerID;
    std::unordered_map<std::string, RemotePlayer> players;
    std::vector<Bullet> bullets;
    float shootCooldown;
    std::chrono::steady_clock::time_point lastFrameTime;
};

#endif // PLAYER_MANAGER_H

