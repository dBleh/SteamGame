#include "LobbyState.h"
#include "../utils/Config.h"
#include <iostream>
#include <Steam/steam_api.h>
#include "../network/Client.h"
#include "../network/Host.h"
#include "../render/PlayerRenderer.h"
#include "../entities/PlayerManager.h"

LobbyState::LobbyState(Game* game)
    : State(game), 
      playerLoaded(false), 
      loadingTimer(0.f), 
      chatMessages(""),
      grid(50.f, sf::Color(180, 180, 180)), // Darker grid color to match the UI theme
      showGrid(true) {
    
    std::cout << "[DEBUG] LobbyState constructor start\n";
    
    // Use fixed dimensions based on BASE_WIDTH and BASE_HEIGHT
    float centerX = BASE_WIDTH / 2.0f;
    
    // Get lobby name
    std::string lobbyName = SteamMatchmaking()->GetLobbyData(game->GetLobbyID(), "name");
    if (lobbyName.empty()) {
        lobbyName = "Lobby";
    }
    
    // Fixed positions instead of percentages
    float titleY = 25.0f; // Fixed pixel position at top
    float statusBarY = BASE_HEIGHT - 120.0f; // Fixed position from bottom
    float lineWidth = 800.0f; // Fixed width for lines
    float lineThickness = 2.0f; // Fixed thickness
    
    // Calculate line start X position to center the line
    float lineStartX = centerX - (lineWidth / 2.0f);
    
    // ===== TOP SECTION =====
    // Lobby title at the top
    game->GetHUD().addElement("lobbyHeader", lobbyName, 48, 
                             sf::Vector2f(centerX - (lobbyName.length() * 12), titleY), 
                             GameState::Lobby, 
                             HUD::RenderMode::ScreenSpace, false);
    
    // Top separator line
    game->GetHUD().addGradientLine("lobbyTopLine", 
                                  lineStartX,
                                  titleY + 60.0f, 
                                  lineWidth, 
                                  lineThickness,
                                  sf::Color::Black, 
                                  GameState::Lobby, 
                                  HUD::RenderMode::ScreenSpace,
                                  30);
    
    // Player count and loading status
    game->GetHUD().addElement("playerLoading", "Loading players...", 20, 
                             sf::Vector2f(centerX - 100.0f, titleY + 80.0f), 
                             GameState::Lobby, 
                             HUD::RenderMode::ScreenSpace, false);
    
    // ===== BOTTOM STATUS BAR =====
    // Bottom status bar background
    game->GetHUD().addGradientLine("statusBarLine", 
                                  lineStartX,
                                  statusBarY, 
                                  lineWidth, 
                                  lineThickness,
                                  sf::Color::Black, 
                                  GameState::Lobby, 
                                  HUD::RenderMode::ScreenSpace,
                                  30);
    
    // Ready status toggle button
    game->GetHUD().addElement("readyButton", "Press R to Ready Up", 20, 
                             sf::Vector2f(centerX - 280.0f, statusBarY + 40.0f), 
                             GameState::Lobby, 
                             HUD::RenderMode::ScreenSpace, true,
                             "statusBarLine", "");
    game->GetHUD().updateBaseColor("readyButton", sf::Color::Black);
    
    // Start game button (for host)
    game->GetHUD().addElement("startGame", "Waiting for players...", 24, 
                             sf::Vector2f(centerX, statusBarY + 40.0f), 
                             GameState::Lobby, 
                             HUD::RenderMode::ScreenSpace, true,
                             "statusBarLine", "");
    game->GetHUD().updateBaseColor("startGame", sf::Color::Black);
    
    // Grid toggle button
    game->GetHUD().addElement("gridToggle", "Toggle Grid [G]", 20, 
                             sf::Vector2f(centerX + 280.0f, statusBarY + 40.0f), 
                             GameState::Lobby, 
                             HUD::RenderMode::ScreenSpace, true,
                             "statusBarLine", "");
    game->GetHUD().updateBaseColor("gridToggle", sf::Color::Black);
    
    // Return to main menu button
    game->GetHUD().addElement("returnMain", "Back to Menu [M]", 20, 
                             sf::Vector2f(centerX, statusBarY + 80.0f), 
                             GameState::Lobby, 
                             HUD::RenderMode::ScreenSpace, true,
                             "", "");
    game->GetHUD().updateBaseColor("returnMain", sf::Color::Black);
    
    // ===== PLAYER SETUP =====
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

    // ===== NETWORK SETUP =====
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
    
    // Add player list display
    game->GetHUD().addElement("playerList", "Players:", 20, 
                             sf::Vector2f(50.f, titleY + 120.0f), 
                             GameState::Lobby, 
                             HUD::RenderMode::ScreenSpace, false);
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
    
    // Update player list display
    std::string playerListText = "Players:";
    auto& players = playerManager->GetPlayers();
    for (const auto& pair : players) {
        const RemotePlayer& player = pair.second;
        std::string readyStatus = player.isReady ? " [READY]" : " [NOT READY]";
        std::string hostStatus = player.isHost ? " (Host)" : "";
        playerListText += "\n• " + player.baseName + hostStatus + readyStatus;
    }
    game->GetHUD().updateText("playerList", playerListText);
}


