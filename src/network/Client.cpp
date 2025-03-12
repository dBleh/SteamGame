#include "Client.h"
#include "../Game.h"
#include "../network/NetworkManager.h"
#include "../utils/MessageHandler.h"
#include <iostream>

ClientNetwork::ClientNetwork(Game* game, PlayerManager* manager)
    : game(game), playerManager(manager), lastSendTime(std::chrono::steady_clock::now()) {
    hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
}

ClientNetwork::~ClientNetwork() {}

void ClientNetwork::ProcessMessage(const std::string& msg, CSteamID sender) {
    ParsedMessage parsed = MessageHandler::ParseMessage(msg);
    if (parsed.type == MessageType::Chat) {
        // Chat UI update if needed
    } else if (parsed.type == MessageType::Connection) {
        std::cout << "[CLIENT] Connection acknowledgment received." << std::endl;
    } else if (parsed.type == MessageType::Movement) {
        if (parsed.steamID != std::to_string(SteamUser()->GetSteamID().ConvertToUint64())) {
            RemotePlayer rp;
            rp.player = Player(parsed.position, sf::Color::Blue);
            auto& playersMap = playerManager->GetPlayers();
            if (playersMap.find(parsed.steamID) != playersMap.end()) {
                rp.nameText.setString(playersMap[parsed.steamID].nameText.getString());
                rp.baseName = playersMap[parsed.steamID].baseName;
            } else {
                rp.nameText.setString(parsed.steamName.empty() ? "Player_" + parsed.steamID : parsed.steamName);
                rp.baseName = parsed.steamName.empty() ? "Player_" + parsed.steamID : parsed.steamName;
            }
            rp.nameText.setFont(game->GetFont());
            rp.nameText.setCharacterSize(16);
            rp.nameText.setFillColor(sf::Color::Black);
            playerManager->AddOrUpdatePlayer(parsed.steamID, rp);
        }
    } else if (parsed.type == MessageType::ReadyStatus) {
        std::cout << "[CLIENT] Received ready status for " << parsed.steamID << ": " 
                  << (parsed.isReady ? "true" : "false") << "\n";
        playerManager->SetReadyStatus(parsed.steamID, parsed.isReady);
    }
}

void ClientNetwork::SendMovementUpdate(const sf::Vector2f& position) {
    std::string msg = MessageHandler::FormatMovementMessage(
        std::to_string(SteamUser()->GetSteamID().ConvertToUint64()),
        position
    );
    game->GetNetworkManager().SendMessage(hostID, msg);
}

void ClientNetwork::SendChatMessage(const std::string& message) {
    std::string msg = MessageHandler::FormatChatMessage(
        std::to_string(SteamUser()->GetSteamID().ConvertToUint64()),
        message
    );
    game->GetNetworkManager().SendMessage(hostID, msg);
}

void ClientNetwork::SendConnectionMessage() {
    CSteamID myID = SteamUser()->GetSteamID();
    std::string steamIDStr = std::to_string(myID.ConvertToUint64());
    std::string steamName = SteamFriends()->GetPersonaName();
    sf::Color color = sf::Color::Blue;
    std::string connectMsg = MessageHandler::FormatConnectionMessage(steamIDStr, steamName, color);
    if (game->GetNetworkManager().SendMessage(hostID, connectMsg)) {
        std::cout << "[CLIENT] Connection message sent: " << connectMsg << "\n";
        pendingConnectionMessage = false;
    } else {
        std::cout << "[CLIENT] Failed to send connection message, will retry.\n";
        pendingConnectionMessage = true;
        pendingMessage = connectMsg;
    }
}

void ClientNetwork::SendReadyStatus(bool isReady) {
    std::string steamIDStr = std::to_string(SteamUser()->GetSteamID().ConvertToUint64());
    std::string msg = MessageHandler::FormatReadyStatusMessage(steamIDStr, isReady);
    if (game->GetNetworkManager().SendMessage(hostID, msg)) {
        std::cout << "[CLIENT] Sent ready status: " << (isReady ? "true" : "false") << " (" << msg << ")\n";
        playerManager->SetReadyStatus(steamIDStr, isReady); // Update locally
    } else {
        std::cout << "[CLIENT] Failed to send ready status: " << msg << " - will retry\n";
        // Optional: Queue for retry in Update if needed
    }
}

void ClientNetwork::Update() {
    auto now = std::chrono::steady_clock::now();
    float elapsed = std::chrono::duration<float>(now - lastSendTime).count();
    if (elapsed >= SEND_INTERVAL) {
        auto& localPlayer = playerManager->GetLocalPlayer().player;
        SendMovementUpdate(localPlayer.GetPosition());
        lastSendTime = now;
    }
    if (pendingConnectionMessage) {
        if (game->GetNetworkManager().SendMessage(hostID, pendingMessage)) {
            std::cout << "[CLIENT] Pending connection message sent successfully.\n";
            pendingConnectionMessage = false;
        }
    }
    // Removed "R" key logic, handled in LobbyState
}