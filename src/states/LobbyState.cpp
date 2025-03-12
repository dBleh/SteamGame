#include "LobbyState.h"
#include <iostream>
#include <Steam/steam_api.h>

LobbyState::LobbyState(Game* game)
    : State(game),
      playerLoaded(false),
      loadingTimer(0.f),
      localPlayer(sf::Vector2f(400.f, 300.f), sf::Color::Blue),
      chatMessages("")
{
    std::string lobbyName = SteamMatchmaking()->GetLobbyData(game->GetLobbyID(), "name");
    if (lobbyName.empty()) {
        lobbyName = "Lobby";
    }

    game->GetHUD().addElement("lobbyHeader", lobbyName, 32, sf::Vector2f(SCREEN_WIDTH * 0.5f, 20.f), GameState::Lobby, HUD::RenderMode::ScreenSpace, true);
    game->GetHUD().updateBaseColor("lobbyHeader", sf::Color::White);
    game->GetHUD().addElement("playerLoading", "Loading player...", 24, sf::Vector2f(50.f, SCREEN_HEIGHT - 150.f), GameState::Lobby, HUD::RenderMode::ScreenSpace, false);
    game->GetHUD().addElement("startGame", "", 24, sf::Vector2f(SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT - 100.f), GameState::Lobby, HUD::RenderMode::ScreenSpace, true);
    game->GetHUD().updateBaseColor("startGame", sf::Color::Black);
    game->GetHUD().addElement("returnMain", "Press M to Return to Main Menu", 24, sf::Vector2f(SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT - 60.f), GameState::Lobby, HUD::RenderMode::ScreenSpace, true);
    game->GetHUD().updateBaseColor("returnMain", sf::Color::Black);
    game->GetHUD().addElement("chat", "Chat:\n", 20, sf::Vector2f(50.f, SCREEN_HEIGHT - 200.f), GameState::Lobby, HUD::RenderMode::ScreenSpace, false);

    game->GetNetworkManager().SetMessageHandler([this, game](const std::string& msg, CSteamID sender) {
        ParsedMessage parsed = MessageHandler::ParseMessage(msg);
        CSteamID myID = SteamUser()->GetSteamID();
        CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());

        if (parsed.type == MessageType::Connection) {
            if (myID == hostID) { // Host processes new player
                std::string key = parsed.steamID;
                if (remotePlayers.find(key) == remotePlayers.end()) {
                    RemotePlayer newPlayer;
                    newPlayer.player.SetPosition(sf::Vector2f(200.f, 200.f));
                    newPlayer.player.GetShape().setFillColor(parsed.color);
                    newPlayer.nameText.setFont(game->GetFont());
                    newPlayer.nameText.setString(parsed.steamName);
                    newPlayer.nameText.setCharacterSize(16);
                    newPlayer.nameText.setFillColor(sf::Color::Black);
                    remotePlayers[key] = newPlayer;
                    std::cout << "[HOST] New player added: " << parsed.steamID << " (" << parsed.steamName << ")\n";
                    BroadcastPlayersList(); // Broadcast updated list
                } else {
                    std::cout << "[HOST] Player already exists: " << parsed.steamID << "\n";
                }
            }
        } else if (parsed.type == MessageType::Movement) {
            std::string key = parsed.steamID;
            auto it = remotePlayers.find(key);
            if (it != remotePlayers.end()) {
                it->second.player.SetPosition(parsed.position);
                std::cout << "[LOBBY] Updated position for " << key << " to (" << parsed.position.x << ", " << parsed.position.y << ")\n";
            }
        } else if (parsed.type == MessageType::Chat) {
            chatMessages += parsed.chatMessage + "\n";
            game->GetHUD().updateText("chat", "Chat:\n" + chatMessages);
        } else {
            std::cout << "[LOBBY] Unknown message type received: " << msg << "\n";
        }
    });


}

void LobbyState::BroadcastPlayersList() {
    if (SteamUser()->GetSteamID() != SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID()))
        return;
    
    for (const auto& pair : remotePlayers) {
        const RemotePlayer& rp = pair.second;
        std::string msg = MessageHandler::FormatMovementMessage(
            pair.first,
            rp.player.GetPosition()
        ); // Use Movement message to send position
        game->GetNetworkManager().BroadcastMessage(msg);
        std::cout << "[HOST] Broadcasted position for " << pair.first << ": " << msg << "\n";
    }
}

void LobbyState::UpdateRemotePlayers() {
    for (auto& pair : remotePlayers) {
        RemotePlayer& rp = pair.second;
        sf::Vector2f pos = rp.player.GetPosition();
        rp.nameText.setPosition(pos.x, pos.y - 20.f);
    }
}

void LobbyState::Update(float dt) {
    if (!playerLoaded) {
        loadingTimer += dt;
        if (loadingTimer >= 2.0f) {
            playerLoaded = true;
            game->GetHUD().updateText("playerLoading", "");
        }
    } else {
        localPlayer.Update(dt);
        // Send local player's position to host if not the host
        CSteamID myID = SteamUser()->GetSteamID();
        CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
        if (myID != hostID) {
            static float sendTimer = 0.f;
            sendTimer += dt;
            if (sendTimer >= 0.1f) { // Send every 100ms
                std::string msg = MessageHandler::FormatMovementMessage(
                    std::to_string(myID.ConvertToUint64()),
                    localPlayer.GetPosition()
                );
                game->GetNetworkManager().SendMessage(hostID, msg);
                sendTimer = 0.f;
            }
        } else {
            // Host broadcasts periodically
            static float broadcastTimer = 0.f;
            broadcastTimer += dt;
            if (broadcastTimer >= 0.5f) { // Broadcast every 500ms
                BroadcastPlayersList();
                broadcastTimer = 0.f;
            }
        }
    }
    UpdateRemotePlayers();
    if (SteamUser()->GetSteamID() == SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID())) {
        game->GetHUD().updateText("startGame", "Press S to Start Game (Host Only)");
    } else {
        game->GetHUD().updateText("startGame", "");
    }
}

void LobbyState::Render() {
    game->GetWindow().clear(sf::Color::White);
    if (playerLoaded) {
        game->GetWindow().draw(localPlayer.GetShape());
    }
    for (const auto& pair : remotePlayers) {
        const RemotePlayer& rp = pair.second;
        game->GetWindow().draw(rp.player.GetShape()); // Draw the cube
        game->GetWindow().draw(rp.nameText);          // Draw the name above the cube
    }
    game->GetHUD().render(game->GetWindow(), game->GetWindow().getDefaultView(), game->GetCurrentState());
    game->GetWindow().display();
}

void LobbyState::ProcessEvent(const sf::Event& event)
{
    ProcessEvents(event);
}

void LobbyState::ProcessEvents(const sf::Event& event)
{
    if (event.type == sf::Event::KeyPressed)
    {
        if (event.key.code == sf::Keyboard::S &&
            SteamUser()->GetSteamID() == SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID()))
        {
            std::cout << "User wants to start game" << std::endl;
        }
    }
}

void LobbyState::UpdateLobbyMembers() {
    if (!game->IsInLobby()) return;
}

bool LobbyState::AllPlayersReady() {
    return true;
}

bool LobbyState::IsFullyLoaded() {
    return (
        game->GetHUD().isFullyLoaded() &&
        game->GetWindow().isOpen() &&
        playerLoaded
    );
}