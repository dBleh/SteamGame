#include "PlayingState.h"
#include "../core/Game.h"
#include "../utils/config/Config.h"
#include "../entities/PlayerManager.h"
#include "../entities/enemies/EnemyManager.h"
#include "../entities/enemies/Enemy.h"
#include "../network/Host.h"
#include "../network/Client.h"
#include "shop/Shop.h"
#include "../render/PlayerRenderer.h"
#include "../network/messages/MessageHandler.h"
#include <steam/steam_api.h>
#include <iostream>
Shop* PlayingState::shopInstance = nullptr;
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
      showEscapeMenu(false),
      waveTimer(0.0f),
      showShop(false),
      waitingForNextWave(false) {
    
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
                             sf::Vector2f(BASE_WIDTH - 280.0f, topBarY + 15.0f), 
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
    // Death timer text (initially hidden)
    game->GetHUD().addElement("deathTimer", "", 36, 
        sf::Vector2f(centerX - 100.0f, BASE_HEIGHT / 2.0f - 50.0f), 
        GameState::Playing, 
        HUD::RenderMode::ScreenSpace, false);
    game->GetHUD().updateBaseColor("deathTimer", sf::Color::Red);
    isDeathTimerVisible = false;
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
    // Shop hint
    game->GetHUD().addElement("shopHint", "B - Open Shop", 16, 
        sf::Vector2f(30.0f + spacing * 4, controlsY), 
        GameState::Playing, 
        HUD::RenderMode::ScreenSpace, true,
        "bottomBarLine", "");
    game->GetHUD().updateBaseColor("shopHint", sf::Color::Black);
    
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
    localPlayer.kills = 0;
    localPlayer.money = 0;
    playerManager->AddOrUpdatePlayer(myIDStr, localPlayer);
    
    // Create PlayerRenderer after PlayerManager
    playerRenderer = std::make_unique<PlayerRenderer>(playerManager.get());

    // Initialize enemy manager
    enemyManager = std::make_unique<EnemyManager>(game, playerManager.get());
    
    // Check if we're the client
    bool isClient = (myID != hostIDSteam);

    // Initialize the static Shop pointer
    shop = std::make_unique<Shop>(game, playerManager.get());
    shopInstance = shop.get();

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
    shopInstance = nullptr; // Clear static pointer first
    shop.reset(); // Then reset the unique_ptr
    hostNetwork.reset();
    clientNetwork.reset();
    enemyManager.reset();
    playerRenderer.reset();
    playerManager.reset();
    
    std::cout << "[DEBUG] PlayingState destructor completed\n";
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
        }
    } else {
        // Update PlayerManager with the game instance (for InputManager access)
        playerManager->Update(game);
        if (clientNetwork) clientNetwork->Update();
        if (hostNetwork) hostNetwork->Update();
        
        // Update enemies
        if (playerLoaded && enemyManager) {
            enemyManager->Update(dt);
            
            // Check for bullet-enemy collisions
            CheckBulletEnemyCollisions();
            
            // Handle wave logic
            if (enemyManager->IsWaveComplete() && !waitingForNextWave) {
                waitingForNextWave = true;
                waveTimer = WAVE_COOLDOWN_TIME; // 5 second delay between waves
                
                // Display a message about the next wave
                std::string waveMsg = "Wave " + std::to_string(enemyManager->GetCurrentWave()) + 
                                    " complete! Next wave in " + std::to_string(static_cast<int>(waveTimer)) + "...";
                game->GetHUD().updateText("waveInfo", waveMsg);
            }
            
            if (waitingForNextWave) {
                waveTimer -= dt;
                
                // Update the countdown timer
                if (waveTimer > 0.0f) {
                    std::string waveMsg = "Wave " + std::to_string(enemyManager->GetCurrentWave()) + 
                                        " complete! Next wave in " + std::to_string(static_cast<int>(std::ceil(waveTimer))) + "...";
                    game->GetHUD().updateText("waveInfo", waveMsg);
                }
                
                if (waveTimer <= 0.0f) {
                    waitingForNextWave = false;
                    int nextWave = enemyManager->GetCurrentWave() + 1;
                    int enemyCount = BASE_ENEMIES_PER_WAVE + (nextWave * ENEMIES_SCALE_PER_WAVE);
                    StartWave(enemyCount);
                }
            }
            
            // Only the host starts the first wave
            CSteamID myID = SteamUser()->GetSteamID();
            CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
            
            if (myID == hostID && enemyManager->GetCurrentWave() == 0 && playerLoaded && !waitingForNextWave) {
                StartWave(FIRST_WAVE_ENEMY_COUNT); // First wave with 5 enemies
            }
            
            // Update wave info on HUD if not waiting for next wave
            if (!waitingForNextWave) {
                UpdateWaveInfo();
            }
        }
        
        auto& localPlayer = playerManager->GetLocalPlayer();
        if (localPlayer.player.IsDead() && localPlayer.respawnTimer > 0.0f) {
            // Format timer text
            int seconds = static_cast<int>(std::ceil(localPlayer.respawnTimer));
            std::string timerText = "Respawning in " + std::to_string(seconds) + "...";
            
            // Update the HUD element
            game->GetHUD().updateText("deathTimer", timerText);
            
            if (!isDeathTimerVisible) {
                isDeathTimerVisible = true;
            }
        } else if (isDeathTimerVisible) {
            // Hide timer when player is alive
            game->GetHUD().updateText("deathTimer", "");
            isDeathTimerVisible = false;
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
        if (showShop && shop) {
            shop->Update(dt);
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
            
            // Render enemies after players
            if (enemyManager) {
                enemyManager->Render(game->GetWindow());
            }
        }
        
        // Switch to UI view for HUD rendering
        game->GetWindow().setView(game->GetUIView());
        
        // Render HUD elements using the UI view
        game->GetHUD().render(game->GetWindow(), game->GetUIView(), GameState::Playing);
        // Render shop if visible
        if (showShop && shop) {
            shop->Render(game->GetWindow());
        }
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

void PlayingState::ProcessEvent(const sf::Event& event) {
    // Only process shop events if shop is open
    if (showShop && shop) {
        shop->ProcessEvent(event);
    }
    
    // Call the internal event processing function
    ProcessEvents(event);
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
        else if (event.key.code == sf::Keyboard::B) { // Check for 'B' key directly
            std::cout << "Opened shop" << std::endl;
            // Toggle shop visibility
            showShop = !showShop;
            if (shop) {
                if (showShop) {
                    shop->Toggle();
                    
                    // Ensure escape menu is closed when shop is opened
                    showEscapeMenu = false;
                } else {
                    shop->Close();
                }
            }
        }
        else if (game->GetInputManager().IsActionTriggered(GameAction::OpenMenu, event)) {
            // Toggle escape menu
            showEscapeMenu = !showEscapeMenu;
            
            if (showEscapeMenu) {
                // Show menu, release cursor, and make system cursor visible
                cursorLocked = false;
                game->GetWindow().setMouseCursorGrabbed(false);
                
                // Close shop if open
                if (showShop && shop) {
                    showShop = false;
                    shop->Close();
                }
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

void PlayingState::AttemptShoot(int mouseX, int mouseY) {
    // Skip if the game is paused or not fully loaded
    if (showEscapeMenu || !playerLoaded) return;
    
    // Get the current mouse position in world coordinates
    sf::Vector2i mousePos(mouseX, mouseY);
    sf::Vector2f worldPos = game->GetWindow().mapPixelToCoords(mousePos, game->GetCamera());
    
    // Get the local player
    auto& localPlayer = playerManager->GetLocalPlayer();
    
    // Skip if player is dead
    if (localPlayer.player.IsDead()) return;
    
    // Calculate direction vector from player to mouse position
    sf::Vector2f playerPos = localPlayer.player.GetPosition();
    sf::Vector2f direction = worldPos - playerPos;
    
    // Normalize the direction vector
    float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
    if (length > 0) {
        direction /= length;
    }
    
    // Get local player ID
    std::string localPlayerID = std::to_string(SteamUser()->GetSteamID().ConvertToUint64());
    
    // Create a bullet
    float bulletSpeed = BULLET_SPEED * localPlayer.player.GetBulletSpeedMultiplier();
    playerManager->AddBullet(localPlayerID, playerPos, direction, bulletSpeed);
    
    // Send the bullet message to all other players
    std::string bulletMsg = MessageHandler::FormatBulletMessage(
        localPlayerID, playerPos, direction, bulletSpeed);
    
    // Send to host if client, broadcast if host
    if (clientNetwork) {
        game->GetNetworkManager().SendMessage(clientNetwork->GetHostID(), bulletMsg);
    } else if (hostNetwork) {
        game->GetNetworkManager().BroadcastMessage(bulletMsg);
    }
}

void PlayingState::UpdatePlayerStats() {
    if (!playerManager) return;
    
    // Get the local player
    const auto& localPlayer = playerManager->GetLocalPlayer();
    
    // Format stats string
    std::string statsText = "HP: " + std::to_string(localPlayer.player.GetHealth()) + 
                            " | Kills: " + std::to_string(localPlayer.kills) + 
                            " | Money: " + std::to_string(localPlayer.money);
    
    // Update the HUD
    game->GetHUD().updateText("playerStats", statsText);
    
    // Update color based on health
    int health = localPlayer.player.GetHealth();
    if (health < 30) {
        game->GetHUD().updateBaseColor("playerStats", sf::Color::Red);
    } else if (health < 70) {
        game->GetHUD().updateBaseColor("playerStats", sf::Color(255, 165, 0)); // Orange
    } else {
        game->GetHUD().updateBaseColor("playerStats", sf::Color::Black);
    }
}

void PlayingState::UpdateLeaderboard() {
    if (!playerManager) return;
    
    // Get all players
    auto& players = playerManager->GetPlayers();
    
    // Vector to hold player data for sorting
    std::vector<std::pair<std::string, int>> playerData;
    
    // Collect data
    for (const auto& pair : players) {
        playerData.push_back({pair.second.baseName, pair.second.kills});
    }
    
    // Sort by kills (highest first)
    std::sort(playerData.begin(), playerData.end(), 
        [](const auto& a, const auto& b) { return a.second > b.second; });
    
    // Format leaderboard string
    std::string leaderboardText = "LEADERBOARD\n\n";
    
    for (size_t i = 0; i < playerData.size(); ++i) {
        leaderboardText += std::to_string(i + 1) + ". " + 
                          playerData[i].first + " - " + 
                          std::to_string(playerData[i].second) + " kills\n";
    }
    
    // Update the HUD
    game->GetHUD().updateText("leaderboard", leaderboardText);
}

void PlayingState::CheckBulletEnemyCollisions() {
    if (!playerManager || !enemyManager) return;
    
    // Get all bullets
    const auto& bullets = playerManager->GetAllBullets();
    std::vector<size_t> bulletsToRemove;
    
    // Check each bullet for collisions with enemies
    for (size_t bulletIdx = 0; bulletIdx < bullets.size(); bulletIdx++) {
        const Bullet& bullet = bullets[bulletIdx];
        
        // Skip already expired bullets
        if (bullet.IsExpired()) {
            bulletsToRemove.push_back(bulletIdx);
            continue;
        }
        
        sf::Vector2f bulletPos = bullet.GetPosition();
        int enemyId = -1;
        
        // Check if bullet hits any enemy
        if (enemyManager->CheckBulletCollision(bulletPos, 4.0f, enemyId)) {
            // If we hit an enemy, add this bullet to remove list
            bulletsToRemove.push_back(bulletIdx);
            
            // Apply damage to the enemy
            std::string shooterId = bullet.GetShooterID();
            
            // Get local player ID
            CSteamID myID = SteamUser()->GetSteamID();
            CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
            std::string localPlayerID = std::to_string(myID.ConvertToUint64());
            
            if (myID == hostID) {
                // Host handles damage directly
                bool killed = enemyManager->InflictDamage(enemyId, 20.0f); // 20 damage per bullet
                
                // If player killed an enemy, reward them
                if (killed && !shooterId.empty()) {
                    playerManager->IncrementPlayerKills(shooterId);
                    
                    // Add money reward
                    auto& players = playerManager->GetPlayers();
                    auto it = players.find(shooterId);
                    if (it != players.end()) {
                        it->second.money += TRIANGLE_KILL_REWARD;
                    }
                }
            } else {
                // Client sends damage message to host
                std::string damageMsg = MessageHandler::FormatEnemyDamageMessage(enemyId, 20.0f, 0.0f);
                game->GetNetworkManager().SendMessage(hostID, damageMsg);
            }
        }
    }
    
    // Remove all collided bullets
    if (!bulletsToRemove.empty()) {
        playerManager->RemoveBullets(bulletsToRemove);
    }
}

void PlayingState::StartWave(int enemyCount) {
    if (!enemyManager) return;
    
    // Start the wave
    enemyManager->StartNewWave(enemyCount, EnemyType::Triangle);
    
    // Update wave info
    UpdateWaveInfo();
    
    // Update the game header with current wave
    std::string headerText = "WAVE " + std::to_string(enemyManager->GetCurrentWave());
    game->GetHUD().updateText("gameHeader", headerText);
}

void PlayingState::UpdateWaveInfo() {
    if (!enemyManager) return;
    
    // Update wave info on HUD
    std::string waveInfo = "Wave: " + std::to_string(enemyManager->GetCurrentWave()) + 
                          " | Enemies: " + std::to_string(enemyManager->GetEnemyCount());
    game->GetHUD().updateText("waveInfo", waveInfo);
}

bool PlayingState::IsFullyLoaded() {
    return playerLoaded;
}