void LobbyState::Render() {
    // Clear with background color
    game->GetWindow().clear(MAIN_BACKGROUND_COLOR);
    
    // Use game camera for world elements (grid and players)
    game->GetWindow().setView(game->GetCamera());
    
    if (showGrid) {
        grid.render(game->GetWindow(), game->GetCamera());
    }
    
    if (playerLoaded) {
        playerRenderer->Render(game->GetWindow()); // Renders all players
    }
    
    // Use UI view for UI elements
    game->GetWindow().setView(game->GetUIView());
    game->GetHUD().render(game->GetWindow(), game->GetUIView(), GameState::Lobby);
    
    game->GetWindow().display();
}

void LobbyState::ProcessEvents(const sf::Event& event) {
    InputManager& inputManager = game->GetInputManager();
    
    if (event.type == sf::Event::KeyPressed) {
        // Check if the ready toggle key was pressed using InputManager
        if (event.key.code == inputManager.GetKeyBinding(GameAction::ToggleReady)) {
            // Toggle ready status
            std::string myID = std::to_string(SteamUser()->GetSteamID().ConvertToUint64());
            bool currentReady = playerManager->GetLocalPlayer().isReady;
            bool newReady = !currentReady;
            playerManager->SetReadyStatus(myID, newReady);
            std::cout << "Ready status set to " << newReady << "\n";
            std::string msg = MessageHandler::FormatReadyStatusMessage(myID, newReady);
            if (hostNetwork) {
                game->GetNetworkManager().BroadcastMessage(msg);
            } else if (clientNetwork) {
                clientNetwork->SendReadyStatus(newReady);
            }
        }
        else if (event.key.code == inputManager.GetKeyBinding(GameAction::ToggleGrid)) {
            // Toggle grid visibility
            showGrid = !showGrid;
        }
        else if (event.key.code == sf::Keyboard::M) {
            // Return to main menu
            game->SetCurrentState(GameState::MainMenu);
        }
    } 
    else if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        // Check if this should trigger the shoot action based on input settings
        bool shouldShoot = false;
        
        // If the shoot action is bound to a mouse button
        if (event.mouseButton.button == sf::Mouse::Left && 
            inputManager.GetKeyBinding(GameAction::Shoot) == sf::Keyboard::Unknown) {
            shouldShoot = true;
        }
        
        // Get mouse position
        sf::Vector2i mousePos(event.mouseButton.x, event.mouseButton.y);
        
        // Check for UI element clicks
        sf::Vector2f mouseUIPos = game->WindowToUICoordinates(mousePos);
        
        // Only process UI clicks if within the UI viewport
        if (mouseUIPos.x >= 0 && mouseUIPos.y >= 0) {
            const auto& elements = game->GetHUD().getElements();
            
            for (const auto& [id, element] : elements) {
                if (element.hoverable && element.visibleState == GameState::Lobby) {
                    sf::Text textCopy = element.text;
                    textCopy.setPosition(element.pos);
                    sf::FloatRect bounds = textCopy.getGlobalBounds();
                    
                    if (bounds.contains(mouseUIPos)) {
                        // Handle UI element clicks
                        if (id == "startGame") {
                            // Check if we're the host
                            CSteamID myID = SteamUser()->GetSteamID();
                            CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
                            if (myID == hostID && AllPlayersReady()) {
                                // Only proceed if we're not already transitioning
                                if (game->GetCurrentState() == GameState::Lobby) {
                                    std::cout << "[LOBBY] Host clicked start game, sending message and transitioning" << std::endl;
                                    // Broadcast "start game" message
                                    std::string startMsg = MessageHandler::FormatStartGameMessage(std::to_string(myID.ConvertToUint64()));
                                    game->GetNetworkManager().BroadcastMessage(startMsg);
                                    
                                    // Switch to playing state
                                    game->SetCurrentState(GameState::Playing);
                                }
                            }
                        }
                        else if (id == "readyButton") {
                            // Toggle ready status
                            std::string myID = std::to_string(SteamUser()->GetSteamID().ConvertToUint64());
                            bool currentReady = playerManager->GetLocalPlayer().isReady;
                            bool newReady = !currentReady;
                            playerManager->SetReadyStatus(myID, newReady);
                            std::cout << "Ready status set to " << newReady << "\n";
                            std::string msg = MessageHandler::FormatReadyStatusMessage(myID, newReady);
                            if (hostNetwork) {
                                game->GetNetworkManager().BroadcastMessage(msg);
                            } else if (clientNetwork) {
                                clientNetwork->SendReadyStatus(newReady);
                            }
                        }
                        else if (id == "gridToggle") {
                            // Toggle grid visibility
                            showGrid = !showGrid;
                        }
                        else if (id == "returnMain") {
                            // Return to main menu
                            game->SetCurrentState(GameState::MainMenu);
                        }
                        
                        // We found a UI element that was clicked, so don't process as a game click
                        return;
                    }
                }
            }
        }
        
        // If we got here and should shoot, no UI element was clicked, so process as a game click
        if (shouldShoot) {
            std::string myIDStr = std::to_string(SteamUser()->GetSteamID().ConvertToUint64());
            
            // Don't shoot if player is dead
            if (playerManager->GetLocalPlayer().player.IsDead()) {
                return;
            }
            
            mouseHeld = true;
            AttemptShoot(mousePos.x, mousePos.y);
        }
    }
    else if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
        mouseHeld = false;
    }
    // Handle keyboard-based shooting
    else if (event.type == sf::Event::KeyPressed && 
             event.key.code == inputManager.GetKeyBinding(GameAction::Shoot) && 
             event.key.code != sf::Keyboard::Unknown) {
        // Shooting with keyboard
        sf::Vector2i mousePos = sf::Mouse::getPosition(game->GetWindow());
        
        std::string myIDStr = std::to_string(SteamUser()->GetSteamID().ConvertToUint64());
        
        // Don't shoot if player is dead
        if (playerManager->GetLocalPlayer().player.IsDead()) {
            return;
        }
        
        mouseHeld = true;
        AttemptShoot(mousePos.x, mousePos.y);
    }
    else if (event.type == sf::Event::KeyReleased && 
             event.key.code == inputManager.GetKeyBinding(GameAction::Shoot) && 
             event.key.code != sf::Keyboard::Unknown) {
        mouseHeld = false;
    }
}

