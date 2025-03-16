#include "LobbySearchState.h"
#include "../../utils/config/Config.h"
#include "../../core/Game.h"
#include <steam/steam_api.h>
#include <iostream>

LobbySearchState::LobbySearchState(Game* game) : State(game) {
    // Use fixed dimensions based on BASE_WIDTH and BASE_HEIGHT
    float centerX = BASE_WIDTH / 2.0f;
    
    // Fixed positions instead of percentages
    float titleY = 50.0f; // Fixed pixel position
    float titleGapAfter = 80.0f; // Fixed pixel gap
    float elementSpacing = 70.0f; // Fixed pixel spacing
    float lineWidth = 600.0f; // Fixed line width
    float lineThickness = 2.0f; // Fixed thickness
    
    // Calculate line start X position to center the line
    float lineStartX = centerX - (lineWidth / 2.0f);
    
    // Title - centered at the top with larger font
    game->GetHUD().addElement("searchTitle", "Search Lobbies", 48, 
                             sf::Vector2f(centerX - 160.0f, titleY), 
                             GameState::LobbySearch, 
                             HUD::RenderMode::ScreenSpace, false);
    
    // Current Y position tracker
    float currentY = titleY + titleGapAfter;
    
    // Top separator line
    game->GetHUD().addGradientLine("searchTopLine", 
                                  lineStartX,
                                  currentY, 
                                  lineWidth, 
                                  lineThickness,
                                  sf::Color::Black, 
                                  GameState::LobbySearch, 
                                  HUD::RenderMode::ScreenSpace,
                                  30);
    
    // Update Y position
    currentY += elementSpacing * 0.5f;
    
    // Status text - shows search status
    game->GetHUD().addElement("searchStatus", "Searching for lobbies...", 22, 
                             sf::Vector2f(centerX - 130.0f, currentY), 
                             GameState::LobbySearch, 
                             HUD::RenderMode::ScreenSpace, false,
                             "searchTopLine", "searchMiddleLine");
    
    // Update Y position
    currentY += elementSpacing * 0.8f;
    
    // Middle separator line
    game->GetHUD().addGradientLine("searchMiddleLine", 
                                  lineStartX,
                                  currentY, 
                                  lineWidth, 
                                  lineThickness,
                                  sf::Color::Black, 
                                  GameState::LobbySearch, 
                                  HUD::RenderMode::ScreenSpace,
                                  30);
    
    // Update Y position
    currentY += elementSpacing * 0.5f;
    
    // Lobby list title
    game->GetHUD().addElement("lobbyListTitle", "Available Lobbies:", 24, 
                             sf::Vector2f(centerX - 100.0f, currentY), 
                             GameState::LobbySearch, 
                             HUD::RenderMode::ScreenSpace, false);
    
    // Update Y position
    currentY += elementSpacing * 0.6f;
    
    // Set up lobby list area with empty list
    float lobbyListY = currentY;
    game->GetHUD().addElement("lobbyList", "No lobbies found.", 20, 
                             sf::Vector2f(centerX - 250.0f, lobbyListY), 
                             GameState::LobbySearch, 
                             HUD::RenderMode::ScreenSpace, false);
    
    // Leave space for lobby entries (dynamic content)
    currentY += elementSpacing * 3.5f;
    
    // Bottom separator line
    game->GetHUD().addGradientLine("searchBottomLine", 
                                  lineStartX,
                                  currentY, 
                                  lineWidth, 
                                  lineThickness,
                                  sf::Color::Black, 
                                  GameState::LobbySearch, 
                                  HUD::RenderMode::ScreenSpace,
                                  30);
    
    // Update Y position
    currentY += elementSpacing * 0.6f;
    
    // Instructions text
    game->GetHUD().addElement("instructions", "Click on a lobby to join | Press ESC to return", 20, 
                             sf::Vector2f(centerX - 200.0f, currentY), 
                             GameState::LobbySearch, 
                             HUD::RenderMode::ScreenSpace, false,
                             "searchBottomLine", "");
    
    // Initialize lobby buttons (will be updated with real data)
    InitializeLobbyButtons(game, centerX, lobbyListY, elementSpacing * 0.35f);
    
    // Refresh button
    currentY += elementSpacing;
    game->GetHUD().addElement("refreshButton", "Refresh", 24, 
                             sf::Vector2f(centerX - 50.0f, currentY), 
                             GameState::LobbySearch, 
                             HUD::RenderMode::ScreenSpace, true,
                             "", "");
    
    // Back button
    currentY += elementSpacing;
    game->GetHUD().addElement("backButton", "Back to Menu", 24, 
                             sf::Vector2f(centerX - 80.0f, currentY), 
                             GameState::LobbySearch, 
                             HUD::RenderMode::ScreenSpace, true,
                             "", "");
    
    // Start the initial lobby search
    SearchLobbies();
}



