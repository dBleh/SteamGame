#ifndef CLIENT_H
#define CLIENT_H

#include <SFML/Graphics.hpp>
#include <steam/steam_api.h>
#include <string>
#include "../utils/MessageHandler.h"
#include "../entities/Player.h"
#include "../utils/SteamHelpers.h"
#include "../entities/PlayerManager.h"  // Use PlayerManager instead of a local map

class Game; // Forward declaration

class ClientNetwork {
public:
    explicit ClientNetwork(Game* game, PlayerManager* manager);
    ~ClientNetwork();

    // Handle an incoming message from the host.
    void ProcessMessage(const std::string& msg, CSteamID sender);
    std::unordered_map<std::string, RemotePlayer>& GetRemotePlayers() { return remotePlayers; }

    // Send a movement update (position) to the host.
    void SendMovementUpdate(const sf::Vector2f& position);

    // Send a chat message to the host.
    void SendChatMessage(const std::string& message);

    // Send the initial connection message to the host.
    void SendConnectionMessage();

    // Periodic update for retry logic, etc.
    void Update(float dt);

private:
    Game* game;
    float sendTimer = 0.f;
    PlayerManager* playerManager;
    CSteamID hostID;
    bool pendingConnectionMessage{false};
    std::string pendingMessage;
    std::unordered_map<std::string, RemotePlayer> remotePlayers;

};

#endif // CLIENT_H
