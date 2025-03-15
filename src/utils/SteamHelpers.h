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
    Player player;
    sf::Text nameText;
    std::string baseName;
    sf::Color cubeColor;
    bool isReady = false;
    bool isHost = false;
    
    // Interpolation values for network movement
    sf::Vector2f previousPosition;
    sf::Vector2f targetPosition;
    std::chrono::steady_clock::time_point lastUpdateTime;
    float interpDuration = 0.1f;
    std::vector<Bullet> bullets;
    // Respawn timer
    float respawnTimer = 0.0f;
    
    // Player stats
    int kills = 0;
    int money = 0;
};

// Helper function to get the PlayingState from Game
// This is declared in the header but defined in a separate cpp file
// to prevent multiple definition errors
PlayingState* GetPlayingState(Game* game);

#endif // STEAM_HELPERS_H