void LobbySearchState::InitializeLobbyButtons(Game* game, float centerX, float startY, float spacing) {
    // Create placeholder buttons for lobbies that will be populated later
    for (int i = 0; i < 10; i++) {
        std::string buttonId = "lobby" + std::to_string(i);
        float yPos = startY + (i * spacing * 0.35f);
        
        // Create invisible buttons initially
        game->GetHUD().addElement(buttonId, "", 20, 
                                 sf::Vector2f(centerX - 240.0f, yPos), 
                                 GameState::LobbySearch, 
                                 HUD::RenderMode::ScreenSpace, true);
    }
}

void LobbySearchState::Update(float dt) {
    static float searchTimer = 0.f;
    searchTimer += dt;
    
    // Auto-refresh lobby list every 5 seconds
    if (searchTimer >= 5.0f) {
        SearchLobbies();
        searchTimer = 0.f;
    }

    // Check if lobby list has been updated in NetworkManager
    if (game->GetNetworkManager().IsLobbyListUpdated()) {
        UpdateLobbyListDisplay();
        game->GetNetworkManager().ResetLobbyListUpdated();
    }
    
    // Update HUD animations
    game->GetHUD().update(game->GetWindow(), GameState::LobbySearch, dt);
    
    // Update button hover states
    const auto& elements = game->GetHUD().getElements();
    sf::Vector2i mousePos = sf::Mouse::getPosition(game->GetWindow());
    sf::Vector2f mouseUIPos = game->WindowToUICoordinates(mousePos);
    
    for (const auto& [id, element] : elements) {
        if (element.hoverable && element.visibleState == GameState::LobbySearch) {
            // Create a text copy to check bounds
            sf::Text textCopy = element.text;
            textCopy.setPosition(element.pos);
            sf::FloatRect bounds = textCopy.getGlobalBounds();
            
            if (bounds.contains(mouseUIPos)) {
                game->GetHUD().updateBaseColor(id, sf::Color(100, 100, 100)); // Darker when hovered
            } else {
                game->GetHUD().updateBaseColor(id, sf::Color::Black); // Normal color
            }
        }
    }
}

void LobbySearchState::Render() {
    // Clear with background color
    game->GetWindow().clear(MAIN_BACKGROUND_COLOR);
    
    // Use UI view for menu elements
    game->GetWindow().setView(game->GetUIView());
    game->GetHUD().render(game->GetWindow(), game->GetUIView(), GameState::LobbySearch);
    
    game->GetWindow().display();
}

void LobbySearchState::ProcessEvents(const sf::Event& event) {
    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code >= sf::Keyboard::Num0 && event.key.code <= sf::Keyboard::Num9) {
            int index = event.key.code - sf::Keyboard::Num0;
            JoinLobbyByIndex(index);
        } else if (event.key.code == sf::Keyboard::Escape) {
            game->SetCurrentState(GameState::MainMenu);
        } else if (event.key.code == sf::Keyboard::R) {
            // Refresh with R key
            SearchLobbies();
        }
    } else if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        // Get window mouse position
        sf::Vector2i mouseWindowPos(event.mouseButton.x, event.mouseButton.y);
        
        // Convert to UI coordinates
        sf::Vector2f mouseUIPos = game->WindowToUICoordinates(mouseWindowPos);
        
        // Only process clicks within the UI viewport
        if (mouseUIPos.x >= 0 && mouseUIPos.y >= 0) {
            const auto& elements = game->GetHUD().getElements();
            
            for (const auto& [id, element] : elements) {
                if (element.hoverable && element.visibleState == GameState::LobbySearch) {
                    // Create a copy of the text to check bounds
                    sf::Text textCopy = element.text;
                    textCopy.setPosition(element.pos);
                    sf::FloatRect bounds = textCopy.getGlobalBounds();
                    
                    if (bounds.contains(mouseUIPos)) {
                        // Handle lobby buttons (lobby0 through lobby9)
                        if (id.length() > 5 && id.substr(0, 5) == "lobby") {
                            int index = std::stoi(id.substr(5));
                            JoinLobbyByIndex(index);
                        } else if (id == "refreshButton") {
                            SearchLobbies();
                        } else if (id == "backButton") {
                            game->SetCurrentState(GameState::MainMenu);
                        }
                    }
                }
            }
        }
    }
}
void LobbySearchState::ProcessEvent(const sf::Event& event) {
    ProcessEvents(event);
}



