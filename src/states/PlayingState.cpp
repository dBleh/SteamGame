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
      shootTimer(0.f),
      showLeaderboard(false) {
    
    std::cout << "[DEBUG] PlayingState constructor start\n";
    
    // HUD initialization
    game->GetHUD().addElement("gameHeader", "Game In Progress", 32, sf::Vector2f(SCREEN_WIDTH * 0.5f, 20.f), GameState::Playing, HUD::RenderMode::ScreenSpace, true);
    game->GetHUD().updateBaseColor("gameHeader", sf::Color::White);
    game->GetHUD().addElement("playerLoading", "Loading players...", 24, sf::Vector2f(50.f, SCREEN_HEIGHT - 150.f), GameState::Playing, HUD::RenderMode::ScreenSpace, false);
    game->GetHUD().addElement("gridToggle", "Press G to toggle grid", 20, 
        sf::Vector2f(SCREEN_WIDTH - 150.f, SCREEN_HEIGHT - 30.f), 
        GameState::Playing, HUD::RenderMode::ScreenSpace, true);
    game->GetHUD().updateBaseColor("gridToggle", sf::Color::Black);
    
    // Stats HUD initialization
    game->GetHUD().addElement("playerStats", "HP: 100 | Kills: 0 | Money: 0", 18, 
        sf::Vector2f(10.f, 10.f), 
        GameState::Playing, HUD::RenderMode::ScreenSpace, false);
    game->GetHUD().updateBaseColor("playerStats", sf::Color::White);
    
    // Leaderboard initialization (initially hidden)
    game->GetHUD().addElement("leaderboard", "Leaderboard", 24, 
        sf::Vector2f(SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.3f), 
        GameState::Playing, HUD::RenderMode::ScreenSpace, false);
    game->GetHUD().updateBaseColor("leaderboard", sf::Color::White);
    
    // Custom cursor setup
    // Create crosshair shapes
    float cursorSize = 16.f;
    
    // Outer circle
    cursorOuterCircle.setRadius(cursorSize);
    cursorOuterCircle.setFillColor(sf::Color::Transparent);
    cursorOuterCircle.setOutlineColor(sf::Color::Black);
    cursorOuterCircle.setOutlineThickness(2.f);
    cursorOuterCircle.setOrigin(cursorSize, cursorSize);
    
    // Center dot
    cursorCenterDot.setRadius(2.f);
    cursorCenterDot.setFillColor(sf::Color::Black);
    cursorCenterDot.setOrigin(2.f, 2.f);
    
    // Horizontal line
    cursorHorizontalLine.setSize(sf::Vector2f(cursorSize, 2.f));
    cursorHorizontalLine.setFillColor(sf::Color::Black);
    cursorHorizontalLine.setOrigin(cursorSize/2.f, 1.f);
    
    // Vertical line
    cursorVerticalLine.setSize(sf::Vector2f(2.f, cursorSize));
    cursorVerticalLine.setFillColor(sf::Color::Black);
    cursorVerticalLine.setOrigin(1.f, cursorSize/2.f);
    
    // Hide system cursor
    game->GetWindow().setMouseCursorVisible(false);

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
    localPlayer.kills = 0;
    localPlayer.money = 0;
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
        
        // Update statistics display
        UpdatePlayerStats();
        
        // Update leaderboard if visible
        if (showLeaderboard) {
            UpdateLeaderboard();
        }
        
        // Update cursor position
        sf::Vector2i mousePos = sf::Mouse::getPosition(game->GetWindow());
        sf::Vector2f cursorPos = static_cast<sf::Vector2f>(mousePos);
        cursorOuterCircle.setPosition(cursorPos);
        cursorCenterDot.setPosition(cursorPos);
        cursorHorizontalLine.setPosition(cursorPos);
        cursorVerticalLine.setPosition(cursorPos);
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
    
    // Render custom cursor
    if (playerLoaded) {
        game->GetWindow().draw(cursorOuterCircle);
        game->GetWindow().draw(cursorCenterDot);
        game->GetWindow().draw(cursorHorizontalLine);
        game->GetWindow().draw(cursorVerticalLine);
    }
    
    game->GetWindow().display();
}

void PlayingState::ProcessEvents(const sf::Event& event) {
    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::G) {
            // Toggle grid visibility
            showGrid = !showGrid;
        }
        else if (event.key.code == sf::Keyboard::Tab) {
            // Toggle leaderboard visibility
            showLeaderboard = true;
            UpdateLeaderboard();
        }
    } 
    else if (event.type == sf::Event::KeyReleased) {
        if (event.key.code == sf::Keyboard::Tab) {
            // Hide leaderboard when Tab is released
            showLeaderboard = false;
            game->GetHUD().updateText("leaderboard", "");
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

void PlayingState::UpdatePlayerStats() {
    const RemotePlayer& localPlayer = playerManager->GetLocalPlayer();
    int health = localPlayer.player.GetHealth();
    int kills = localPlayer.kills;
    int money = localPlayer.money;
    
    std::string statsText = "HP: " + std::to_string(health) + 
                           " | Kills: " + std::to_string(kills) + 
                           " | Money: " + std::to_string(money);
    
    game->GetHUD().updateText("playerStats", statsText);
}

void PlayingState::UpdateLeaderboard() {
    std::string leaderboardText = "LEADERBOARD\n\n";
    
    // Get all players sorted by kills
    auto& playersMap = playerManager->GetPlayers();
    std::vector<std::pair<std::string, RemotePlayer>> players(playersMap.begin(), playersMap.end());
    
    // Sort players by kills (descending)
    std::sort(players.begin(), players.end(), [](const auto& a, const auto& b) {
        return a.second.kills > b.second.kills;
    });
    
    // Format the leaderboard text
    for (const auto& [id, player] : players) {
        leaderboardText += player.baseName + ": " + 
                          std::to_string(player.kills) + " kills | " +
                          std::to_string(player.player.GetHealth()) + " HP | " +
                          std::to_string(player.money) + " money\n";
    }
    
    // Create a semi-transparent background
    sf::RectangleShape background;
    background.setSize(sf::Vector2f(400.f, 300.f));
    background.setFillColor(sf::Color(0, 0, 0, 180)); // Semi-transparent black
    background.setPosition(SCREEN_WIDTH * 0.5f - 200.f, SCREEN_HEIGHT * 0.3f - 40.f);
    
    // Update the leaderboard text
    game->GetHUD().updateText("leaderboard", leaderboardText);
}