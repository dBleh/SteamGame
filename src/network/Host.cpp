#include "Host.h"
#include "../core/Game.h"
#include "NetworkManager.h"
#include "messages/MessageHandler.h"
#include "../states/PlayingState.h"
#include <iostream>

HostNetwork::HostNetwork(Game* game, PlayerManager* manager)
    : game(game), playerManager(manager), lastBroadcastTime(std::chrono::steady_clock::now()) {
    CSteamID hostID = game->GetLocalSteamID();
    std::string hostIDStr = std::to_string(hostID.ConvertToUint64());
    std::string hostName = SteamFriends()->GetPersonaName();
    RemotePlayer hostPlayer;
    hostPlayer.playerID = hostIDStr;
    hostPlayer.isHost = true;
    hostPlayer.player = Player(sf::Vector2f(200.f, 200.f), sf::Color::Blue);
    hostPlayer.cubeColor = sf::Color::Blue;
    hostPlayer.nameText.setFont(game->GetFont());
    hostPlayer.nameText.setString(hostName);
    hostPlayer.baseName = hostName;
    hostPlayer.nameText.setCharacterSize(16);
    hostPlayer.nameText.setFillColor(sf::Color::Black);
    hostPlayer.isReady = false;
    playerManager->AddOrUpdatePlayer(hostIDStr, hostPlayer);
    std::cout << "[HOST] Added host to player list: " << hostName << " (" << hostIDStr << ")\n";
    BroadcastFullPlayerList();
}

HostNetwork::~HostNetwork() {}

void HostNetwork::ProcessMessage(const std::string& msg, CSteamID sender) {
    ParsedMessage parsed = MessageHandler::ParseMessage(msg);
    const auto* descriptor = MessageHandler::GetDescriptorByType(parsed.type);
    if (descriptor && descriptor->hostHandler) {
        descriptor->hostHandler(*game, *this, parsed, sender);
    } else {
        std::cout << "[HOST] Unhandled message type received: " << msg << "\n";
        ProcessUnknownMessage(*game, *this, parsed, sender);
    }
}

