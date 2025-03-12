#include "Host.h"
#include "../Game.h"
#include "../network/NetworkManager.h"
#include "../utils/MessageHandler.h"
#include <iostream>

HostNetwork::HostNetwork(Game* game, PlayerManager* manager)
    : game(game), playerManager(manager)
{
}

HostNetwork::~HostNetwork() {}

void HostNetwork::ProcessMessage(const std::string& msg, CSteamID sender) {
    ParsedMessage parsed = MessageHandler::ParseMessage(msg);
    if (parsed.type == MessageType::Connection) {
        if (parsed.steamID.empty() || parsed.steamName.empty()) {
            std::cout << "[HOST] Invalid connection message from " << sender.ConvertToUint64() 
                      << ": steamID='" << parsed.steamID << "', steamName='" << parsed.steamName << "'\n";
            return;
        }
        std::string key = parsed.steamID;
        RemotePlayer rp;
        rp.player.SetPosition(sf::Vector2f(200.f, 200.f));
        rp.player.GetShape().setFillColor(parsed.color); // Should be Blue from client
        rp.nameText.setFont(game->GetFont());
        rp.nameText.setString(parsed.steamName);
        rp.nameText.setCharacterSize(16);
        rp.nameText.setFillColor(sf::Color::Black);
        playerManager->AddOrUpdatePlayer(key, rp);
        
        BroadcastPlayersList();
    } else if (parsed.type == MessageType::Movement) {
        if (parsed.steamID.empty()) {
            std::cout << "[HOST] Invalid movement message from " << sender.ConvertToUint64() << ": " << msg << "\n";
            return;
        }
        std::string key = parsed.steamID;
        RemotePlayer rp;
        rp.player.SetPosition(parsed.position);
        rp.player.GetShape().setFillColor(sf::Color::Blue);
        rp.nameText.setFont(game->GetFont());
        rp.nameText.setString("Player_" + parsed.steamID);
        rp.nameText.setCharacterSize(16);
        rp.nameText.setFillColor(sf::Color::Black);
        playerManager->AddOrUpdatePlayer(key, rp);
        
    } else if (parsed.type == MessageType::Chat) {
        ProcessChatMessage(parsed.chatMessage, sender);
    }
}

void HostNetwork::BroadcastPlayersList() {
    auto& players = playerManager->GetPlayers();
    for (auto& pair : players) {
        std::string key = pair.first;
        // Use the current position from the player manager.
        RemotePlayer rp = pair.second; // For simplicity; ideally use a reference if safe.
        std::string msg = MessageHandler::FormatMovementMessage(
            key,
            rp.player.GetPosition()
        );
        if (game->GetNetworkManager().BroadcastMessage(msg)) {
            //std::cout << "[HOST] Broadcasted position for " << key << "\n";
        }
    }
}

void HostNetwork::ProcessChatMessage(const std::string& message, CSteamID sender) {
    std::string msg = MessageHandler::FormatChatMessage(
        std::to_string(sender.ConvertToUint64()), 
        message
    );
    game->GetNetworkManager().BroadcastMessage(msg);
}

void HostNetwork::Update(float dt) {
    broadcastTimer += dt;
    if (broadcastTimer >= 0.5f) {
        BroadcastPlayersList();
        broadcastTimer = 0.f;
    }
}