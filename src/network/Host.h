#ifndef HOST_H
#define HOST_H

#include <SFML/Graphics.hpp>
#include <steam/steam_api.h>
#include <string>
#include <unordered_map>
#include "../utils/MessageHandler.h"
#include "../entities/Player.h"

// Structure representing a remote player.
struct RemotePlayer {
    Player player;
    sf::Text nameText;
};

class Game; // Forward declaration

class HostNetwork {
public:
    explicit HostNetwork(Game* game);
    ~HostNetwork();

    // Process an incoming message from a client.
    void ProcessMessage(const std::string& msg, CSteamID sender);

    // Broadcast the current players' positions to all connected clients.
    void BroadcastPlayersList();

    // Process an incoming chat message from a client.
    void ProcessChatMessage(const std::string& message, CSteamID sender);

    // Periodic update for host networking tasks.
    void Update(float dt);

    // Getter for the remote players map.
    const std::unordered_map<std::string, RemotePlayer>& GetRemotePlayers() const { return remotePlayers; }

private:
    Game* game;
    // Maintain remote players using their Steam ID (as string) as the key.
    std::unordered_map<std::string, RemotePlayer> remotePlayers;
};

#endif // HOST_H
