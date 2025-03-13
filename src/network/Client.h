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
    void SendReadyStatus(bool isReady);
    void Update();
    void ProcessBulletMessage(const ParsedMessage& parsed);  // Add to private section
    CSteamID GetHostID() const { return hostID; }
private:
    void ProcessChatMessage(const ParsedMessage& parsed);
    void ProcessConnectionMessage(const ParsedMessage& parsed);
    void ProcessReadyStatusMessage(const ParsedMessage& parsed);
    void ProcessMovementMessage(const ParsedMessage& parsed);

    Game* game;
    std::chrono::steady_clock::time_point lastSendTime;
    PlayerManager* playerManager;
    CSteamID hostID;
    bool pendingConnectionMessage{false};
    std::string pendingMessage;
    std::string pendingReadyMessage;
    std::unordered_map<std::string, RemotePlayer> remotePlayers;
    static constexpr float SEND_INTERVAL = 0.1f;
};

#endif // CLIENT_H