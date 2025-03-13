#include "PlayingState.h"
#include "../Game.h"
#include "../entities/PlayerManager.h"
#include "../render/PlayerRenderer.h"
#include <Steam/steam_api.h>
#include <iostream>

PlayingState::PlayingState(Game* game)
    : State(game), 
      playerLoaded(false), 
      loadingTimer(0.f), 
      connectionSent(false),
      grid(50.f, sf::Color(220, 220, 220)), 
      showGrid(true),
      mouseHeld(false),
      shootTimer(0.f) {
    
    std::cout << "[DEBUG] PlayingState constructor start\n";
    
    // HUD initialization
    game->GetHUD().addElement("gameHeader", "Game In Progress", 32, sf::Vector2f(SCREEN_WIDTH * 0.5f, 20.f), GameState::Playing, HUD::RenderMode::ScreenSpace, true);
    game->GetHUD().updateBaseColor("gameHeader", sf::Color::White);
    game->GetHUD().addElement("playerLoading", "Loading players...", 24, sf::Vector2f(50.f, SCREEN_HEIGHT - 150.f), GameState::Playing, HUD::RenderMode::ScreenSpace, false);
    game->GetHUD().addElement("gridToggle", "Press G to toggle grid", 20, 
        sf::Vector2f(SCREEN_WIDTH - 150.f, SCREEN_HEIGHT - 30.f), 
        GameState::Playing, HUD::RenderMode::ScreenSpace, true);
    game->GetHUD().updateBaseColor("gridToggle", sf::Color::Black);

    // PlayerManager setup - Similar to LobbyState
    CSteamID myID = SteamUser()->GetSteamID();
    std::string myIDStr = std::to_string(myID.ConvertToUint64());
    playerManager = std::make_unique<PlayerManager>(game, myIDStr);
    playerRenderer = std::make_unique<PlayerRenderer>(playerManager.get());

    std::string myName = SteamFriends()->GetPersonaName();
    CSteamID hostIDSteam = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
    
    RemotePlayer localPlayer;
    localPlayer.playerID = myIDStr;
    localPlayer.isHost = (myID == hostIDSteam);
    localPlayer.player = Player(sf::Vector2f(0.f, 0.f), sf::Color::Blue); // Start at 0,0
    localPlayer.nameText.setFont(game->GetFont());
    localPlayer.nameText.setString(myName);
    localPlayer.baseName = myName;
    localPlayer.cubeColor = sf::Color::Blue;
    localPlayer.nameText.setCharacterSize(16);
    localPlayer.nameText.setFillColor(sf::Color::Black);
    localPlayer.player.SetRespawnPosition(sf::Vector2f(0.f, 0.f));
    playerManager->AddOrUpdatePlayer(myIDStr, localPlayer);

    // Network setup - Similar to LobbyState
    if (myID == hostIDSteam) {
        hostNetwork = std::make_unique<HostNetwork>(game, playerManager.get());
        game->GetNetworkManager().SetMessageHandler(
            [this](const std::string& msg, CSteamID sender) {
                hostNetwork->ProcessMessage(msg, sender);
            }
        );
        // Broadcast host's connection
        std::string hostConnectMsg = MessageHandler::FormatConnectionMessage(
            myIDStr,
            myName,
            sf::Color::Blue,
            false,
            true
        );
        game->GetNetworkManager().BroadcastMessage(hostConnectMsg);
        std::cout << "[HOST] Sent host connection message in PlayingState: " << hostConnectMsg << "\n";
        hostNetwork->BroadcastFullPlayerList();
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

void PlayingState::Update(float dt) {
    if (!playerLoaded) {
        loadingTimer += dt;
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
        
        // Handle continuous shooting if mouse button is held down
        if (mouseHeld) {
            shootTimer -= dt;
            if (shootTimer <= 0.f) {
                // Get current mouse position
                sf::Vector2i mousePos = sf::Mouse::getPosition(game->GetWindow());
                AttemptShoot(mousePos.x, mousePos.y);
                shootTimer = 0.1f; // Shoot every 0.1 seconds when holding down
            }
        }
    }
    
    sf::Vector2f localPlayerPos = playerManager->GetLocalPlayer().player.GetPosition();
    game->GetCamera().setCenter(localPlayerPos);
}

void PlayingState::Render() {
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

void PlayingState::ProcessEvents(const sf::Event& event) {
    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::G) {
            // Toggle grid visibility
            showGrid = !showGrid;
        }
    } 
    else if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        mouseHeld = true;
        AttemptShoot(event.mouseButton.x, event.mouseButton.y);
    } 
    else if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
        mouseHeld = false;
    }
}

void PlayingState::ProcessEvent(const sf::Event& event) {
    ProcessEvents(event);
}

bool PlayingState::IsFullyLoaded() {
    return (
        game->GetHUD().isFullyLoaded() &&
        game->GetWindow().isOpen() &&
        playerLoaded
    );
}

void PlayingState::AttemptShoot(int mouseX, int mouseY) {
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