void HostNetwork::ProcessConnectionMessage(Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender) {
    auto& players = playerManager->GetPlayers();
    auto it = players.find(parsed.steamID);
    if (it == players.end()) {
        RemotePlayer rp;
        rp.playerID = parsed.steamID;
        rp.isHost = false;
        rp.player = Player(sf::Vector2f(200.f, 200.f), parsed.color);
        rp.cubeColor = parsed.color;
        rp.nameText.setFont(game.GetFont());
        rp.nameText.setString(parsed.steamName);
        rp.baseName = parsed.steamName;
        rp.nameText.setCharacterSize(16);
        rp.nameText.setFillColor(sf::Color::Black);
        playerManager->AddOrUpdatePlayer(parsed.steamID, rp);
        std::cout << "[HOST] New player connected: " << parsed.steamName << " (" << parsed.steamID << ")\n";
    } else {
        RemotePlayer& rp = it->second;
        rp.isHost = false;
        rp.player.SetPosition(sf::Vector2f(200.f, 200.f));
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

void HostNetwork::ProcessMovementMessage(Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender) {
    if (parsed.steamID.empty()) {
        std::cout << "[HOST] Invalid movement message from " << sender.ConvertToUint64() << "\n";
        return;
    }
    RemotePlayer rp;
    rp.player = Player(parsed.position, sf::Color::Blue);
    rp.nameText.setFont(game.GetFont());
    auto& playersMap = playerManager->GetPlayers();
    rp.nameText.setCharacterSize(16);
    rp.nameText.setFillColor(sf::Color::Black);
    playerManager->AddOrUpdatePlayer(parsed.steamID, rp);
    std::string broadcastMsg = MessageHandler::FormatMovementMessage(parsed.steamID, parsed.position);
    game.GetNetworkManager().BroadcastMessage(broadcastMsg);
}

void HostNetwork::ProcessChatMessageParsed(Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender) {
    ProcessChatMessage(parsed.chatMessage, sender);
}

void HostNetwork::ProcessChatMessage(const std::string& message, CSteamID sender) {
    std::string msg = MessageHandler::FormatChatMessage(std::to_string(sender.ConvertToUint64()), message);
    game->GetNetworkManager().BroadcastMessage(msg);
}

void HostNetwork::ProcessReadyStatusMessage(Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender) {
    std::string localSteamIDStr = std::to_string(game.GetLocalSteamID().ConvertToUint64());
    if (localSteamIDStr != parsed.steamID) {
        auto& players = playerManager->GetPlayers();
        if (players.find(parsed.steamID) != players.end() && players[parsed.steamID].isReady != parsed.isReady) {
            playerManager->SetReadyStatus(parsed.steamID, parsed.isReady);
        }
    }
    std::string broadcastMsg = MessageHandler::FormatReadyStatusMessage(parsed.steamID, parsed.isReady);
    game.GetNetworkManager().BroadcastMessage(broadcastMsg);
}

void HostNetwork::ProcessBulletMessage(Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender) {
    std::string localSteamIDStr = std::to_string(game.GetLocalSteamID().ConvertToUint64());
    std::string normalizedShooterID;
    std::string normalizedLocalID;
    try {
        uint64_t shooterNum = std::stoull(parsed.steamID);
        normalizedShooterID = std::to_string(shooterNum);
        uint64_t localNum = std::stoull(localSteamIDStr);
        normalizedLocalID = std::to_string(localNum);
    } catch (const std::exception& e) {
        std::cout << "[HOST] Error normalizing IDs: " << e.what() << "\n";
        normalizedShooterID = parsed.steamID;
        normalizedLocalID = localSteamIDStr;
    }
    if (parsed.direction.x == 0.f && parsed.direction.y == 0.f) {
        std::cout << "[HOST] Received invalid bullet direction, ignoring\n";
        return;
    }
    std::string broadcastMsg = MessageHandler::FormatBulletMessage(normalizedShooterID, parsed.position, parsed.direction, parsed.velocity);
    bool sent = game.GetNetworkManager().BroadcastMessage(broadcastMsg);
    if (sent) std::cout << "[HOST] Broadcast bullet from " << normalizedShooterID << "\n";
    else std::cout << "[HOST] Failed to broadcast bullet message\n";
    if (normalizedShooterID == normalizedLocalID) {
        std::cout << "[HOST] Ignoring own bullet that was received as a message\n";
        return;
    }
    playerManager->AddBullet(normalizedShooterID, parsed.position, parsed.direction, parsed.velocity);
}

void HostNetwork::BroadcastFullPlayerList() {
    auto& players = playerManager->GetPlayers();
    for (const auto& pair : players) {
        const RemotePlayer& rp = pair.second;
        std::string msg = MessageHandler::FormatConnectionMessage(rp.playerID, rp.baseName, rp.cubeColor, rp.isReady, rp.isHost);
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
}

void HostNetwork::ProcessPlayerDeathMessage(Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender) {
    std::string playerID = parsed.steamID;
    std::string killerID = parsed.killerID;
    auto& players = playerManager->GetPlayers();
    if (players.find(playerID) != players.end()) {
        RemotePlayer& player = players[playerID];
        player.player.TakeDamage(100);
        player.respawnTimer = 3.0f;
    }
    if (players.find(killerID) != players.end()) {
        playerManager->IncrementPlayerKills(killerID);
    }
    std::string deathMsg = MessageHandler::FormatPlayerDeathMessage(playerID, killerID);
    game.GetNetworkManager().BroadcastMessage(deathMsg);
}

void HostNetwork::ProcessPlayerRespawnMessage(Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender) {
    std::string playerID = parsed.steamID;
    sf::Vector2f respawnPos = parsed.position;
    auto& players = playerManager->GetPlayers();
    if (players.find(playerID) != players.end()) {
        RemotePlayer& player = players[playerID];
        player.player.SetRespawnPosition(respawnPos);
        player.player.Respawn();
    }
    std::string respawnMsg = MessageHandler::FormatPlayerRespawnMessage(playerID, respawnPos);
    game.GetNetworkManager().BroadcastMessage(respawnMsg);
}

void HostNetwork::ProcessStartGameMessage(Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender) {
    std::cout << "[HOST] Received start game message, changing to Playing state\n";
    if (game.GetCurrentState() != GameState::Playing) {
        game.SetCurrentState(GameState::Playing);
    }
}

void HostNetwork::ProcessPlayerDamageMessage(Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender) {
    std::cout << "[HOST] Received player damage message for player " << parsed.steamID << "\n";
}

void HostNetwork::ProcessUnknownMessage(Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender) {
    std::cout << "[HOST] Unknown message type received\n";
}