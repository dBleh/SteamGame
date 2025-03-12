#include "Client.h"
#include "../Game.h"
#include "../network/NetworkManager.h"
#include "../utils/MessageHandler.h"
#include <iostream>

ClientNetwork::ClientNetwork(Game* game, PlayerManager* manager)
    : game(game), playerManager(manager)
{
    // Determine the host from the lobby.
    hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
}

ClientNetwork::~ClientNetwork() {}

void ClientNetwork::ProcessMessage(const std::string& msg, CSteamID sender) {
    ParsedMessage parsed = MessageHandler::ParseMessage(msg);
    if (parsed.type == MessageType::Chat) {
        std::cout << "[CLIENT] Chat message received: " << parsed.chatMessage << std::endl;
        // Optionally update chat UI here.
    }
    else if (parsed.type == MessageType::Connection) {
        std::cout << "[CLIENT] Connection acknowledgment received." << std::endl;
    }
    else if (parsed.type == MessageType::Movement) {
        // Update the player manager with new movement info.
        RemotePlayer rp;
        rp.player.SetPosition(parsed.position);
        rp.player.GetShape().setFillColor(sf::Color::Blue);
        rp.nameText.setFont(game->GetFont());
        rp.nameText.setString("Player_" + parsed.steamID);
        rp.nameText.setCharacterSize(16);
        rp.nameText.setFillColor(sf::Color::Black);
        playerManager->AddOrUpdatePlayer(parsed.steamID, rp);
        std::cout << "[CLIENT] Processed movement for " << parsed.steamID << "\n";
    }
}

void ClientNetwork::SendMovementUpdate(const sf::Vector2f& position) {
    std::string msg = MessageHandler::FormatMovementMessage(
        std::to_string(SteamUser()->GetSteamID().ConvertToUint64()),
        position
    );
    if (!game->GetNetworkManager().SendMessage(hostID, msg)) {
        std::cout << "[CLIENT] Failed to send movement update.\n";
    } else {
        std::cout << "[CLIENT] Movement update sent: " << msg << "\n";
    }
}

void ClientNetwork::SendChatMessage(const std::string& message) {
    std::string msg = MessageHandler::FormatChatMessage(
        std::to_string(SteamUser()->GetSteamID().ConvertToUint64()),
        message
    );
    if (!game->GetNetworkManager().SendMessage(hostID, msg)) {
        std::cout << "[CLIENT] Chat message failed to send.\n";
    } else {
        std::cout << "[CLIENT] Chat message sent: " << message << "\n";
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

void ClientNetwork::Update(float dt) {
    if (pendingConnectionMessage) {
        if (game->GetNetworkManager().SendMessage(hostID, pendingMessage)) {
            std::cout << "[CLIENT] Pending connection message sent successfully.\n";
            pendingConnectionMessage = false;
        }
    }
}
