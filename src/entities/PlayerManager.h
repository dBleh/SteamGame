#ifndef PLAYER_MANAGER_H
#define PLAYER_MANAGER_H

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <chrono>
#include <thread>
#include <SFML/Graphics.hpp>
#include "../network/messages/MessageHandler.h"
#include "Player.h"
#include "Bullet.h"
#include "../utils/SteamHelpers.h"

class Game;

class PlayerManager {
public:
    PlayerManager(Game* game, const std::string& localPlayerID);
    ~PlayerManager();

    // Main update methods
    void Update(Game* game);         // Primary update method with game reference
    void Update();                   // Legacy update method for backward compatibility

    // Player management
    void AddLocalPlayer(const std::string& id, const std::string& name, 
        const sf::Vector2f& position, const sf::Color& color);
void AddOrUpdatePlayer(const std::string& playerID, const RemotePlayer& player);
void AddOrUpdatePlayer(const std::string& playerID, RemotePlayer&& player);
void RemovePlayer(const std::string& id);
    RemotePlayer& GetLocalPlayer();
    std::unordered_map<std::string, RemotePlayer>& GetPlayers();
    
    // Player status management
    void SetReadyStatus(const std::string& playerID, bool isReady);
    bool AreAllPlayersReady() const;
    void IncrementPlayerKills(const std::string& playerID);
    void PlayerDied(const std::string& playerID, const std::string& killerID);
    void RespawnPlayer(const std::string& playerID);
    
    // Bullet management
    void AddBullet(const std::string& playerID, const sf::Vector2f& position, 
                   const sf::Vector2f& direction, float velocity);
    const std::vector<Bullet>& GetAllBullets() const;
    void RemoveBullets(const std::vector<size_t>& indicesToRemove);
    void CheckBulletCollisions();
    
    // New method for handling player shooting action
    bool PlayerShoot(const sf::Vector2f& mouseWorldPos);
    void SendBulletMessageToNetwork(const sf::Vector2f& position, const sf::Vector2f& direction, float bulletSpeed);

    void InitializeForceFields();
    void HandleForceFieldZap(const std::string& playerID, int enemyId, float damage, bool killed);

private:
    // Helper methods for organization
    void UpdatePlayers(float dt, Game* game);
    void HandlePlayerRespawn(float dt, const std::string& playerID, RemotePlayer& rp);
    void UpdatePlayerPosition(float dt, const std::string& playerID, RemotePlayer& rp, Game* game);
    void UpdatePlayerNameDisplay(const std::string& playerID, RemotePlayer& rp, Game* game);
    void UpdateBullets(float dt);

    // Private member variables
    Game* game;                      // Reference to main game object
    std::string localPlayerID;       // ID of the local player
    std::unordered_map<std::string, RemotePlayer> players; // All players in the game
    std::vector<Bullet> bullets;     // All active bullets
    std::chrono::steady_clock::time_point lastFrameTime; // Time of the last frame update
};

#endif // PLAYER_MANAGER_H