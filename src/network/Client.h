#ifndef CLIENT_H
#define CLIENT_H

#include <SFML/Graphics.hpp>
#include <steam/steam_api.h>
#include <string>
#include "../utils/MessageHandler.h"
#include "../entities/Player.h"
#include "../utils/SteamHelpers.h"
#include "../entities/PlayerManager.h"

class Game;

class ClientNetwork {
public:
    explicit ClientNetwork(Game* game, PlayerManager* manager);
    ~ClientNetwork();
    void ProcessMessage(const std::string& msg, CSteamID sender);
    std::unordered_map<std::string, RemotePlayer>& GetRemotePlayers() { return remotePlayers; }
    void SendMovementUpdate(const sf::Vector2f& position);
    void SendChatMessage(const std::string& message);
    void SendConnectionMessage();
    void Update(); // Time-based update

private:
    Game* game;
    std::chrono::steady_clock::time_point lastSendTime; // Time-based sending
    PlayerManager* playerManager;
    CSteamID hostID;
    bool pendingConnectionMessage{false};
    std::string pendingMessage;
    std::unordered_map<std::string, RemotePlayer> remotePlayers;
    static constexpr float SEND_INTERVAL = 0.1f; // Send every 0.1s
};

#endif // CLIENT_H