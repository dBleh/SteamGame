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
        rp.nameText.setString(parsed.steamName); // Use Steam name
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
        // Fix: Use playerManager->GetPlayers() to get the existing name
        auto& playersMap = playerManager->GetPlayers();
        if (playersMap.find(parsed.steamID) != playersMap.end()) {
            rp.nameText.setString(playersMap[parsed.steamID].nameText.getString()); // Preserve Steam name
        } else {
            rp.nameText.setString("Player_" + parsed.steamID); // Fallback if not found
        }
        rp.nameText.setCharacterSize(16);
        rp.nameText.setFillColor(sf::Color::Black);
        playerManager->AddOrUpdatePlayer(parsed.steamID, rp);
        std::string broadcastMsg = MessageHandler::FormatMovementMessage(parsed.steamID, parsed.position);
        game->GetNetworkManager().BroadcastMessage(broadcastMsg);
    } else if (parsed.type == MessageType::Chat) {
        ProcessChatMessage(parsed.chatMessage, sender);
    } else if (parsed.type == MessageType::ReadyStatus) {
        playerManager->SetReadyStatus(parsed.steamID, parsed.isReady);
        std::string broadcastMsg = MessageHandler::FormatReadyStatusMessage(parsed.steamID, parsed.isReady);
        game->GetNetworkManager().BroadcastMessage(broadcastMsg);
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
    // Check for ready toggle (host only)
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::R) && elapsed >= 0.2f) {
        std::string myID = std::to_string(SteamUser()->GetSteamID().ConvertToUint64());
        bool currentReady = playerManager->GetLocalPlayer().isReady;
        playerManager->SetReadyStatus(myID, !currentReady);
        std::string msg = MessageHandler::FormatReadyStatusMessage(myID, !currentReady);
        game->GetNetworkManager().BroadcastMessage(msg);
        lastBroadcastTime = now; // Debounce
    }
}