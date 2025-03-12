#ifndef HOST_H
#define HOST_H

#include <SFML/Graphics.hpp>
#include <steam/steam_api.h>
#include <string>
#include "../utils/MessageHandler.h"
#include "../entities/Player.h"
#include "../utils/SteamHelpers.h"
#include "../entities/PlayerManager.h"  // Use PlayerManager

class Game; // Forward declaration

class HostNetwork {
public:
    explicit HostNetwork(Game* game, PlayerManager* manager);
    ~HostNetwork();

    // Process an incoming message from a client.
    void ProcessMessage(const std::string& msg, CSteamID sender);
    std::unordered_map<std::string, RemotePlayer>& GetRemotePlayers() { return remotePlayers; }

    // Broadcast the current players' positions to all connected clients.
    void BroadcastPlayersList();

    // Process an incoming chat message from a client.
    void ProcessChatMessage(const std::string& message, CSteamID sender);

    // Periodic update for host networking tasks.
    void Update(float dt);

private:
float broadcastTimer = 0.f;
    Game* game;
    PlayerManager* playerManager;
    std::unordered_map<std::string, RemotePlayer> remotePlayers;

};

#endif // HOST_H
