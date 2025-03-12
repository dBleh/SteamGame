#ifndef CLIENT_H
#define CLIENT_H

#include <SFML/Graphics.hpp>
#include <steam/steam_api.h>
#include <string>
#include <unordered_map>
#include "../utils/MessageHandler.h"
#include "../entities/Player.h"

// Structure representing a remote player (client's view of host or others).
struct RemotePlayer {
    Player player;
    sf::Text nameText;
};

class Game; // Forward declaration

class ClientNetwork {
public:
    explicit ClientNetwork(Game* game);
    ~ClientNetwork();

    // Handle an incoming message from the host.
    void ProcessMessage(const std::string& msg, CSteamID sender);

    // Send a movement update (position) to the host.
    void SendMovementUpdate(const sf::Vector2f& position);

    // Send a chat message to the host.
    void SendChatMessage(const std::string& message);

    // Send the initial connection message to the host.
    void SendConnectionMessage();

    // Periodic update for retry logic, etc.
    void Update(float dt);

    // Getter for the remote players map.
    const std::unordered_map<std::string, RemotePlayer>& GetRemotePlayers() const { return remotePlayers; }

private:
    Game* game;
    CSteamID hostID;
    bool pendingConnectionMessage{false};
    std::string pendingMessage;
    // For the client, we also store remote players (e.g., the host and any broadcast updates).
    std::unordered_map<std::string, RemotePlayer> remotePlayers;
};

#endif // CLIENT_H
