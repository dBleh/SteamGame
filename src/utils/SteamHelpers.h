#ifndef STEAM_HELPERS_H
#define STEAM_HELPERS_H

#include "../entities/Player.h"
#include <SFML/Graphics.hpp>
#include <steam/steam_api.h>
#include <cstdint>
#include <functional>
#include <unordered_map>
#include <string>
#include <chrono>
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
    
    // Respawn timer
    float respawnTimer = 0.0f;
    
    // Player stats
    int kills = 0;
    int money = 0;
};

#endif // STEAM_HELPERS_H
