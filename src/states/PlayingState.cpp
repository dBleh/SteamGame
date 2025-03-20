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
#include "../network/messages/PlayerMessageHandler.h"
#include "../network/messages/EnemyMessageHandler.h"
#include "../network/messages/StateMessageHandler.h"
#include "../network/messages/SystemMessageHandler.h"
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
      waitingForNextWave(false),
      forceFieldsInitialized(false) {  // Add new flag to track force field initialization
    
    std::cout << "[DEBUG] PlayingState constructor start\n";
    
    // Initialize default zoom with settings if available
    if (game->GetGameSettingsManager()) {
        GameSetting* defaultZoomSetting = game->GetGameSettingsManager()->GetSetting("default_zoom");
        if (defaultZoomSetting) {
            defaultZoom = defaultZoomSetting->GetFloatValue();
            currentZoom = defaultZoom;
        } else {
            currentZoom = DEFAULT_ZOOM;
        }
    } else {
        currentZoom = DEFAULT_ZOOM;
    }
    
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
        std::string hostConnectMsg = PlayerMessageHandler::FormatConnectionMessage(
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
    
    // Apply settings BEFORE initializing force fields
    if (game->GetGameSettingsManager()) {
        std::cout << "[PlayingState] Applying initial game settings" << std::endl;
        ApplyAllSettings();
    }
    
    // Defer force field initialization until after settings are applied
    // This will be handled in the first Update() call
    
    std::cout << "[DEBUG] PlayingState constructor completed\n";
}
void PlayingState::InitializeForceFields() {
    if (forceFieldsInitialized) return; // Only do this once
    
    std::cout << "[PlayingState] Now initializing force fields after settings are applied\n";
    
    // Only initialize force fields for the local player here
    auto& localPlayerRef = playerManager->GetLocalPlayer();
    if (!localPlayerRef.player.HasForceField()) {
        localPlayerRef.player.InitializeForceField();
        std::cout << "[PlayingState] Initialized force field for local player\n";
    }
    
    // Initialize force fields for all remote players too
    auto& players = playerManager->GetPlayers();
    for (auto& pair : players) {
        // Skip local player, already handled
        if (pair.first == playerManager->GetLocalPlayer().playerID) continue;
        
        RemotePlayer& rp = pair.second;
        if (!rp.player.HasForceField()) {
            rp.player.InitializeForceField();
            std::cout << "[PlayingState] Initialized force field for remote player " << rp.baseName << "\n";
        }
    }
    
    // Initialize callbacks for force fields
    for (auto& pair : players) {
        std::string playerID = pair.first;
        RemotePlayer& rp = pair.second;
        
        if (rp.player.HasForceField()) {
            ForceField* forceField = rp.player.GetForceField();
            if (forceField && !forceField->HasZapCallback()) {
                // Store player ID by value to avoid reference issues
                std::string callbackPlayerID = playerID;
                forceField->SetZapCallback([this, callbackPlayerID](int enemyId, float damage, bool killed) {
                    // Handle zap event safely
                    if (playerManager) {
                        playerManager->HandleForceFieldZap(callbackPlayerID, enemyId, damage, killed);
                    }
                });
                std::cout << "[PlayingState] Set zap callback for player " << rp.baseName << "\n";
            }
        }
    }
    
    forceFieldsInitialized = true;
}
PlayingState::~PlayingState() {
    game->GetWindow().setMouseCursorGrabbed(false);
    
    // Reset player states before destroying
    if (playerManager) {
        auto& players = playerManager->GetPlayers();
        for (auto& pair : players) {
            // Reset health, position, etc.
            pair.second.player.Respawn();
            // Reset kills and money for lobby
            pair.second.kills = 0;
            pair.second.money = 0;
        }
    }
    
    shopInstance = nullptr; 
    ui.reset(); 
    shop.reset(); 
    hostNetwork.reset();
    clientNetwork.reset();
    enemyManager.reset();
    playerRenderer.reset();
    playerManager.reset();
    
    std::cout << "[DEBUG] PlayingState destructor completed\n";
}

void PlayingState::Update(float dt) {
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
        // Once player is loaded, initialize force fields if not already done
        // This ensures we have received settings first
        if (!forceFieldsInitialized) {
            // If we're a client, wait until we've received settings from host
            bool isClient = false;
            CSteamID myID = SteamUser()->GetSteamID();
            CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
            
            if (myID != hostID) {
                isClient = true;
            }
            
            // For clients, check if the client has received settings
            if (isClient && clientNetwork) {
                if (clientNetwork->HasReceivedInitialSettings()) {
                    std::cout << "[CLIENT] Initial settings received, initializing force fields\n";
                    InitializeForceFields();
                } else {
                    // Request settings if not received yet
                    static float requestTimer = 0.0f;
                    requestTimer += dt;
                    if (requestTimer >= 1.0f) {
                        std::cout << "[CLIENT] Waiting for initial settings before force field initialization\n";
                        clientNetwork->RequestGameSettings();
                        requestTimer = 0.0f;
                    }
                }
            } else {
                // Host can initialize immediately
                InitializeForceFields();
            }
        }
        
        // Update PlayerManager with the game instance (for InputManager access)
        playerManager->Update(game);
        if (clientNetwork) {
            clientNetwork->Update();
        }
        if (hostNetwork) {
            hostNetwork->Update();
        }
        
        // The rest of the update method remains the same...
        // [existing code...]
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
            
            // MODIFIED: Safely render force fields for all players
            for (auto& pair : playerManager->GetPlayers()) {
                try {
                    RemotePlayer& rp = pair.second;
                    if (rp.player.HasForceField() && !rp.player.IsDead()) {
                        ForceField* forceField = rp.player.GetForceField();
                        if (forceField) {
                            forceField->Render(game->GetWindow());
                        }
                    }
                } catch (const std::exception& e) {
                    std::cerr << "[ERROR] Exception rendering force field: " << e.what() << std::endl;
                }
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

    if(!showShop){
    if (event.type == sf::Event::MouseWheelScrolled && event.mouseWheelScroll.wheel == sf::Mouse::VerticalWheel) {
        // Negative delta means scroll down (zoom out), positive means scroll up (zoom in)
        HandleZoom(-event.mouseWheelScroll.delta * ZOOM_SPEED);
        return;
    }
}
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
                
                // Convert mouse coords to world position and delegate to PlayerManager
                sf::Vector2f mouseWorldPos = game->GetWindow().mapPixelToCoords(mouseWindowPos, game->GetCamera());
                if (playerManager) {
                    playerManager->PlayerShoot(mouseWorldPos);
                }
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
    else if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::F) {
        // Toggle force field for local player
        auto& localPlayer = playerManager->GetLocalPlayer();
        
        // Initialize force field if not already done
        if (!localPlayer.player.HasForceField()) {
            localPlayer.player.InitializeForceField();
        }
        
        // Toggle the force field
        bool newState = !localPlayer.player.HasForceField();
        localPlayer.player.EnableForceField(newState);
        
        // Display a message about the force field status
        std::string statusMsg = newState ? 
                              "Force Field Activated" : 
                              "Force Field Deactivated";
        
        // Send force field update to network
        if (localPlayer.player.GetForceField()) {
            ForceField* forceField = localPlayer.player.GetForceField();
            
            // Create the force field update message
            std::string updateMsg = PlayerMessageHandler::FormatForceFieldUpdateMessage(
                localPlayer.playerID,
                forceField->GetRadius(),
                forceField->GetDamage(),
                forceField->GetCooldown(),
                forceField->GetChainLightningTargets(),
                static_cast<int>(forceField->GetFieldType()),
                forceField->GetPowerLevel(),
                forceField->IsChainLightningEnabled()
            );
            
            // Check if we're the host
            CSteamID localSteamID = SteamUser()->GetSteamID();
            CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
            
            if (localSteamID == hostID) {
                // We are the host, broadcast to all clients
                game->GetNetworkManager().BroadcastMessage(updateMsg);
            } else {
                // We are a client, send to host
                game->GetNetworkManager().SendMessage(hostID, updateMsg);
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

void PlayingState::HandleZoom(float delta) {
    // Get mouse position before zoom for centering
    sf::Vector2i mousePos = sf::Mouse::getPosition(game->GetWindow());
    sf::Vector2f worldPos = game->GetWindow().mapPixelToCoords(mousePos, game->GetCamera());
    
    // Apply zoom change
    float newZoom = currentZoom + delta * zoomSpeed; // Use cached zoom speed
    
    // Clamp zoom to min/max values from settings
    newZoom = std::max(minZoom, std::min(maxZoom, newZoom));
    
    // Only update if zoom actually changed
    if (newZoom != currentZoom) {
        // Store new zoom level
        currentZoom = newZoom;
        
        // Update camera size based on zoom
        sf::Vector2u windowSize = game->GetWindow().getSize();
        float viewWidth = windowSize.x / currentZoom;
        float viewHeight = windowSize.y / currentZoom;
        
        // Set new view size
        game->GetCamera().setSize(viewWidth, viewHeight);
        
        // Center around mouse position
        // Get the new world position of the mouse
        sf::Vector2f newWorldPos = game->GetWindow().mapPixelToCoords(mousePos, game->GetCamera());
        
        // Adjust the camera position to keep the mouse over the same world position
        sf::Vector2f cameraDelta = worldPos - newWorldPos;
        game->GetCamera().setCenter(game->GetCamera().getCenter() + cameraDelta);
        
        std::cout << "Zoom level: " << currentZoom << std::endl;
    }
}

void PlayingState::AdjustViewToWindow() {
    sf::Vector2u windowSize = game->GetWindow().getSize();
    
    // Game world camera - shows more of the world when window is larger
    // Apply zoom factor to the camera size
    game->GetCamera().setSize(static_cast<float>(windowSize.x) / currentZoom, 
                              static_cast<float>(windowSize.y) / currentZoom);
    
    // Preserve current center
    game->GetCamera().setCenter(game->GetCamera().getCenter());
}

// In PlayingState.cpp, modify the OnSettingsChanged method to use the bulletDamage setting:

void PlayingState::OnSettingsChanged() {
    // Apply settings locally without propagating to network
    ApplyAllSettings();
    
    // Only propagate to network if we're the host and if this wasn't triggered by a network message
    CSteamID myID = SteamUser()->GetSteamID();
    CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
    
    static bool isProcessingNetworkSettings = false;
    if (myID == hostID && hostNetwork && !isProcessingNetworkSettings) {
        isProcessingNetworkSettings = true;
        // Broadcast updated settings to all clients
        hostNetwork->BroadcastGameSettings();
        isProcessingNetworkSettings = false;
    }
}


void PlayingState::ApplyAllSettings() {
    GameSettingsManager* settingsManager = game->GetGameSettingsManager();
    if (!settingsManager) return;
    
    // Apply settings to all components
    
    // 1. Apply to enemy manager
    if (enemyManager) {
        enemyManager->ApplySettings();
    }
    
    // 2. Apply to player manager and all players/bullets
    if (playerManager) {
        playerManager->ApplySettings();
    }
    
    // 3. Apply to wave settings
    GameSetting* waveCooldownSetting = settingsManager->GetSetting("wave_cooldown_time");
    if (waveCooldownSetting) {
        waveCooldownTime = waveCooldownSetting->GetFloatValue();
    }
    
    // 4. Apply to camera/zoom settings
    GameSetting* defaultZoomSetting = settingsManager->GetSetting("default_zoom");
    GameSetting* minZoomSetting = settingsManager->GetSetting("min_zoom");
    GameSetting* maxZoomSetting = settingsManager->GetSetting("max_zoom");
    GameSetting* zoomSpeedSetting = settingsManager->GetSetting("zoom_speed");
    
    if (defaultZoomSetting) {
        defaultZoom = defaultZoomSetting->GetFloatValue();
        // Only set current zoom if it hasn't been modified by the player yet
        if (currentZoom == DEFAULT_ZOOM) {
            currentZoom = defaultZoom;
            AdjustViewToWindow();
        }
    }
    
    if (minZoomSetting) minZoom = minZoomSetting->GetFloatValue();
    if (maxZoomSetting) maxZoom = maxZoomSetting->GetFloatValue();
    if (zoomSpeedSetting) zoomSpeed = zoomSpeedSetting->GetFloatValue();
    
    // 5. Apply to shop system if needed
    if (shop) {
        // The shop might need its own method to apply settings
        // shop->ApplySettings(settingsManager);
    }
    
    // 6. Apply to UI
    if (ui) {
        // The UI might need its own method to apply settings
        // ui->ApplySettings(settingsManager);
    }
    
    std::cout << "[PlayingState] Applied all game settings" << std::endl;
}

void PlayingState::UpdateForceFields(float dt) {
    if (!playerLoaded || !playerManager) return;
    
    // Thread safe copy of player IDs to avoid iterator invalidation during callback
    std::vector<std::string> playerIDs;
    
    {
        auto& players = playerManager->GetPlayers();
        playerIDs.reserve(players.size());
        for (const auto& pair : players) {
            playerIDs.push_back(pair.first);
        }
    }
    
    // Now iterate through the copied IDs
    for (const std::string& playerID : playerIDs) {
        auto& players = playerManager->GetPlayers();
        auto playerIt = players.find(playerID);
        if (playerIt == players.end()) continue;
        
        RemotePlayer& rp = playerIt->second;
        
        // Skip dead players
        if (rp.player.IsDead()) continue;
        
        try {
            // Ensure player has a force field initialized
            if (!rp.player.HasForceField()) {
                rp.player.InitializeForceField();
                std::cout << "[PlayingState] Initialized missing force field for player " << rp.baseName << "\n";
            }
            
            ForceField* forceField = rp.player.GetForceField();
            if (forceField) {
                // Check if we're a client and need a zap callback
                CSteamID myID = SteamUser()->GetSteamID();
                CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
                bool isClient = (myID != hostID);
                
                if (isClient && !forceField->HasZapCallback()) {
                    std::cout << "[CLIENT] Setting missing zap callback for player " << rp.baseName << "\n";
                    
                    // Store player ID by value to avoid reference issues
                    std::string callbackPlayerID = playerID;
                    forceField->SetZapCallback([this, callbackPlayerID](int enemyId, float damage, bool killed) {
                        // Handle zap event safely
                        if (playerManager) {
                            playerManager->HandleForceFieldZap(callbackPlayerID, enemyId, damage, killed);
                        }
                    });
                }
                
                // Update the force field with careful exception handling
                try {
                    forceField->Update(dt, *playerManager, *enemyManager);
                } catch (const std::exception& e) {
                    std::cerr << "[ERROR] Exception in force field update for player " 
                             << rp.baseName << ": " << e.what() << std::endl;
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "[ERROR] Exception in force field processing for player " 
                     << rp.baseName << ": " << e.what() << std::endl;
        }
    }
}