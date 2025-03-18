#ifndef STEAM_HELPERS_H
#define STEAM_HELPERS_H

#include "../entities/Player.h"
#include "../entities/Bullet.h"
#include <SFML/Graphics.hpp>
#include <steam/steam_api.h>
#include <cstdint>
#include <functional>
#include <unordered_map>
#include <string>
#include <chrono>
#include <vector>

// Forward declarations
class Game;
class PlayingState;
class EnemyManager;
class HostNetwork;
class ClientNetwork;
class Player;

/**
 * @brief Hash functor for CSteamID.
 *
 * Converts a CSteamID to a uint64_t and returns its hash value.
 */
struct CSteamIDHash {
    std::size_t operator()(const CSteamID& id) const {
        return std::hash<uint64_t>{}(id.ConvertToUint64());
    }
};

struct RemotePlayer {
    std::string playerID;
    bool isHost;
    bool isReady = false;
    Player player;
    
    // Name display
    sf::Text nameText;
    std::string baseName;
    sf::Color cubeColor;
    
    // Position interpolation
    sf::Vector2f previousPosition;
    sf::Vector2f targetPosition;
    std::chrono::steady_clock::time_point lastUpdateTime;
    float interpDuration = 0.1f; // Time to interpolate between positions
    
    // Game stats
    int kills = 0;
    int money = 0;
    
    // Respawn data
    float respawnTimer = 0.0f;
    
    // Custom move constructor
    RemotePlayer(RemotePlayer&& other) noexcept : 
        playerID(std::move(other.playerID)),
        isHost(other.isHost),
        isReady(other.isReady),
        player(std::move(other.player)),  // This will use Player's move constructor
        nameText(std::move(other.nameText)),
        baseName(std::move(other.baseName)),
        cubeColor(other.cubeColor),
        previousPosition(other.previousPosition),
        targetPosition(other.targetPosition),
        lastUpdateTime(other.lastUpdateTime),
        interpDuration(other.interpDuration),
        kills(other.kills),
        money(other.money),
        respawnTimer(other.respawnTimer) {}
    
    // Custom move assignment operator
    RemotePlayer& operator=(RemotePlayer&& other) noexcept {
        if (this != &other) {
            playerID = std::move(other.playerID);
            isHost = other.isHost;
            isReady = other.isReady;
            player = std::move(other.player);  // This will use Player's move assignment
            nameText = std::move(other.nameText);
            baseName = std::move(other.baseName);
            cubeColor = other.cubeColor;
            previousPosition = other.previousPosition;
            targetPosition = other.targetPosition;
            lastUpdateTime = other.lastUpdateTime;
            interpDuration = other.interpDuration;
            kills = other.kills;
            money = other.money;
            respawnTimer = other.respawnTimer;
        }
        return *this;
    }
    
    // Default constructor needed for STL containers
    RemotePlayer() = default;
    
    // Delete copy constructor and assignment operator
    RemotePlayer(const RemotePlayer&) = delete;
    RemotePlayer& operator=(const RemotePlayer&) = delete;
};

inline std::string GetSteamIDString(CSteamID id) {
    return std::to_string(id.ConvertToUint64());
}

// Helper function to get the PlayingState from Game
// This is declared in the header but defined in a separate cpp file
// to prevent multiple definition errors
PlayingState* GetPlayingState(Game* game);

#endif // STEAM_HELPERS_H