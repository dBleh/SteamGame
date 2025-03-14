#include "PlayingState.h"
#include "../Game.h"
#include "../utils/Config.h"
#include "../entities/PlayerManager.h"
#include "../network/Host.h"
#include "../network/Client.h"
#include "../entities/EnemyManager.h"
#include "../render/PlayerRenderer.h"
#include "../entities/TriangleManager.h"
#include <Steam/steam_api.h>
#include <iostream>

PlayingState::PlayingState(Game* game)
    : State(game), 
      playerLoaded(false), 
      loadingTimer(0.f), 
      connectionSent(false),
      showGrid(true),
      mouseHeld(false),
      shootTimer(0.f),
      showLeaderboard(false),
      cursorLocked(true),
      showEscapeMenu(false) {
    
    std::cout << "[DEBUG] PlayingState constructor start\n";
    
    // Use fixed dimensions based on BASE_WIDTH and BASE_HEIGHT
    float centerX = BASE_WIDTH / 2.0f;
    
    // Fixed positions instead of percentages
    float topBarY = 40.0f; // Fixed pixel position
    float bottomBarY = BASE_HEIGHT - 70.0f; // Fixed position from bottom
    float lineWidth = 1000.0f; // Fixed line width
    float lineThickness = 2.0f; // Fixed thickness
    
    // Calculate line start X position to center the line
    float lineStartX = centerX - (lineWidth / 2.0f);
    
    // Create a stylish grid with modern colors that match the UI theme
    grid = Grid(50.f, sf::Color(180, 180, 180, 100)); // Slightly transparent grid lines
    grid.setMajorLineInterval(5);
    grid.setMajorLineColor(sf::Color(150, 150, 150, 180)); // Slightly darker for major lines
    grid.setOriginHighlight(true);
    grid.setOriginHighlightColor(sf::Color(100, 100, 255, 120)); // Subtle blue for origin
    grid.setOriginHighlightSize(15.0f);
    
    // ===== TOP BAR UI =====
    // Top separator line
    game->GetHUD().addGradientLine("topBarLine", 
                                  lineStartX,
                                  topBarY, 
                                  lineWidth, 
                                  lineThickness,
                                  sf::Color::Black, 
                                  GameState::Playing, 
                                  HUD::RenderMode::ScreenSpace,
                                  30);
    
    // Game header (centered with wave info)
    game->GetHUD().addElement("gameHeader", "WAVE 1", 32, 
                             sf::Vector2f(centerX - 60.0f, topBarY - 40.0f), 
                             GameState::Playing, 
                             HUD::RenderMode::ScreenSpace, false,
                             "", "topBarLine");
    game->GetHUD().updateBaseColor("gameHeader", sf::Color::Black);
    
    // Player stats (left aligned)
    game->GetHUD().addElement("playerStats", "HP: 100 | Kills: 0 | Money: 0", 18, 
                             sf::Vector2f(30.0f, topBarY + 15.0f), 
                             GameState::Playing, 
                             HUD::RenderMode::ScreenSpace, false);
    game->GetHUD().updateBaseColor("playerStats", sf::Color::Black);
    
    // Wave info (right aligned)
    game->GetHUD().addElement("waveInfo", "Enemies: 0", 18, 
                             sf::Vector2f(BASE_WIDTH - 150.0f, topBarY + 15.0f), 
                             GameState::Playing, 
                             HUD::RenderMode::ScreenSpace, false);
    game->GetHUD().updateBaseColor("waveInfo", sf::Color::Black);
    
    // Loading message (centered, temporary)
    game->GetHUD().addElement("playerLoading", "Loading game...", 24, 
                             sf::Vector2f(centerX - 80.0f, BASE_HEIGHT / 2.0f), 
                             GameState::Playing, 
                             HUD::RenderMode::ScreenSpace, false);
    game->GetHUD().updateBaseColor("playerLoading", sf::Color::Black);
    
    // ===== BOTTOM BAR UI =====
    // Bottom separator line
    game->GetHUD().addGradientLine("bottomBarLine", 
                                  lineStartX,
                                  bottomBarY, 
                                  lineWidth, 
                                  lineThickness,
                                  sf::Color::Black, 
                                  GameState::Playing, 
                                  HUD::RenderMode::ScreenSpace,
                                  30);
    
    // Control hints
    float controlsY = bottomBarY + 20.0f;
    float spacing = 220.0f;
    
    // Tab for leaderboard
    game->GetHUD().addElement("tabHint", "TAB - Show Leaderboard", 16, 
                             sf::Vector2f(30.0f, controlsY), 
                             GameState::Playing, 
                             HUD::RenderMode::ScreenSpace, false,
                             "bottomBarLine", "");
    game->GetHUD().updateBaseColor("tabHint", sf::Color::Black);
    
    // Grid toggle
    game->GetHUD().addElement("gridToggle", "G - Toggle Grid", 16, 
                             sf::Vector2f(30.0f + spacing, controlsY), 
                             GameState::Playing, 
                             HUD::RenderMode::ScreenSpace, true,
                             "bottomBarLine", "");
    game->GetHUD().updateBaseColor("gridToggle", sf::Color::Black);
    
    // Cursor lock toggle
    game->GetHUD().addElement("cursorLockHint", "L - Toggle Cursor Lock", 16, 
                             sf::Vector2f(30.0f + spacing * 2, controlsY), 
                             GameState::Playing, 
                             HUD::RenderMode::ScreenSpace, true,
                             "bottomBarLine", "");
    game->GetHUD().updateBaseColor("cursorLockHint", sf::Color::Black);
    
    // Escape menu hint
    game->GetHUD().addElement("escHint", "ESC - Menu", 16, 
                             sf::Vector2f(30.0f + spacing * 3, controlsY), 
                             GameState::Playing, 
                             HUD::RenderMode::ScreenSpace, false,
                             "bottomBarLine", "");
    game->GetHUD().updateBaseColor("escHint", sf::Color::Black);
    
    // ===== LEADERBOARD (initially hidden) =====
    game->GetHUD().addElement("leaderboard", "", 20, 
                             sf::Vector2f(centerX - 180.0f, BASE_HEIGHT * 0.25f), 
                             GameState::Playing, 
                             HUD::RenderMode::ScreenSpace, false);
    game->GetHUD().updateBaseColor("leaderboard", sf::Color::White);
    
    // ===== ESCAPE MENU SETUP =====
    // Background panel with modern styling
    menuBackground.setSize(sf::Vector2f(400.f, 300.f));
    menuBackground.setFillColor(sf::Color(40, 40, 40, 230)); // Dark, semi-transparent
    menuBackground.setOutlineColor(sf::Color(100, 100, 255, 150)); // Subtle blue glow
    menuBackground.setOutlineThickness(2.f);

    // Menu title
    menuTitle.setFont(game->GetFont());
    menuTitle.setString("Game Paused");
    menuTitle.setCharacterSize(28);
    menuTitle.setFillColor(sf::Color(220, 220, 220));
    menuTitle.setStyle(sf::Text::Bold);

    // Return button with modern styling
    returnButton.setSize(sf::Vector2f(220.f, 50.f));
    returnButton.setFillColor(sf::Color(60, 60, 60, 230));
    returnButton.setOutlineColor(sf::Color(120, 120, 200));
    returnButton.setOutlineThickness(1.5f);

    returnButtonText.setFont(game->GetFont());
    returnButtonText.setString("Return to Main Menu");
    returnButtonText.setCharacterSize(20);
    returnButtonText.setFillColor(sf::Color(220, 220, 220));

    // Continue button
    continueButton.setSize(sf::Vector2f(220.f, 50.f));
    continueButton.setFillColor(sf::Color(60, 60, 60, 230));
    continueButton.setOutlineColor(sf::Color(120, 200, 120));
    continueButton.setOutlineThickness(1.5f);

    continueButtonText.setFont(game->GetFont());
    continueButtonText.setString("Continue Playing");
    continueButtonText.setCharacterSize(20);
    continueButtonText.setFillColor(sf::Color(220, 220, 220));
    
    
    // Hide system cursor and lock it to window
    game->GetWindow().setMouseCursorGrabbed(true);
    
    // Calculate window center
    windowCenter = sf::Vector2i(
        static_cast<int>(game->GetWindow().getSize().x / 2),
        static_cast<int>(game->GetWindow().getSize().y / 2)
    );
    
    // Center the mouse at start
    sf::Mouse::setPosition(windowCenter, game->GetWindow());

    // ===== PLAYER MANAGER SETUP =====
    // Setup Player Manager first
    CSteamID myID = SteamUser()->GetSteamID();
    std::string myIDStr = std::to_string(myID.ConvertToUint64());
    playerManager = std::make_unique<PlayerManager>(game, myIDStr);
    
    std::string myName = SteamFriends()->GetPersonaName();
    CSteamID hostIDSteam = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
    
    RemotePlayer localPlayer;
    localPlayer.playerID = myIDStr;
    localPlayer.isHost = (myID == hostIDSteam);
    localPlayer.player = Player(sf::Vector2f(0.f, 0.f), sf::Color::Blue);
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
    
    // Create PlayerRenderer after PlayerManager
    playerRenderer = std::make_unique<PlayerRenderer>(playerManager.get());

    // ===== ENEMY MANAGER SETUP =====
    // Create the Enemy Managers after Player Manager
    enemyManager = std::make_unique<EnemyManager>(game, playerManager.get());
    triangleEnemyManager = std::make_unique<TriangleEnemyManager>(game, playerManager.get());

    // ===== NETWORK SETUP =====
    // Create networking components last, so they can use the other managers
    if (myID == hostIDSteam) {
        hostNetwork = std::make_unique<HostNetwork>(game, playerManager.get());
        game->GetNetworkManager().SetMessageHandler(
            [this](const std::string& msg, CSteamID sender) {
                if (hostNetwork) {
                    hostNetwork->ProcessMessage(msg, sender);
                }
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
        
        if (hostNetwork) {
            hostNetwork->BroadcastFullPlayerList();
        }
    } else {
        clientNetwork = std::make_unique<ClientNetwork>(game, playerManager.get());
        game->GetNetworkManager().SetMessageHandler(
            [this](const std::string& msg, CSteamID sender) {
                if (clientNetwork) {
                    clientNetwork->ProcessMessage(msg, sender);
                }
            }
        );
        
        if (clientNetwork) {
            clientNetwork->SendConnectionMessage();
        }
    }
    
    std::cout << "[DEBUG] PlayingState constructor completed\n";
}

PlayingState::~PlayingState() {
    // Make sure to release the cursor when leaving this state
    game->GetWindow().setMouseCursorGrabbed(false);
    
    
  
    
    // Clean up components in the correct order to avoid any crashes
    hostNetwork.reset();
    clientNetwork.reset();
    triangleEnemyManager.reset();
    enemyManager.reset();
    playerRenderer.reset();
    playerManager.reset();
    
    std::cout << "[DEBUG] PlayingState destructor completed\n";
}

void PlayingState::UpdateWaveInfo() {
    if (!enemyManager) return;
    
    int currentWave = enemyManager->GetCurrentWave();
    int remainingEnemies = enemyManager->GetRemainingEnemies();
    float waveTimer = enemyManager->GetWaveTimer();
    
    // Update the wave header
    std::string headerText = "WAVE " + std::to_string(currentWave);
    game->GetHUD().updateText("gameHeader", headerText);
    
    // Update the wave info
    std::string waveText;
    if (enemyManager->IsWaveComplete()) {
        waveText = "Next wave in: " + std::to_string(static_cast<int>(waveTimer)) + "s";
        game->GetHUD().updateBaseColor("waveInfo", sf::Color(76, 175, 80)); // Green for wave complete
    } else {
        waveText = "Enemies: " + std::to_string(remainingEnemies);
        game->GetHUD().updateBaseColor("waveInfo", sf::Color::Black);
    }
    
    game->GetHUD().updateText("waveInfo", waveText);
}

void PlayingState::StartFirstWave() {
    // Only the host should start the wave
    if (!playerLoaded) return;
    
    CSteamID myID = SteamUser()->GetSteamID();
    CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
    
    if (myID == hostID) {
        std::cout << "[HOST] Starting first wave" << std::endl;
        
        // Start the regular enemy wave
        if (enemyManager) {
            try {
                enemyManager->StartNextWave();
                
                // Broadcast the wave start to all clients
                std::string waveMsg = MessageHandler::FormatWaveStartMessage(
                    enemyManager->GetCurrentWave());
                game->GetNetworkManager().BroadcastMessage(waveMsg);
                
                std::cout << "[HOST] Regular enemy wave started" << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "[ERROR] Failed to start enemy wave: " << e.what() << std::endl;
            }
        }
        
        // Also spawn triangle enemies
        if (triangleEnemyManager) {
            try {
                // Spawn 100 triangle enemies
                triangleEnemyManager->SpawnWave(100); // Reduced from 1000 to prevent overload
                
                // Synchronize with clients
                triangleEnemyManager->SyncFullEnemyList();
                
                std::cout << "[HOST] Triangle enemy wave started" << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "[ERROR] Failed to start triangle enemies: " << e.what() << std::endl;
            }
        }
    }
}

bool PlayingState::isPointInRect(const sf::Vector2f& point, const sf::FloatRect& rect) {
    return point.x >= rect.left && point.x <= rect.left + rect.width &&
           point.y >= rect.top && point.y <= rect.top + rect.height;
}

void PlayingState::updateButtonHoverState(sf::RectangleShape& button, const sf::Vector2f& mousePos, bool& isHovered) {
    bool hovering = isPointInRect(mousePos, button.getGlobalBounds());
    
    if (hovering != isHovered) {
        isHovered = hovering;
        if (isHovered) {
            button.setFillColor(sf::Color(80, 80, 100, 230)); // Lighter when hovered
        } else {
            button.setFillColor(sf::Color(60, 60, 60, 230)); // Default color
        }
    }
}

void PlayingState::Update(float dt) {
    // Update HUD animations
    game->GetHUD().update(game->GetWindow(), GameState::Playing, dt);
    
    if (!playerLoaded) {
        loadingTimer += dt;
        if (loadingTimer >= 2.0f) {
            playerLoaded = true;
            game->GetHUD().updateText("playerLoading", "");
            if (clientNetwork && !connectionSent) {
                clientNetwork->SendConnectionMessage();
                connectionSent = true;
            }
            
            // Start the first wave once players are loaded
            StartFirstWave();
        }
    } else {
        // Update PlayerManager with the game instance (for InputManager access)
        playerManager->Update(game);
        if (clientNetwork) clientNetwork->Update();
        if (hostNetwork) hostNetwork->Update();
        
        // Update EnemyManager
        if (enemyManager) {
            enemyManager->Update(dt);
            
            // Check bullet collisions with enemies
            enemyManager->CheckBulletCollisions(playerManager->GetAllBullets());
            
            // Update wave info in HUD
            UpdateWaveInfo();
        }

        if (triangleEnemyManager) {
            triangleEnemyManager->Update(dt);
            
            // Check bullet collisions with triangle enemies
            triangleEnemyManager->CheckBulletCollisions(playerManager->GetAllBullets());
        }
        
        // Handle continuous shooting if mouse button is held down
        if (!showEscapeMenu && playerLoaded) {
            // Check if the shoot action is active through InputHandler
            if (game->GetInputHandler()->IsActionActive(InputAction::Shoot)) {
                shootTimer -= dt;
                if (shootTimer <= 0.f) {
                    // Get current mouse position and attempt to shoot
                    sf::Vector2i mousePos = sf::Mouse::getPosition(game->GetWindow());
                    AttemptShoot(mousePos.x, mousePos.y);
                    shootTimer = 0.1f; // Shoot every 0.1 seconds when holding down
                }
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
        
        // Handle cursor lock if enabled
        if (cursorLocked && !showEscapeMenu) {
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
        
        // Update button colors based on hover state
        sf::Vector2f mouseUIPos = game->WindowToUICoordinates(mousePos);
        
        if (mouseUIPos.x >= 0 && mouseUIPos.y >= 0) {
            const auto& elements = game->GetHUD().getElements();
            
            for (const auto& [id, element] : elements) {
                if (element.hoverable && element.visibleState == GameState::Playing) {
                    // Create a text copy to check bounds
                    sf::Text textCopy = element.text;
                    textCopy.setPosition(element.pos);
                    sf::FloatRect bounds = textCopy.getGlobalBounds();
                    
                    if (bounds.contains(mouseUIPos)) {
                        // Hovering over element
                        game->GetHUD().updateBaseColor(id, sf::Color(100, 100, 100)); // Darker when hovered
                    } else {
                        // Not hovering - set appropriate color based on state
                        if (id == "gridToggle") {
                            game->GetHUD().updateBaseColor(id, showGrid ? sf::Color::Black : sf::Color(150, 150, 150));
                        } else if (id == "cursorLockHint") {
                            game->GetHUD().updateBaseColor(id, cursorLocked ? sf::Color::Black : sf::Color(150, 150, 150));
                        } else {
                            game->GetHUD().updateBaseColor(id, sf::Color::Black);
                        }
                    }
                }
            }
        }
        
        // Update escape menu button hover states
        if (showEscapeMenu) {
            static bool continueHovered = false;
            static bool returnHovered = false;
            
            float centerX = BASE_WIDTH / 2.0f;
            float centerY = BASE_HEIGHT / 2.0f;
            
            // Update button positions
            continueButton.setPosition(
                centerX - continueButton.getSize().x / 2.0f,
                centerY - 40.0f
            );
            
            returnButton.setPosition(
                centerX - returnButton.getSize().x / 2.0f,
                centerY + 30.0f
            );
            
            updateButtonHoverState(continueButton, mouseUIPos, continueHovered);
            updateButtonHoverState(returnButton, mouseUIPos, returnHovered);
        }
    }
    
    // Center camera on local player
    sf::Vector2f localPlayerPos = playerManager->GetLocalPlayer().player.GetPosition();
    game->GetCamera().setCenter(localPlayerPos);
    
    // Update the custom cursor position
    sf::Vector2i mousePos = sf::Mouse::getPosition(game->GetWindow());
    sf::Vector2f mousePosView = game->GetWindow().mapPixelToCoords(mousePos, game->GetUIView());
    
   
}

void PlayingState::Render() {
    game->GetWindow().clear(MAIN_BACKGROUND_COLOR);
    
    try {
        // Set the game camera view for world rendering
        game->GetWindow().setView(game->GetCamera());
        
        if (showGrid) {
            grid.render(game->GetWindow(), game->GetCamera());
        }
        
        if (playerLoaded) {
            // Render all players
            if (playerRenderer) {
                playerRenderer->Render(game->GetWindow());
            }
            
            // Render enemies
            if (enemyManager) {
                enemyManager->Render(game->GetWindow());
            }
            if (triangleEnemyManager) {
                triangleEnemyManager->Render(game->GetWindow());
            }
        }
        
        // Switch to UI view for HUD rendering
        game->GetWindow().setView(game->GetUIView());
        
        // Render HUD elements using the UI view
        game->GetHUD().render(game->GetWindow(), game->GetUIView(), GameState::Playing);
        
        // Render escape menu if active
        if (showEscapeMenu) {
            // Draw a semi-transparent overlay for the entire screen
            sf::RectangleShape overlay;
            overlay.setSize(sf::Vector2f(BASE_WIDTH, BASE_HEIGHT));
            overlay.setFillColor(sf::Color(0, 0, 0, 150));
            game->GetWindow().draw(overlay);
            
            // Position menu elements using fixed coordinates based on BASE_WIDTH and BASE_HEIGHT
            float centerX = BASE_WIDTH / 2.0f;
            float centerY = BASE_HEIGHT / 2.0f;
            
            // Update menu background position
            menuBackground.setPosition(
                centerX - menuBackground.getSize().x / 2.0f,
                centerY - menuBackground.getSize().y / 2.0f
            );
            
            // Position the title at the top of the menu
            sf::FloatRect titleBounds = menuTitle.getLocalBounds();
            menuTitle.setOrigin(
                titleBounds.left + titleBounds.width / 2.0f,
                titleBounds.top + titleBounds.height / 2.0f
            );
            menuTitle.setPosition(
                centerX,
                centerY - menuBackground.getSize().y / 2.0f + 40.0f
            );
            
            // Position the continue button
            continueButton.setPosition(
                centerX - continueButton.getSize().x / 2.0f,
                centerY - 40.0f
            );
            
            // Position the continue button text
            sf::FloatRect continueBounds = continueButtonText.getLocalBounds();
            continueButtonText.setOrigin(
                continueBounds.left + continueBounds.width / 2.0f,
                continueBounds.top + continueBounds.height / 2.0f
            );
            continueButtonText.setPosition(
                centerX,
                centerY - 40.0f + continueButton.getSize().y / 2.0f - 5.0f
            );
            
            // Position the return button below the continue button
            returnButton.setPosition(
                centerX - returnButton.getSize().x / 2.0f,
                centerY + 30.0f
            );
            
            // Position the return button text
            sf::FloatRect returnBounds = returnButtonText.getLocalBounds();
            returnButtonText.setOrigin(
                returnBounds.left + returnBounds.width / 2.0f,
                returnBounds.top + returnBounds.height / 2.0f
            );
            returnButtonText.setPosition(
                centerX,
                centerY + 30.0f + returnButton.getSize().y / 2.0f - 5.0f
            );
            
            // Draw the menu components
            game->GetWindow().draw(menuBackground);
            game->GetWindow().draw(menuTitle);
            game->GetWindow().draw(continueButton);
            game->GetWindow().draw(continueButtonText);
            game->GetWindow().draw(returnButton);
            game->GetWindow().draw(returnButtonText);
        }
        
       
        
        
        
        game->GetWindow().display();
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Exception in PlayingState::Render: " << e.what() << std::endl;
    }
}

void PlayingState::ProcessEvents(const sf::Event& event) {
    if (event.type == sf::Event::KeyPressed) {
        // Use InputManager for keybindings
        if (game->GetInputManager().IsActionTriggered(GameAction::ToggleGrid, event)) {
            // Toggle grid visibility
            showGrid = !showGrid;
        }
        else if (game->GetInputManager().IsActionTriggered(GameAction::ShowLeaderboard, event)) {
            // Toggle leaderboard visibility
            showLeaderboard = true;
            UpdateLeaderboard();
        }
        else if (game->GetInputManager().IsActionTriggered(GameAction::ToggleCursorLock, event)) {
            // Toggle cursor lock
            cursorLocked = !cursorLocked;
            game->GetWindow().setMouseCursorGrabbed(cursorLocked);
        }
        else if (game->GetInputManager().IsActionTriggered(GameAction::OpenMenu, event)) {
            // Toggle escape menu
            showEscapeMenu = !showEscapeMenu;
            
            if (showEscapeMenu) {
                // Show menu, release cursor, and make system cursor visible
                cursorLocked = false;
                game->GetWindow().setMouseCursorGrabbed(false);
            } else {
                // Hide menu, restore cursor lock, and hide system cursor
                cursorLocked = true;
                game->GetWindow().setMouseCursorGrabbed(true);
            }
        }
    } 
    else if (event.type == sf::Event::KeyReleased) {
        if (event.key.code == game->GetInputManager().GetKeyBinding(GameAction::ShowLeaderboard)) {
            // Hide leaderboard when Tab is released
            showLeaderboard = false;
            game->GetHUD().updateText("leaderboard", "");
        }
    }
    else if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        // Get mouse position in window coordinates
        sf::Vector2i mouseWindowPos(event.mouseButton.x, event.mouseButton.y);
        
        // Convert to UI coordinates for UI interaction
        sf::Vector2f mouseUIPos = game->WindowToUICoordinates(mouseWindowPos);
        
        // Only process if within UI viewport
        if (mouseUIPos.x >= 0 && mouseUIPos.y >= 0) {
            if (showEscapeMenu) {
                // Position menu elements using fixed coordinates based on BASE_WIDTH and BASE_HEIGHT
                float centerX = BASE_WIDTH / 2.0f;
                float centerY = BASE_HEIGHT / 2.0f;
                
                // Update button positions for checking bounds
                continueButton.setPosition(
                    centerX - continueButton.getSize().x / 2.0f,
                    centerY - 40.0f
                );
                
                returnButton.setPosition(
                    centerX - returnButton.getSize().x / 2.0f,
                    centerY + 30.0f
                );
                
                // Check if continue button was clicked
                sf::FloatRect continueBounds = continueButton.getGlobalBounds();
                if (continueBounds.contains(mouseUIPos)) {
                    // Continue button clicked - close menu and resume game
                    showEscapeMenu = false;
                    cursorLocked = true;
                    game->GetWindow().setMouseCursorGrabbed(true);
                    return;
                }
                
                // Check if return button was clicked
                sf::FloatRect returnBounds = returnButton.getGlobalBounds();
                if (returnBounds.contains(mouseUIPos)) {
                    // Return to main menu
                    game->SetCurrentState(GameState::MainMenu);
                    return;
                }
            } else {
                // Check for UI element clicks
                bool clickedUI = false;
                
                const auto& elements = game->GetHUD().getElements();
                for (const auto& [id, element] : elements) {
                    if (element.hoverable && element.visibleState == GameState::Playing) {
                        sf::Text textCopy = element.text;
                        textCopy.setPosition(element.pos);
                        sf::FloatRect bounds = textCopy.getGlobalBounds();
                        
                        if (bounds.contains(mouseUIPos)) {
                            // UI element was clicked
                            clickedUI = true;
                            
                            if (id == "gridToggle") {
                                showGrid = !showGrid;
                                break;
                            }
                            else if (id == "cursorLockHint") {
                                cursorLocked = !cursorLocked;
                                game->GetWindow().setMouseCursorGrabbed(cursorLocked);
                                break;
                            }
                            // Handle other UI elements...
                        }
                    }
                }
                
                // If no UI was clicked, check if this is the shoot action
                if (!clickedUI) {
                    // Only treat left mouse as shooting if no keyboard key is bound
                    bool isLeftMouseShoot = game->GetInputHandler()->GetKeyForAction(InputAction::Shoot) == sf::Keyboard::Unknown;
                    
                    if (isLeftMouseShoot) {
                        mouseHeld = true;
                        AttemptShoot(mouseWindowPos.x, mouseWindowPos.y);
                    }
                }
            }
        }
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
    try {
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
        if (params.success) {
            float bulletSpeed = 400.f;
            
            // As the local player, we always add our own bullets locally
            playerManager->AddBullet(myID, params.position, params.direction, bulletSpeed);
            
            // Send bullet message to others
            std::string msg = MessageHandler::FormatBulletMessage(myID, params.position, params.direction, bulletSpeed);
            
            if (hostNetwork) {
                // If we're the host, broadcast to all clients
                game->GetNetworkManager().BroadcastMessage(msg);
            } else if (clientNetwork) {
                // If we're a client, send to the host
                CSteamID hostID = clientNetwork->GetHostID();
                game->GetNetworkManager().SendMessage(hostID, msg);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Exception in PlayingState::AttemptShoot: " << e.what() << std::endl;
    }
}

void PlayingState::UpdatePlayerStats() {
    try {
        const RemotePlayer& localPlayer = playerManager->GetLocalPlayer();
        int health = localPlayer.player.GetHealth();
        int kills = localPlayer.kills;
        int money = localPlayer.money;
        
        // Format stats with modern UI style
        std::string statsText = "HP: " + std::to_string(health) + 
                               " | Kills: " + std::to_string(kills) + 
                               " | Money: " + std::to_string(money);
        
        // Change color based on health status
        if (health < 30) {
            game->GetHUD().updateBaseColor("playerStats", sf::Color(255, 0, 0)); // Red for low health
        } else if (health < 70) {
            game->GetHUD().updateBaseColor("playerStats", sf::Color(255, 165, 0)); // Orange for medium health
        } else {
            game->GetHUD().updateBaseColor("playerStats", sf::Color::Black); // Normal color for good health
        }
        
        game->GetHUD().updateText("playerStats", statsText);
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Exception in PlayingState::UpdatePlayerStats: " << e.what() << std::endl;
    }
}

void PlayingState::UpdateLeaderboard() {
    try {
        std::string leaderboardText = "LEADERBOARD\n\n";
        
        // Get all players sorted by kills
        auto& playersMap = playerManager->GetPlayers();
        std::vector<std::pair<std::string, RemotePlayer>> players(playersMap.begin(), playersMap.end());
        
        // Sort players by kills (descending)
        std::sort(players.begin(), players.end(), [](const auto& a, const auto& b) {
            return a.second.kills > b.second.kills;
        });
        
        // Format the leaderboard text with modern styling
        for (size_t i = 0; i < players.size(); i++) {
            const auto& [id, player] = players[i];
            
            // Add rank number and highlight the top player
            if (i == 0) {
                leaderboardText += "🏆 ";
            } else {
                leaderboardText += std::to_string(i + 1) + ". ";
            }
            
            // Add player info
            leaderboardText += player.baseName + 
                              (player.isHost ? " (Host)" : "") + 
                              "\nKills: " + std::to_string(player.kills) + 
                              " | HP: " + std::to_string(player.player.GetHealth()) + 
                              " | Money: " + std::to_string(player.money) + "\n\n";
        }
        
        if (players.empty()) {
            leaderboardText += "No players found.";
        }
        
        // Create a semi-transparent background for the leaderboard
        // Use fixed position based on BASE_WIDTH and BASE_HEIGHT
        float centerX = BASE_WIDTH / 2.0f;
        float centerY = BASE_HEIGHT / 2.0f;
        
        // Use UI view for drawing the leaderboard background
        sf::View oldView = game->GetWindow().getView();
        game->GetWindow().setView(game->GetUIView());
        
        // Create a glass-like background
        sf::RectangleShape leaderboardBg;
        leaderboardBg.setSize(sf::Vector2f(500.f, 400.f));
        leaderboardBg.setFillColor(sf::Color(30, 30, 50, 220)); // Dark blue semi-transparent
        leaderboardBg.setOutlineColor(sf::Color(100, 100, 200, 150)); // Light blue outline
        leaderboardBg.setOutlineThickness(2.f);
        leaderboardBg.setPosition(
            centerX - 250.f,  // Centered horizontally (500/2 = 250)
            centerY - 200.f   // Centered vertically (400/2 = 200)
        );
        
        // Add a header bar
        sf::RectangleShape headerBar;
        headerBar.setSize(sf::Vector2f(500.f, 50.f));
        headerBar.setFillColor(sf::Color(50, 50, 80, 230)); // Slightly lighter than background
        headerBar.setPosition(
            centerX - 250.f,
            centerY - 200.f
        );
        
        // Create header text
        sf::Text headerText;
        headerText.setFont(game->GetFont());
        headerText.setString("LEADERBOARD");
        headerText.setCharacterSize(24);
        headerText.setFillColor(sf::Color(220, 220, 255)); // Light blue-white
        headerText.setStyle(sf::Text::Bold);
        
        // Center the header text on the header bar
        sf::FloatRect headerBounds = headerText.getLocalBounds();
        headerText.setOrigin(
            headerBounds.left + headerBounds.width / 2.0f,
            headerBounds.top + headerBounds.height / 2.0f
        );
        headerText.setPosition(
            centerX,
            centerY - 200.f + 25.f // Centered on the header bar
        );
        
        // Display the background and header
        game->GetWindow().draw(leaderboardBg);
        game->GetWindow().draw(headerBar);
        game->GetWindow().draw(headerText);
        
        // Update the content area
        game->GetHUD().updateText("leaderboard", leaderboardText);
        
        // Position the leaderboard text below the header
        game->GetHUD().updateElementPosition("leaderboard", sf::Vector2f(centerX - 220.f, centerY - 130.f));
        
        // Restore previous view
        game->GetWindow().setView(oldView);
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Exception in PlayingState::UpdateLeaderboard: " << e.what() << std::endl;
    }
}