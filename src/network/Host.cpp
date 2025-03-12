#include "Host.h"
#include "../Game.h"
#include "../network/NetworkManager.h"
#include "../utils/MessageHandler.h"
#include <iostream>

HostNetwork::HostNetwork(Game* game, PlayerManager* manager)
    : game(game), playerManager(manager), lastBroadcastTime(std::chrono::steady_clock::now()) {
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
        RemotePlayer rp;
        rp.player = Player(sf::Vector2f(200.f, 200.f), parsed.color);
        rp.nameText.setFont(game->GetFont());
        rp.nameText.setString(parsed.steamName);
        rp.baseName = parsed.steamName; // Set baseName
        rp.nameText.setCharacterSize(16);
        rp.nameText.setFillColor(sf::Color::Black);
        playerManager->AddOrUpdatePlayer(parsed.steamID, rp);
        BroadcastPlayersList();
    } else if (parsed.type == MessageType::Movement) {
        if (parsed.steamID.empty()) {
            std::cout << "[HOST] Invalid movement message from " << sender.ConvertToUint64() << ": " << msg << "\n";
            return;
        }
        RemotePlayer rp;
        rp.player = Player(parsed.position, sf::Color::Blue);
        rp.nameText.setFont(game->GetFont());
        auto& playersMap = playerManager->GetPlayers();
        if (playersMap.find(parsed.steamID) != playersMap.end()) {
            rp.nameText.setString(playersMap[parsed.steamID].nameText.getString());
            rp.baseName = playersMap[parsed.steamID].baseName; // Preserve baseName
        } else {
            rp.nameText.setString("Player_" + parsed.steamID);
            rp.baseName = "Player_" + parsed.steamID; // Fallback
        }
        rp.nameText.setCharacterSize(16);
        rp.nameText.setFillColor(sf::Color::Black);
        playerManager->AddOrUpdatePlayer(parsed.steamID, rp);
        std::string broadcastMsg = MessageHandler::FormatMovementMessage(parsed.steamID, parsed.position);
        game->GetNetworkManager().BroadcastMessage(broadcastMsg);
    } else if (parsed.type == MessageType::Chat) {
        ProcessChatMessage(parsed.chatMessage, sender);
    } else if (parsed.type == MessageType::ReadyStatus) {
        std::string broadcastMsg = MessageHandler::FormatReadyStatusMessage(parsed.steamID, parsed.isReady);
        if (game->GetNetworkManager().BroadcastMessage(broadcastMsg)) {
            std::cout << "[HOST] Broadcasted ready status: " << broadcastMsg << "\n";
        } else {
            std::cout << "[HOST] Failed to broadcast ready status: " << broadcastMsg << "\n";
        }
    }
}

void HostNetwork::BroadcastPlayersList() {
    auto& players = playerManager->GetPlayers();
    for (auto& pair : players) {
        std::string msg = MessageHandler::FormatMovementMessage(pair.first, pair.second.player.GetPosition());
        game->GetNetworkManager().BroadcastMessage(msg);
    }
}

void HostNetwork::ProcessChatMessage(const std::string& message, CSteamID sender) {
    std::string msg = MessageHandler::FormatChatMessage(std::to_string(sender.ConvertToUint64()), message);
    game->GetNetworkManager().BroadcastMessage(msg);
}

void HostNetwork::Update() {
    auto now = std::chrono::steady_clock::now();
    float elapsed = std::chrono::duration<float>(now - lastBroadcastTime).count();
    if (elapsed >= BROADCAST_INTERVAL) {
        BroadcastPlayersList();
        lastBroadcastTime = now;
    }
    // Removed "R" key logic, handled in LobbyState
}