void LobbySearchState::SearchLobbies() {
    if (game->IsInLobby()) return;
    
    // Update the status text
    game->GetHUD().updateText("searchStatus", "Searching for lobbies...");
    
    // Clear local list
    lobbyList.clear();
    
    // Request lobbies through Steam API
    SteamMatchmaking()->AddRequestLobbyListStringFilter("game_id", GAME_ID, k_ELobbyComparisonEqual);
    SteamAPICall_t call = SteamMatchmaking()->RequestLobbyList();
    
    if (call == k_uAPICallInvalid) {
        std::cerr << "[ERROR] Failed to request lobby list!" << std::endl;
        game->GetHUD().updateText("searchStatus", "Failed to search lobbies");
    } else {
        std::cout << "[LOBBY] Lobby list request sent\n";
    }
}

void LobbySearchState::UpdateLobbyListDisplay() {
    const auto& networkLobbyList = game->GetNetworkManager().GetLobbyList();
    localLobbyList = networkLobbyList; // Store locally
    
    // Update status text
    if (networkLobbyList.empty()) {
        game->GetHUD().updateText("searchStatus", "No lobbies found");
        game->GetHUD().updateText("lobbyList", "No lobbies available.");
        
        // Hide all lobby buttons
        for (int i = 0; i < 10; i++) {
            std::string buttonId = "lobby" + std::to_string(i);
            game->GetHUD().updateText(buttonId, "");
        }
    } else {
        game->GetHUD().updateText("searchStatus", "Found " + std::to_string(networkLobbyList.size()) + " lobbies");
        game->GetHUD().updateText("lobbyList", ""); // Clear the general lobby list text
        
        // Update each lobby button with actual lobby data
        for (int i = 0; i < 10; i++) {
            std::string buttonId = "lobby" + std::to_string(i);
            if (i < static_cast<int>(networkLobbyList.size())) {
                std::string lobbyName = networkLobbyList[i].second;
                // Add a number prefix and format the lobby name
                std::string displayText = std::to_string(i) + ". " + lobbyName;
                game->GetHUD().updateText(buttonId, displayText);
            } else {
                // Hide buttons for non-existent lobbies
                game->GetHUD().updateText(buttonId, "");
            }
        }
    }
    
    std::cout << "[LOBBY] UI updated with " << networkLobbyList.size() << " lobbies\n";
}

void LobbySearchState::JoinLobby(CSteamID lobby) {
    if (game->IsInLobby()) return;
    
    // Update status
    game->GetHUD().updateText("searchStatus", "Joining lobby...");
    
    // Request to join through Steam API
    SteamAPICall_t call = SteamMatchmaking()->JoinLobby(lobby);
    if (call == k_uAPICallInvalid) {
        std::cerr << "[ERROR] Failed to join lobby!" << std::endl;
        game->GetHUD().updateText("searchStatus", "Failed to join lobby");
        
        // After a delay, go back to main menu on failure
        // In a real implementation, you might want to use a timer here
        game->SetCurrentState(GameState::MainMenu);
    }
}

void LobbySearchState::JoinLobbyByIndex(int index) {
    const auto& networkLobbyList = game->GetNetworkManager().GetLobbyList();
    std::cout << "[LOBBY] Attempting to join lobby at index " << index 
              << ", list size: " << networkLobbyList.size() << "\n";
              
    if (index >= 0 && index < static_cast<int>(networkLobbyList.size())) {
        std::cout << "[LOBBY] Valid index, joining lobby: " << networkLobbyList[index].second 
                  << " (ID: " << networkLobbyList[index].first.ConvertToUint64() << ")\n";
        JoinLobby(networkLobbyList[index].first);
    } else {
        std::cout << "[LOBBY] Invalid lobby index: " << index << ", list size: " << networkLobbyList.size() << "\n";
        game->GetHUD().updateText("searchStatus", "Invalid lobby selection");
    }
}