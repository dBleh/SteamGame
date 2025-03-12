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
            // Only update remote players, not self
            RemotePlayer rp;
            rp.player = Player(parsed.position, sf::Color::Blue);
            rp.nameText.setFont(game->GetFont());
            rp.nameText.setString("Player_" + parsed.steamID);
            rp.nameText.setCharacterSize(16);
            rp.nameText.setFillColor(sf::Color::Black);
            playerManager->AddOrUpdatePlayer(parsed.steamID, rp);
        }
    }
}

void ClientNetwork::SendMovementUpdate(const sf::Vector2f& position) {
    std::string msg = MessageHandler::FormatMovementMessage(
        std::to_string(SteamUser()->GetSteamID().ConvertToUint64()),
        position
    );
    if (!game->GetNetworkManager().SendMessage(hostID, msg)) {
        std::cout << "[CLIENT] Failed to send movement update.\n";
    }
}

void ClientNetwork::SendChatMessage(const std::string& message) {
    std::string msg = MessageHandler::FormatChatMessage(
        std::to_string(SteamUser()->GetSteamID().ConvertToUint64()),
        message
    );
    if (!game->GetNetworkManager().SendMessage(hostID, msg)) {
        std::cout << "[CLIENT] Chat message failed to send.\n";
    }
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
}