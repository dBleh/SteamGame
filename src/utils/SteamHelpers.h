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
    Player player;
    sf::Text nameText;
    sf::Vector2f previousPosition;
    sf::Vector2f targetPosition;
    std::chrono::steady_clock::time_point lastUpdateTime; // Timestamp of last update
    float interpDuration; // Fixed duration between updates (e.g., 0.1s)
    bool isReady = false; 
};

#endif // STEAM_HELPERS_H
