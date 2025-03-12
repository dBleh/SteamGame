#include "LobbyState.h"
#include <iostream>
#include <Steam/steam_api.h>
#include "../network/Client.h"
#include "../network/Host.h"
#include "../render/PlayerRenderer.h"
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

    localPlayerName.setFont(game->GetFont());
    localPlayerName.setString(SteamFriends()->GetPersonaName());
    localPlayerName.setCharacterSize(16);
    localPlayerName.setFillColor(sf::Color::Black);

    // Create the player manager and renderer.
    playerManager = std::make_unique<PlayerManager>(game);
    playerRenderer = std::make_unique<PlayerRenderer>(playerManager.get());

    // Determine role: host or client.
    CSteamID myID = SteamUser()->GetSteamID();
    CSteamID hostIDSteam = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
    if (myID == hostIDSteam) {
        // Instantiate HostNetwork with the player manager.
        hostNetwork = std::make_unique<HostNetwork>(game, playerManager.get());
        game->GetNetworkManager().SetMessageHandler(
            [this](const std::string& msg, CSteamID sender) {
                hostNetwork->ProcessMessage(msg, sender);
            }
        );
        // Add the host (local player) to the player manager.
        RemotePlayer rp;
        rp.player = localPlayer;
        rp.nameText = localPlayerName;
        playerManager->AddOrUpdatePlayer(std::to_string(myID.ConvertToUint64()), rp);
    } else {
        // Instantiate ClientNetwork with the player manager.
        clientNetwork = std::make_unique<ClientNetwork>(game, playerManager.get());
        game->GetNetworkManager().SetMessageHandler(
            [this](const std::string& msg, CSteamID sender) {
                clientNetwork->ProcessMessage(msg, sender);
            }
        );
        // Client sends its connection message to the host.
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
        localPlayer.Update(dt);
        CSteamID myID = SteamUser()->GetSteamID();
        CSteamID hostIDSteam = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
        if (myID != hostIDSteam) {
            // Client sends movement updates to the host.
            static float sendTimer = 0.f;
            sendTimer += dt;
            if (sendTimer >= 0.1f && clientNetwork) {
                clientNetwork->SendMovementUpdate(localPlayer.GetPosition());
                sendTimer = 0.f;
            }
        } else {
            // Host updates its own position and broadcasts the updated players list.
            static float broadcastTimer = 0.f;
            broadcastTimer += dt;
            if (broadcastTimer >= 0.5f && hostNetwork) {
                hostNetwork->ProcessMessage(
                    MessageHandler::FormatMovementMessage(
                        std::to_string(myID.ConvertToUint64()),
                        localPlayer.GetPosition()
                    ),
                    myID
                );
                hostNetwork->BroadcastPlayersList();
                broadcastTimer = 0.f;
            }
        }
    }
    // Update the player manager (e.g., reposition names).
    playerManager->Update(dt);
    localPlayerName.setPosition(localPlayer.GetPosition().x, localPlayer.GetPosition().y - 20.f);
    if (SteamUser()->GetSteamID() == SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID())) {
        game->GetHUD().updateText("startGame", "Press S to Start Game (Host Only)");
    } else {
        game->GetHUD().updateText("startGame", "");
    }
    if (clientNetwork) clientNetwork->Update(dt);
    if (hostNetwork) hostNetwork->Update(dt);
}

void LobbyState::Render() {
    game->GetWindow().clear(sf::Color::White);
    if (playerLoaded) {
        game->GetWindow().draw(localPlayer.GetShape());
        game->GetWindow().draw(localPlayerName);
    }
    // Delegate all player rendering to the PlayerRenderer.
    playerRenderer->Render(game->GetWindow());
    game->GetHUD().render(game->GetWindow(), game->GetWindow().getDefaultView(), game->GetCurrentState());
    game->GetWindow().display();
}


void LobbyState::ProcessEvent(const sf::Event& event) {
    ProcessEvents(event);
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