void LobbyState::Update(float dt) {
    // Update HUD animations
    game->GetHUD().update(game->GetWindow(), GameState::Lobby, dt);
    
    if (!playerLoaded) {
        loadingTimer += dt;
        if (loadingTimer >= 2.0f) {
            playerLoaded = true;
            game->GetHUD().updateText("playerLoading", "Players Loaded");
            
            // After a delay, fade out the loading message
            if (loadingTimer >= 3.0f) {
                game->GetHUD().updateText("playerLoading", "");
            }
            
            if (clientNetwork && !connectionSent) {
                clientNetwork->SendConnectionMessage();
                connectionSent = true;
            }
        }
    } else {
        // Update PlayerManager (includes local player movement)
        // Ensure we're passing the Game pointer for InputManager access
        playerManager->Update(game);
        
        if (clientNetwork) clientNetwork->Update();
        if (hostNetwork) hostNetwork->Update();
        
        // Update the player list display
        UpdateRemotePlayers();
    }
    
    // Handle continuous shooting if mouse/key is held down
    if (mouseHeld) {
        shootTimer -= dt;
        if (shootTimer <= 0.f) {
            // Get current mouse position
            sf::Vector2i mousePos = sf::Mouse::getPosition(game->GetWindow());
            AttemptShoot(mousePos.x, mousePos.y);
            shootTimer = 0.1f; // Shoot every 0.1 seconds when holding down
        }
    }
    
    // Update ready status button color
    std::string myID = std::to_string(SteamUser()->GetSteamID().ConvertToUint64());
    bool isReady = playerManager->GetLocalPlayer().isReady;
    if (isReady) {
        game->GetHUD().updateText("readyButton", "Ready [R to Cancel]");
        game->GetHUD().updateBaseColor("readyButton", sf::Color::Green);
    } else {
        game->GetHUD().updateText("readyButton", "Press R to Ready Up");
        game->GetHUD().updateBaseColor("readyButton", sf::Color::Black);
    }
    
    // Update start game button for host
    if (SteamUser()->GetSteamID() == SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID())) {
        // Check if all players are ready
        bool allReady = AllPlayersReady();
        if (allReady) {
            game->GetHUD().updateText("startGame", "Start Game");
            game->GetHUD().updateBaseColor("startGame", sf::Color::Green);
        } else {
            game->GetHUD().updateText("startGame", "Waiting for All Players");
            game->GetHUD().updateBaseColor("startGame", sf::Color(150, 150, 150)); // Gray when not ready
        }
    } else {
        game->GetHUD().updateText("startGame", "Waiting for Host");
        game->GetHUD().updateBaseColor("startGame", sf::Color(150, 150, 150)); // Gray for non-host
    }
    
    // Update grid toggle button color
    if (showGrid) {
        game->GetHUD().updateBaseColor("gridToggle", sf::Color::Black);
    } else {
        game->GetHUD().updateBaseColor("gridToggle", sf::Color(150, 150, 150)); // Gray when grid is off
    }
    
    // Center camera on local player
    sf::Vector2f localPlayerPos = playerManager->GetLocalPlayer().player.GetPosition();
    game->GetCamera().setCenter(localPlayerPos);
    
    // Check for UI hover states using proper coordinate conversion
    const auto& elements = game->GetHUD().getElements();
    sf::Vector2i mousePos = sf::Mouse::getPosition(game->GetWindow());
    sf::Vector2f mouseUIPos = game->WindowToUICoordinates(mousePos);
    
    for (const auto& [id, element] : elements) {
        if (element.hoverable && element.visibleState == GameState::Lobby) {
            // Create a text copy to check bounds
            sf::Text textCopy = element.text;
            textCopy.setPosition(element.pos);
            sf::FloatRect bounds = textCopy.getGlobalBounds();
            
            if (bounds.contains(mouseUIPos)) {
                // Don't override custom colors (like the green ready button)
                if (id != "readyButton" || !isReady) {
                    game->GetHUD().updateBaseColor(id, sf::Color(100, 100, 100)); // Darker when hovered
                }
            } else {
                // Don't override custom colors
                if (id == "startGame") {
                    // Keep the existing color as set above
                } else if (id == "readyButton") {
                    if (isReady) {
                        game->GetHUD().updateBaseColor(id, sf::Color::Green);
                    } else {
                        game->GetHUD().updateBaseColor(id, sf::Color::Black);
                    }
                } else if (id == "gridToggle") {
                    if (showGrid) {
                        game->GetHUD().updateBaseColor(id, sf::Color::Black);
                    } else {
                        game->GetHUD().updateBaseColor(id, sf::Color(150, 150, 150));
                    }
                } else {
                    game->GetHUD().updateBaseColor(id, sf::Color::Black); // Normal color
                }
            }
        }
    }
}
void LobbyState::ProcessEvent(const sf::Event& event) {
    ProcessEvents(event);
}

void LobbyState::UpdateLobbyMembers() {
    if (!game->IsInLobby()) return;
}

bool LobbyState::AllPlayersReady() {
    auto& players = playerManager->GetPlayers();
    
    // If no players, return false
    if (players.empty()) {
        return false;
    }
    
    // Check if all players are ready
    for (const auto& pair : players) {
        if (!pair.second.isReady) {
            return false;
        }
    }
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
    if (params.success) {  // Use the success flag instead of checking direction vector
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
}