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
#include "PlayingStateUI.h"
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
    
    // Create a stylish grid with modern colors that match the UI theme
    grid = Grid(50.f, sf::Color(180, 180, 180, 100)); // Slightly transparent grid lines
    grid.setMajorLineInterval(5);
    grid.setMajorLineColor(sf::Color(150, 150, 150, 180)); // Slightly darker for major lines
    grid.setOriginHighlight(true);
    grid.setOriginHighlightColor(sf::Color(100, 100, 255, 120)); // Subtle blue for origin
    grid.setOriginHighlightSize(15.0f);
    
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

    // Initialize enemy manager with player manager reference
    enemyManager = std::make_unique<EnemyManager>(game, playerManager.get());
    
    // Initialize the PlayingStateUI
    ui = std::make_unique<PlayingStateUI>(game, playerManager.get(), enemyManager.get());
    
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
    ui.reset(); // UI before game systems
    shop.reset(); // Then reset the unique_ptr
    hostNetwork.reset();
    clientNetwork.reset();
    enemyManager.reset();
    playerRenderer.reset();
    playerManager.reset();
    
    std::cout << "[DEBUG] PlayingState destructor completed\n";
}

void PlayingState::Update(float dt) {
    // Update UI with animations
    if (ui) {
        ui->Update(dt);
    }
    
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
            
            // Handle wave logic
            if (enemyManager->IsWaveComplete() && !waitingForNextWave) {
                waitingForNextWave = true;
                waveTimer = WAVE_COOLDOWN_TIME; // 5 second delay between waves
                
                // Display wave complete message using UI
                if (ui) {
                    ui->SetWaveCompleteMessage(enemyManager->GetCurrentWave(), waveTimer);
                }
            }
            
            if (waitingForNextWave) {
                waveTimer -= dt;
                
                // Update the countdown timer
                if (waveTimer > 0.0f && ui) {
                    ui->SetWaveCompleteMessage(enemyManager->GetCurrentWave(), waveTimer);
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
                StartWave(FIRST_WAVE_ENEMY_COUNT); // First wave with defined number of enemies
            }
            
            // Update wave info on HUD if not waiting for next wave
            if (!waitingForNextWave && ui) {
                ui->UpdateWaveInfo();
            }
        }
        
        // Update UI components
        if (ui) {
            // Update player stats display
            ui->UpdatePlayerStats();
            
            // Update death timer if player is dead
            ui->UpdateDeathTimer(isDeathTimerVisible);
            
            // Update leaderboard if visible
            ui->UpdateLeaderboard(showLeaderboard);
            
            // Update button states based on toggles
            ui->SetButtonState("gridToggle", showGrid);
            ui->SetButtonState("cursorLockHint", cursorLocked);
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
        
        // Handle cursor lock if enabled
        if (cursorLocked && !showEscapeMenu) {
            sf::Vector2i mousePos = sf::Mouse::getPosition(game->GetWindow());
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
        
        // Update shop if visible
        if (showShop && shop) {
            shop->Update(dt);
        }
    }
    
    // Center camera on local player
    sf::Vector2f localPlayerPos = playerManager->GetLocalPlayer().player.GetPosition();
    game->GetCamera().setCenter(localPlayerPos);
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
        
        // Render escape menu if active using the UI class
        if (showEscapeMenu && ui) {
            ui->RenderEscapeMenu(game->GetWindow());
        }
        
        game->GetWindow().display();
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Exception in PlayingState::Render: " << e.what() << std::endl;
    }
}

void PlayingState::ProcessEvent(const sf::Event& event) {
    // Process shop events if shop is open
    if (showShop && shop) {
        shop->ProcessEvent(event);
    }
    
    // Let the UI process events first
    bool uiHandled = false;
    bool returnToMainMenu = false;
    
    if (ui) {
        uiHandled = ui->ProcessUIEvent(event, showEscapeMenu, showGrid, 
                                      cursorLocked, showLeaderboard, returnToMainMenu);
        
        // Check if we should return to main menu
        if (returnToMainMenu) {
            game->SetCurrentState(GameState::MainMenu);
            return;
        }
    }
    
    // If UI didn't handle the event, process gameplay events
    if (!uiHandled) {
        ProcessGameplayEvents(event);
    }
}

void PlayingState::ProcessGameplayEvents(const sf::Event& event) {
    // Only process gameplay events if menu is not active
    if (showEscapeMenu) return;
    
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        // Get mouse position in window coordinates
        sf::Vector2i mouseWindowPos(event.mouseButton.x, event.mouseButton.y);
        
        // Convert to UI coordinates for UI interaction
        sf::Vector2f mouseUIPos = game->WindowToUICoordinates(mouseWindowPos);
        
        // Check if we're clicking on a UI element
        if (mouseUIPos.x >= 0 && mouseUIPos.y >= 0) {
            // Only treat left mouse as shooting if no keyboard key is bound
            bool isLeftMouseShoot = game->GetInputHandler()->GetKeyForAction(InputAction::Shoot) == sf::Keyboard::Unknown;
            
            if (isLeftMouseShoot) {
                mouseHeld = true;
                AttemptShoot(mouseWindowPos.x, mouseWindowPos.y);
            }
        }
    } 
    else if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
        mouseHeld = false;
    }
    else if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::B) {
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

void PlayingState::StartWave(int enemyCount) {
    if (!enemyManager) return;
    
    // Start the wave
    enemyManager->StartNewWave(enemyCount, EnemyType::Triangle);
    
    // Update wave info using UI
    if (ui) {
        ui->UpdateWaveInfo();
    }
}

bool PlayingState::IsFullyLoaded() {
    return playerLoaded;
}