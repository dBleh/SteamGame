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
    std::string playerID;         // Steam ID as string
    bool isHost = false;          // Host status
    Player player;               // Player entity with position and color
    sf::Text nameText;           // Display name
    sf::Vector2f previousPosition;
    sf::Vector2f targetPosition;
    std::chrono::steady_clock::time_point lastUpdateTime;
    float interpDuration = 0.1f;
    bool isReady = false;
    std::string baseName;
    sf::Color cubeColor;         // Store the player's cube color
    float respawnTimer = 0.0f;
};

#endif // STEAM_HELPERS_H
