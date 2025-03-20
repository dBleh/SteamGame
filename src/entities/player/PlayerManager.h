#ifndef PLAYER_MANAGER_H
#define PLAYER_MANAGER_H

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <chrono>
#include <thread>
#include <SFML/Graphics.hpp>
#include "../../network/messages/MessageHandler.h"
#include "Player.h"
#include "Bullet.h"
#include "../../utils/SteamHelpers.h"
#include "../../utils/config/PlayerConfig.h"
#include "../../utils/config/BulletConfig.h"

class Game;
class EnemyManager;

// Forward declarations
class NetworkManager;
class HostNetwork;
class ClientNetwork;

class PlayerManager {
public:
    PlayerManager(Game* game, const std::string& localPlayerID);
    ~PlayerManager();

    // Main update methods
    void Update(float dt, Game* game);    // Primary update method with game reference
    void Update(Game* game);             // Secondary method that calls primary with default dt
    void Update();                      // Legacy update method for backward compatibility

    // Player management
    void AddLocalPlayer(const std::string& id, const std::string& name, 
        const sf::Vector2f& position, const sf::Color& color);
    void AddOrUpdatePlayer(const std::string& playerID, const RemotePlayer& player);
    void AddOrUpdatePlayer(const std::string& playerID, RemotePlayer&& player);
    void RemovePlayer(const std::string& id);
    RemotePlayer& GetLocalPlayer();
    std::unordered_map<std::string, RemotePlayer>& GetPlayers();
    
    // Player actions and status
    bool PlayerShoot(const sf::Vector2f& mouseWorldPos);
    void SetReadyStatus(const std::string& playerID, bool isReady);
    bool AreAllPlayersReady() const;
    
    // Event handlers (called by Player callbacks)
    void HandlePlayerDeath(const std::string& playerID, const sf::Vector2f& position, const std::string& killerID);
    void HandlePlayerRespawn(const std::string& playerID, const sf::Vector2f& position);
    void HandlePlayerDamage(const std::string& playerID, int amount, int actualDamage);
    
    // Tracking statistics
    void IncrementPlayerKills(const std::string& playerID);
    void HandleKill(const std::string& killerID, int enemyId);
    
    // Force field management
    void InitializeForceFields();
    void HandleForceFieldZap(const std::string& playerID, int enemyId, float damage, bool killed);
    
    // Bullet management
    void AddBullet(const std::string& playerID, const sf::Vector2f& position, 
                   const sf::Vector2f& direction, float velocity);
    const std::vector<Bullet>& GetAllBullets() const;
    void RemoveBullets(const std::vector<size_t>& indicesToRemove);
    void CheckBulletCollisions();
    
    // Network-related methods
    void SendBulletMessageToNetwork(const sf::Vector2f& position, const sf::Vector2f& direction, float bulletSpeed);
    void BroadcastPlayerDeath(const std::string& playerID, const sf::Vector2f& position, const std::string& killerID);
    void BroadcastPlayerRespawn(const std::string& playerID, const sf::Vector2f& position);
    
private:
    // Helper methods for organization
    void UpdatePlayers(float dt, Game* game);
    void UpdateRemotePlayerPosition(float dt, const std::string& playerID, RemotePlayer& rp);
    void UpdatePlayerNameDisplay(const std::string& playerID, RemotePlayer& rp, Game* game);
    void UpdateBullets(float dt);
    void InitializePlayerCallbacks(RemotePlayer& rp, const std::string& playerID);

    // Private member variables
    Game* game;                      // Reference to main game object
    std::string localPlayerID;       // ID of the local player
    std::unordered_map<std::string, RemotePlayer> players; // All players in the game
    std::vector<Bullet> bullets;     // All active bullets
    std::chrono::steady_clock::time_point lastFrameTime; // Time of the last frame update
    
    // Settings cache for quick reference
    float bulletDamage = BULLET_DAMAGE;
    float bulletSpeed = BULLET_SPEED;
};

#endif // PLAYER_MANAGER_H