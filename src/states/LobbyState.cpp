#include "LobbyState.h"
#include <iostream>
#include <Steam/steam_api.h>
#include "../network/Client.h"
#include "../network/Host.h"
#include "../render/PlayerRenderer.h"
#include "../entities/PlayerManager.h"
LobbyState::LobbyState(Game* game)
    : State(game), playerLoaded(false), loadingTimer(0.f), chatMessages(""),grid(50.f, sf::Color(220, 220, 220)), showGrid(true) {
    std::cout << "[DEBUG] LobbyState constructor start\n";
    
    // HUD initialization
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
    game->GetHUD().addElement("gridToggle", "Press G to toggle grid", 20, 
        sf::Vector2f(SCREEN_WIDTH - 150.f, SCREEN_HEIGHT - 30.f), 
        GameState::Lobby, HUD::RenderMode::ScreenSpace, true);
    game->GetHUD().updateBaseColor("gridToggle", sf::Color::Black);
    // PlayerManager setup
    CSteamID myID = SteamUser()->GetSteamID();
    std::string myIDStr = std::to_string(myID.ConvertToUint64());
    playerManager = std::make_unique<PlayerManager>(game, myIDStr);
    playerRenderer = std::make_unique<PlayerRenderer>(playerManager.get());

    std::string myName = SteamFriends()->GetPersonaName();
    CSteamID hostIDSteam = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
    
    RemotePlayer localPlayer;
    localPlayer.playerID = myIDStr;
    localPlayer.isHost = (myID == hostIDSteam);
    localPlayer.player = Player(sf::Vector2f(400.f, 300.f), sf::Color::Blue);
    localPlayer.nameText.setFont(game->GetFont());
    localPlayer.nameText.setString(myName);
    localPlayer.baseName = myName;
    localPlayer.cubeColor = sf::Color::Blue;
    localPlayer.nameText.setCharacterSize(16);
    localPlayer.nameText.setFillColor(sf::Color::Black);
    localPlayer.player.SetRespawnPosition(sf::Vector2f(0.f, 0.f));
    playerManager->AddOrUpdatePlayer(myIDStr, localPlayer);

    // Network setup
    if (myID == hostIDSteam) {
        hostNetwork = std::make_unique<HostNetwork>(game, playerManager.get());
        game->GetNetworkManager().SetMessageHandler(
            [this](const std::string& msg, CSteamID sender) {
                hostNetwork->ProcessMessage(msg, sender);
            }
        );
        // Explicitly broadcast host's own Connection message
        std::string hostConnectMsg = MessageHandler::FormatConnectionMessage(
            myIDStr,
            myName,
            sf::Color::Blue,
            false,  // Initial ready status
            true    // isHost
        );
        game->GetNetworkManager().BroadcastMessage(hostConnectMsg);
        std::cout << "[HOST] Sent host connection message: " << hostConnectMsg << "\n";
        hostNetwork->BroadcastFullPlayerList();  // Ensure all players, including host, are synced
    } else {
        clientNetwork = std::make_unique<ClientNetwork>(game, playerManager.get());
        game->GetNetworkManager().SetMessageHandler(
            [this](const std::string& msg, CSteamID sender) {
                clientNetwork->ProcessMessage(msg, sender);
            }
        );
        clientNetwork->SendConnectionMessage();  // Request initial player list
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
        loadingTimer += dt; // Keep this for simplicity, though could use timestamps
        if (loadingTimer >= 2.0f) {
            playerLoaded = true;
            game->GetHUD().updateText("playerLoading", "");
            if (clientNetwork && !connectionSent) {
                clientNetwork->SendConnectionMessage();
                connectionSent = true;
            }
        }
    } else {
        // Update PlayerManager (includes local player movement)
        playerManager->Update();
        if (clientNetwork) clientNetwork->Update();
        if (hostNetwork) hostNetwork->Update();
    }
    if (mouseHeld) {
        shootTimer -= dt;
        if (shootTimer <= 0.f) {
            // Get current mouse position
            sf::Vector2i mousePos = sf::Mouse::getPosition(game->GetWindow());
            AttemptShoot(mousePos.x, mousePos.y);
            shootTimer = 0.1f; // Shoot every 0.1 seconds when holding down
        }
    }
    if (SteamUser()->GetSteamID() == SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID())) {
        game->GetHUD().updateText("startGame", "Press S to Start Game (Host Only)");
    } else {
        game->GetHUD().updateText("startGame", "");
    }
    sf::Vector2f localPlayerPos = playerManager->GetLocalPlayer().player.GetPosition();
    game->GetCamera().setCenter(localPlayerPos);
}

void LobbyState::Render() {
    game->GetWindow().clear(sf::Color::White);
    
    if (showGrid) {
        grid.render(game->GetWindow(), game->GetCamera());
    }
    
    if (playerLoaded) {
        playerRenderer->Render(game->GetWindow()); // Renders all players
    }
    
    game->GetHUD().render(game->GetWindow(), game->GetWindow().getDefaultView(), game->GetCurrentState());
    game->GetWindow().display();
}

void LobbyState::ProcessEvents(const sf::Event& event) {
    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::R) {
            std::string myID = std::to_string(SteamUser()->GetSteamID().ConvertToUint64());
            bool currentReady = playerManager->GetLocalPlayer().isReady;
            bool newReady = !currentReady;
            playerManager->SetReadyStatus(myID, newReady);
            std::cout << "ready status set to " << newReady << "\n";
            std::string msg = MessageHandler::FormatReadyStatusMessage(myID, newReady);
            if (hostNetwork) {
                game->GetNetworkManager().BroadcastMessage(msg);
            } else if (clientNetwork) {
                clientNetwork->SendReadyStatus(newReady);
            }
        }
        else if (event.key.code == sf::Keyboard::G) {
            // Toggle grid visibility
            showGrid = !showGrid;
        }
        
    } else if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        mouseHeld = true;
        AttemptShoot(event.mouseButton.x, event.mouseButton.y);
    } 
    else if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
        mouseHeld = false;
    }
}
void LobbyState::ProcessEvent(const sf::Event& event) {
    ProcessEvents(event);
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

void LobbyState::AttemptShoot(int mouseX, int mouseY) {
    std::string myID = std::to_string(SteamUser()->GetSteamID().ConvertToUint64());
    
    // Don't shoot if player is dead
    if (playerManager->GetLocalPlayer().player.IsDead()) {
        return;
    }
    
    // Get mouse position in screen coordinates and convert to world coordinates
    sf::Vector2i mouseScreenPos(mouseX, mouseY);
    sf::Vector2f mouseWorldPos = game->GetWindow().mapPixelToCoords(mouseScreenPos, game->GetCamera());
    
    // Trigger shooting and get bullet parameters
    Player::BulletParams params = playerManager->GetLocalPlayer().player.Shoot(mouseWorldPos);
    
    // Only create and send bullet if shooting was successful (not on cooldown)
    if (params.direction != sf::Vector2f(0.f, 0.f)) {  // Check if direction is valid
        float bulletSpeed = 400.f;
        
        // As the local player, we always add our own bullets locally
        playerManager->AddBullet(myID, params.position, params.direction, bulletSpeed);
        
        // Send bullet message to others
        std::string msg = MessageHandler::FormatBulletMessage(myID, params.position, params.direction, bulletSpeed);
        
        if (hostNetwork) {
            game->GetNetworkManager().BroadcastMessage(msg);
        } else if (clientNetwork) {
            game->GetNetworkManager().SendMessage(clientNetwork->GetHostID(), msg);
        }
    }
}