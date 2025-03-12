#include "LobbyState.h"
#include <iostream>
#include <Steam/steam_api.h>
#include "../network/Client.h"
#include "../network/Host.h"
#include "../render/PlayerRenderer.h"
#include "../entities/PlayerManager.h"
LobbyState::LobbyState(Game* game)
    : State(game), playerLoaded(false), loadingTimer(0.f), chatMessages("") {
    // HUD initialization (unchanged)
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

    // Initialize PlayerManager with local ID
    CSteamID myID = SteamUser()->GetSteamID();
    std::string myIDStr = std::to_string(myID.ConvertToUint64());
    playerManager = std::make_unique<PlayerManager>(game, myIDStr);
    playerRenderer = std::make_unique<PlayerRenderer>(playerManager.get());

    // Add local player to PlayerManager
    std::string myName = SteamFriends()->GetPersonaName();
    playerManager->AddLocalPlayer(myIDStr, myName, sf::Vector2f(400.f, 300.f), sf::Color::Blue);

    // Network setup
    CSteamID hostIDSteam = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
    if (myID == hostIDSteam) {
        hostNetwork = std::make_unique<HostNetwork>(game, playerManager.get());
        game->GetNetworkManager().SetMessageHandler(
            [this](const std::string& msg, CSteamID sender) {
                hostNetwork->ProcessMessage(msg, sender);
            }
        );
    } else {
        clientNetwork = std::make_unique<ClientNetwork>(game, playerManager.get());
        game->GetNetworkManager().SetMessageHandler(
            [this](const std::string& msg, CSteamID sender) {
                clientNetwork->ProcessMessage(msg, sender);
            }
        );
        clientNetwork->SendConnectionMessage();
    }
}

void LobbyState::UpdateRemotePlayers() {
    if (hostNetwork) {
        std::unordered_map<std::string, RemotePlayer>& players = hostNetwork->GetRemotePlayers();
        for (auto it = players.begin(); it != players.end(); ++it) {
            RemotePlayer& rp = it->second;
            sf::Vector2f pos = rp.player.GetPosition();
            rp.nameText.setPosition(pos.x, pos.y - 20.f);
        }
    } else if (clientNetwork) {
        std::unordered_map<std::string, RemotePlayer>& players = clientNetwork->GetRemotePlayers();
        for (auto it = players.begin(); it != players.end(); ++it) {
            RemotePlayer& rp = it->second;
            sf::Vector2f pos = rp.player.GetPosition();
            rp.nameText.setPosition(pos.x, pos.y - 20.f);
        }
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
        // Update local player's position
        auto& localPlayer = playerManager->GetLocalPlayer().player;
        localPlayer.Update(dt); // Handles input

        // Delegate network updates
        if (clientNetwork) clientNetwork->Update(dt);
        if (hostNetwork) hostNetwork->Update(dt);

        // Update player list (name text positions)
        playerManager->Update(dt);
    }

    // Update HUD for host
    if (SteamUser()->GetSteamID() == SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID())) {
        game->GetHUD().updateText("startGame", "Press S to Start Game (Host Only)");
    } else {
        game->GetHUD().updateText("startGame", "");
    }
}

void LobbyState::Render() {
    game->GetWindow().clear(sf::Color::White);
    if (playerLoaded) {
        playerRenderer->Render(game->GetWindow()); // Renders all players
    }
    game->GetHUD().render(game->GetWindow(), game->GetWindow().getDefaultView(), game->GetCurrentState());
    game->GetWindow().display();
}


void LobbyState::ProcessEvent(const sf::Event& event) {
    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::S &&
            SteamUser()->GetSteamID() == SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID())) {
            std::cout << "User wants to start game" << std::endl;
        }
    }
}

void LobbyState::ProcessEvents(const sf::Event& event) {
    if (event.type == sf::Event::KeyPressed) {
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
