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

// Forward declarations
class Game;
struct RemotePlayer;

// RemotePlayer definition


class PlayerManager {
public:
    PlayerManager(Game* game, const std::string& localID);
    ~PlayerManager();

    void Update();
    void AddOrUpdatePlayer(const std::string& id, const RemotePlayer& player);
    void AddLocalPlayer(const std::string& id, const std::string& name, const sf::Vector2f& position, const sf::Color& color);
    void SetReadyStatus(const std::string& id, bool ready);
    void RemovePlayer(const std::string& id);
    
    // Bullet methods
    void AddBullet(const std::string& shooterID, const sf::Vector2f& position, const sf::Vector2f& direction, float velocity);
    void CheckBulletCollisions();
    
    // Player death and respawn methods
    void PlayerDied(const std::string& playerID, const std::string& killerID);
    void RespawnPlayer(const std::string& playerID);
    void IncrementPlayerKills(const std::string& playerID);
    
    RemotePlayer& GetLocalPlayer();
    std::unordered_map<std::string, RemotePlayer>& GetPlayers();
    const std::vector<Bullet>& GetBullets() const { return bullets; }

private:
    Game* game;
    std::unordered_map<std::string, RemotePlayer> players;
    std::string localPlayerID;
    std::vector<Bullet> bullets;
    std::chrono::steady_clock::time_point lastFrameTime;
};

#endif // PLAYER_MANAGER_H