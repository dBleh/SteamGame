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
      showLeaderboard(false),
      cursorLocked(true) {
    
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
    game->GetHUD().updateBaseColor("playerStats", sf::Color::Black);
    
    // Leaderboard initialization (initially hidden)
    game->GetHUD().addElement("leaderboard", "", 24, 
        sf::Vector2f(SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.3f), 
        GameState::Playing, HUD::RenderMode::ScreenSpace, false);
    game->GetHUD().updateBaseColor("leaderboard", sf::Color::Black);
    
    // Custom cursor setup
    // Create crosshair shapes
    float cursorSize = 8.f;  // Reduced size
    
    // Outer circle
    cursorOuterCircle.setRadius(cursorSize);
    cursorOuterCircle.setFillColor(sf::Color::Transparent);
    cursorOuterCircle.setOutlineColor(sf::Color::Black);
    cursorOuterCircle.setOutlineThickness(1.5f);
    cursorOuterCircle.setOrigin(cursorSize, cursorSize);
    
    // Center dot
    cursorCenterDot.setRadius(1.5f);
    cursorCenterDot.setFillColor(sf::Color::Black);
    cursorCenterDot.setOrigin(1.5f, 1.5f);
    
    // Horizontal line
    cursorHorizontalLine.setSize(sf::Vector2f(cursorSize*2, 1.5f));
    cursorHorizontalLine.setFillColor(sf::Color::Black);
    cursorHorizontalLine.setOrigin(cursorSize, 0.75f);
    
    // Vertical line
    cursorVerticalLine.setSize(sf::Vector2f(1.5f, cursorSize*2));
    cursorVerticalLine.setFillColor(sf::Color::Black);
    cursorVerticalLine.setOrigin(0.75f, cursorSize);
    
    // Hide system cursor and lock it to window
    game->GetWindow().setMouseCursorVisible(false);
    game->GetWindow().setMouseCursorGrabbed(true);
    
    // Calculate window center
    windowCenter = sf::Vector2i(
        static_cast<int>(game->GetWindow().getSize().x / 2),
        static_cast<int>(game->GetWindow().getSize().y / 2)
    );
    
    // Center the mouse at start
    sf::Mouse::setPosition(windowCenter, game->GetWindow());

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
    
    // Add a cursor lock toggle hint
    game->GetHUD().addElement("cursorLockHint", "Press L to toggle cursor lock", 20, 
        sf::Vector2f(SCREEN_WIDTH - 150.f, SCREEN_HEIGHT - 60.f), 
        GameState::Playing, HUD::RenderMode::ScreenSpace, true);
    game->GetHUD().updateBaseColor("cursorLockHint", sf::Color::Black);
}

PlayingState::~PlayingState() {
    // Make sure to release the cursor when leaving this state
    game->GetWindow().setMouseCursorGrabbed(false);
    game->GetWindow().setMouseCursorVisible(true);
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
                // Get current mouse position in window coordinates
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
        
        // Get mouse position in window coordinates
        sf::Vector2i mousePos = sf::Mouse::getPosition(game->GetWindow());
        
        // If cursor is locked, keep it within window bounds
        if (cursorLocked) {
            sf::Vector2u windowSize = game->GetWindow().getSize();
            bool needsRepositioning = false;
            
            if (mousePos.x < 0) {
                mousePos.x = 0;
                needsRepositioning = true;
            }
            else if (mousePos.x >= static_cast<int>(windowSize.x)) {
                mousePos.x = static_cast<int>(windowSize.x) - 1;
                needsRepositioning = true;
            }
            
            if (mousePos.y < 0) {
                mousePos.y = 0;
                needsRepositioning = true;
            }
            else if (mousePos.y >= static_cast<int>(windowSize.y)) {
                mousePos.y = static_cast<int>(windowSize.y) - 1;
                needsRepositioning = true;
            }
            
            if (needsRepositioning) {
                sf::Mouse::setPosition(mousePos, game->GetWindow());
            }
        }
        
        // Important: Get the current mouse position in screen space (for HUD rendering)
        sf::Vector2f screenCursorPos = static_cast<sf::Vector2f>(mousePos);
        
        // Update cursor position for rendering
        // Convert from screen space to the current view space
        sf::Vector2f viewCursorPos = game->GetWindow().mapPixelToCoords(mousePos, game->GetWindow().getDefaultView());
        
        cursorOuterCircle.setPosition(viewCursorPos);
        cursorCenterDot.setPosition(viewCursorPos);
        cursorHorizontalLine.setPosition(viewCursorPos);
        cursorVerticalLine.setPosition(viewCursorPos);
    }
    
    // Update camera position to follow player
    sf::Vector2f localPlayerPos = playerManager->GetLocalPlayer().player.GetPosition();
    game->GetCamera().setCenter(localPlayerPos);
}

void PlayingState::Render() {
    game->GetWindow().clear(sf::Color::White);
    
    // Set the game camera view for world rendering
    game->GetWindow().setView(game->GetCamera());
    
    if (showGrid) {
        grid.render(game->GetWindow(), game->GetCamera());
    }
    
    if (playerLoaded) {
        playerRenderer->Render(game->GetWindow()); // Renders all players
    }
    
    // Switch to default view for HUD rendering
    game->GetWindow().setView(game->GetWindow().getDefaultView());
    
    // Render HUD elements
    game->GetHUD().render(game->GetWindow(), game->GetWindow().getDefaultView(), game->GetCurrentState());
    
    // Render custom cursor (on top of everything, in default view)
    if (playerLoaded) {
        // Get current mouse position for accurate cursor rendering
        sf::Vector2i mousePos = sf::Mouse::getPosition(game->GetWindow());
        sf::Vector2f cursorPos = static_cast<sf::Vector2f>(mousePos);
        
        // Update cursor positions before drawing
        cursorOuterCircle.setPosition(cursorPos);
        cursorCenterDot.setPosition(cursorPos);
        cursorHorizontalLine.setPosition(cursorPos);
        cursorVerticalLine.setPosition(cursorPos);
        
        game->GetWindow().draw(cursorOuterCircle);
        game->GetWindow().draw(cursorHorizontalLine);
        game->GetWindow().draw(cursorVerticalLine);
        game->GetWindow().draw(cursorCenterDot);
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
        else if (event.key.code == sf::Keyboard::L) {
            // Toggle cursor lock
            cursorLocked = !cursorLocked;
            game->GetWindow().setMouseCursorGrabbed(cursorLocked);
            std::cout << "Cursor lock " << (cursorLocked ? "enabled" : "disabled") << std::endl;
        }
        else if (event.key.code == sf::Keyboard::Escape) {
            // Potential use: release cursor lock with Escape
            if (cursorLocked) {
                cursorLocked = false;
                game->GetWindow().setMouseCursorGrabbed(false);
                std::cout << "Cursor lock disabled with Escape" << std::endl;
            }
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
    else if (event.type == sf::Event::LostFocus) {
        // Release mouse if window loses focus
        if (cursorLocked) {
            game->GetWindow().setMouseCursorGrabbed(false);
        }
    }
    else if (event.type == sf::Event::GainedFocus) {
        // Re-grab mouse if window regains focus and cursor lock is enabled
        if (cursorLocked) {
            game->GetWindow().setMouseCursorGrabbed(true);
        }
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