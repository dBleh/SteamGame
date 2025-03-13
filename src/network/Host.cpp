#include "Host.h"
#include "../Game.h"
#include "../network/NetworkManager.h"
#include "../utils/MessageHandler.h"
#include <iostream>

HostNetwork::HostNetwork(Game* game, PlayerManager* manager)
    : game(game), playerManager(manager), lastBroadcastTime(std::chrono::steady_clock::now()) {
    // Add host to the player list
    CSteamID hostID = game->GetLocalSteamID();  // Host’s SteamID
    std::string hostIDStr = std::to_string(hostID.ConvertToUint64());
    std::string hostName = SteamFriends()->GetPersonaName();  // Host’s Steam name

    RemotePlayer hostPlayer;
    hostPlayer.playerID = hostIDStr;
    hostPlayer.isHost = true;
    hostPlayer.player = Player(sf::Vector2f(200.f, 200.f), sf::Color::Blue);  // Initial position and color
    hostPlayer.cubeColor = sf::Color::Blue;
    hostPlayer.nameText.setFont(game->GetFont());
    hostPlayer.nameText.setString(hostName);
    hostPlayer.baseName = hostName;
    hostPlayer.nameText.setCharacterSize(16);
    hostPlayer.nameText.setFillColor(sf::Color::Black);
    hostPlayer.isReady = false;  // Initial ready status

    // Add or update the host in PlayerManager
    playerManager->AddOrUpdatePlayer(hostIDStr, hostPlayer);
    std::cout << "[HOST] Added host to player list: " << hostName << " (" << hostIDStr << ")\n";

    // Immediately broadcast the full player list to ensure clients get the host’s data
    BroadcastFullPlayerList();
}

HostNetwork::~HostNetwork() {}

void HostNetwork::ProcessMessage(const std::string& msg, CSteamID sender) {
    ParsedMessage parsed = MessageHandler::ParseMessage(msg);
    switch (parsed.type) {
        case MessageType::Connection:
            ProcessConnectionMessage(parsed);
            break;
        case MessageType::Movement:
            ProcessMovementMessage(parsed, sender);
            break;
        case MessageType::Chat:
            ProcessChatMessage(parsed.chatMessage, sender);
            break;
        case MessageType::ReadyStatus:
            ProcessReadyStatusMessage(parsed);
            break;
        case MessageType::Bullet:
            ProcessBulletMessage(parsed);
            break;
        default:
            std::cout << "[HOST] Unknown message type received: " << msg << "\n";
            break;
    }
}

void HostNetwork::ProcessConnectionMessage(const ParsedMessage& parsed) {
    auto& players = playerManager->GetPlayers();
    auto it = players.find(parsed.steamID);

    if (it == players.end()) {
        // New player: create and add
        RemotePlayer rp;
        rp.playerID = parsed.steamID;
        rp.isHost = false;
        rp.player = Player(sf::Vector2f(200.f, 200.f), parsed.color);
        rp.cubeColor = parsed.color;
        rp.nameText.setFont(game->GetFont());
        rp.nameText.setString(parsed.steamName);
        rp.baseName = parsed.steamName;
        rp.nameText.setCharacterSize(16);
        rp.nameText.setFillColor(sf::Color::Black);
        playerManager->AddOrUpdatePlayer(parsed.steamID, rp);
        std::cout << "[HOST] New player connected: " << parsed.steamName << " (" << parsed.steamID << ")\n";
    } else {
        // Existing player: update fields without overwriting name
        RemotePlayer& rp = it->second;
        rp.isHost = false;  // Always false for remote players on host
        rp.player.SetPosition(sf::Vector2f(200.f, 200.f));  // Initial position (could use parsed.position if sent)
        rp.cubeColor = parsed.color;
        if (rp.baseName != parsed.steamName) {
            rp.baseName = parsed.steamName;
            rp.nameText.setString(parsed.steamName);
            std::cout << "[HOST] Updated name for " << parsed.steamID << " to " << parsed.steamName << "\n";
        }
    }
    playerManager->SetReadyStatus(parsed.steamID, parsed.isReady);
    BroadcastFullPlayerList();
}
void HostNetwork::ProcessMovementMessage(const ParsedMessage& parsed, CSteamID sender) {
    if (parsed.steamID.empty()) {
        std::cout << "[HOST] Invalid movement message from " << sender.ConvertToUint64() << "\n";
        return;
    }
    RemotePlayer rp;
    rp.player = Player(parsed.position, sf::Color::Blue);
    rp.nameText.setFont(game->GetFont());
    auto& playersMap = playerManager->GetPlayers();
    rp.nameText.setCharacterSize(16);
    rp.nameText.setFillColor(sf::Color::Black);
    playerManager->AddOrUpdatePlayer(parsed.steamID, rp);
    std::string broadcastMsg = MessageHandler::FormatMovementMessage(parsed.steamID, parsed.position);
    game->GetNetworkManager().BroadcastMessage(broadcastMsg);
}

void HostNetwork::ProcessBulletMessage(const ParsedMessage& parsed) {
    playerManager->AddBullet(parsed.steamID, parsed.position, parsed.direction, parsed.velocity);
    std::string broadcastMsg = MessageHandler::FormatBulletMessage(parsed.steamID, parsed.position, parsed.direction, parsed.velocity);
    game->GetNetworkManager().BroadcastMessage(broadcastMsg);
}

void HostNetwork::ProcessChatMessage(const std::string& message, CSteamID sender) {
    std::string msg = MessageHandler::FormatChatMessage(std::to_string(sender.ConvertToUint64()), message);
    game->GetNetworkManager().BroadcastMessage(msg);
}

void HostNetwork::ProcessReadyStatusMessage(const ParsedMessage& parsed) {
    std::string localSteamIDStr = std::to_string(game->GetLocalSteamID().ConvertToUint64());
    if (localSteamIDStr != parsed.steamID) {
        auto& players = playerManager->GetPlayers();
        if (players.find(parsed.steamID) != players.end() && players[parsed.steamID].isReady != parsed.isReady) {
            playerManager->SetReadyStatus(parsed.steamID, parsed.isReady);
        }
    }
    std::string broadcastMsg = MessageHandler::FormatReadyStatusMessage(parsed.steamID, parsed.isReady);
    game->GetNetworkManager().BroadcastMessage(broadcastMsg);
}
void HostNetwork::BroadcastFullPlayerList() {
    auto& players = playerManager->GetPlayers();
    for (const auto& pair : players) {
        const RemotePlayer& rp = pair.second;
        std::string msg = MessageHandler::FormatConnectionMessage(
            rp.playerID,
            rp.baseName,
            rp.cubeColor,
            rp.isReady,
            rp.isHost
        );
        game->GetNetworkManager().BroadcastMessage(msg);
    }
}
void HostNetwork::BroadcastPlayersList() {
    auto& players = playerManager->GetPlayers();
    for (auto& pair : players) {
        std::string msg = MessageHandler::FormatMovementMessage(pair.first, pair.second.player.GetPosition());
        game->GetNetworkManager().BroadcastMessage(msg);
    }
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