#ifndef HOST_H
#define HOST_H

#include <SFML/Graphics.hpp>
#include <steam/steam_api.h>
#include <string>
#include "../utils/MessageHandler.h"
#include "../entities/Player.h"
#include "../utils/SteamHelpers.h"
#include "../entities/PlayerManager.h"

class Game;

class HostNetwork {
public:
    explicit HostNetwork(Game* game, PlayerManager* manager);
    ~HostNetwork();
    void ProcessMessage(const std::string& msg, CSteamID sender);
    std::unordered_map<std::string, RemotePlayer>& GetRemotePlayers() { return remotePlayers; }
    void BroadcastFullPlayerList();
    
    void BroadcastPlayersList();
    void ProcessChatMessage(const std::string& message, CSteamID sender);
    void Update(); // Time-based
    void ProcessBulletMessage(const ParsedMessage& parsed);
private:
    void ProcessConnectionMessage(const ParsedMessage& parsed);
    void ProcessMovementMessage(const ParsedMessage& parsed, CSteamID sender);
    void ProcessReadyStatusMessage(const ParsedMessage& parsed);
    Game* game;
    PlayerManager* playerManager;
    std::unordered_map<std::string, RemotePlayer> remotePlayers;
    std::chrono::steady_clock::time_point lastBroadcastTime;
    static constexpr float BROADCAST_INTERVAL = 0.1f; // Match client send rate
};

#endif // HOST_H