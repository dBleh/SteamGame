#include "Host.h"
#include "../Game.h"
#include "../network/NetworkManager.h"
#include "../utils/MessageHandler.h"
#include <iostream>

HostNetwork::HostNetwork(Game* game)
    : game(game)
{
    // Optionally, initialize any host-specific data.
}

HostNetwork::~HostNetwork() {}

void HostNetwork::ProcessMessage(const std::string& msg, CSteamID sender) {
    ParsedMessage parsed = MessageHandler::ParseMessage(msg);
    if (parsed.type == MessageType::Connection) {
        std::string key = parsed.steamID;
        if (remotePlayers.find(key) == remotePlayers.end()) {
            RemotePlayer newPlayer;
            newPlayer.player.SetPosition(sf::Vector2f(200.f, 200.f)); // Default spawn position.
            newPlayer.player.GetShape().setFillColor(parsed.color);
            newPlayer.nameText.setFont(game->GetFont());
            newPlayer.nameText.setString(parsed.steamName);
            newPlayer.nameText.setCharacterSize(16);
            newPlayer.nameText.setFillColor(sf::Color::Black);
            remotePlayers[key] = newPlayer;
            std::cout << "[HOST] New player added: " << parsed.steamID << " (" << parsed.steamName << ")\n";
            BroadcastPlayersList();
        }
    }
    else if (parsed.type == MessageType::Movement) {
        std::string key = parsed.steamID;
        auto it = remotePlayers.find(key);
        if (it != remotePlayers.end()) {
            it->second.player.SetPosition(parsed.position);
            std::cout << "[HOST] Updated movement for " << key 
                      << " to (" << parsed.position.x << ", " << parsed.position.y << ")\n";
        } else {
            // If movement arrives before a connection message, add the remote player.
            RemotePlayer newPlayer;
            newPlayer.player.SetPosition(parsed.position);
            newPlayer.player.GetShape().setFillColor(sf::Color::Blue);
            newPlayer.nameText.setFont(game->GetFont());
            newPlayer.nameText.setString("Player_" + parsed.steamID);
            newPlayer.nameText.setCharacterSize(16);
            newPlayer.nameText.setFillColor(sf::Color::Black);
            remotePlayers[key] = newPlayer;
            std::cout << "[HOST] Added remote player on movement: " << parsed.steamID << "\n";
        }
    }
    else if (parsed.type == MessageType::Chat) {
        ProcessChatMessage(parsed.chatMessage, sender);
    }
}

void HostNetwork::BroadcastPlayersList() {
    // Broadcast each remote player's updated position.
    for (const auto& pair : remotePlayers) {
        std::string key = pair.first;
        const RemotePlayer& rp = pair.second;
        std::string msg = MessageHandler::FormatMovementMessage(
            key,
            rp.player.GetPosition()
        );
        if (game->GetNetworkManager().BroadcastMessage(msg)) {
            std::cout << "[HOST] Broadcasted position for " << key << "\n";
        }
    }
}

void HostNetwork::ProcessChatMessage(const std::string& message, CSteamID sender) {
    std::cout << "[HOST] Chat message from " << sender.ConvertToUint64() 
              << ": " << message << "\n";
    // Broadcast the chat message to all clients.
    std::string msg = MessageHandler::FormatChatMessage(
        std::to_string(sender.ConvertToUint64()), 
        message
    );
    game->GetNetworkManager().BroadcastMessage(msg);
}

void HostNetwork::Update(float dt) {
    // Optionally implement periodic tasks (e.g., timed broadcasts).
}
