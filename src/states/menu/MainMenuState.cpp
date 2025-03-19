#include "MainMenuState.h"
#include <iostream>
#include "../core/Game.h"
#include "../utils/config/Config.h"

MainMenuState::MainMenuState(Game* game) : State(game) {
    // Use fixed dimensions based on BASE_WIDTH and BASE_HEIGHT instead of window size
    float titleCenterX = BASE_WIDTH / 2.0f;
    float centerX = BASE_WIDTH / 2.0f - 300.0f;
    
    // Fixed positions instead of percentages
    float titleY = 50.0f; // Fixed pixel position
    float titleGapAfter = 120.0f; // Fixed pixel distance
    float optionSpacing = 80.0f; // Fixed pixel distance
    float lineWidth = 500.0f; // Fixed line width
    float lineThickness = 2.0f; // Fixed thickness for lines
    
    // Calculate line start X position to center the line
    float lineStartX = centerX - (lineWidth / 2.0f);
    
    // Title - centered at the top with larger font
    game->GetHUD().addElement("title", "Main Menu", 48, 
                             sf::Vector2f(titleCenterX - 120.0f, titleY), 
                             GameState::MainMenu, 
                             HUD::RenderMode::ScreenSpace, false);
    
    // Current Y position tracker for adding elements sequentially
    // Adding fixed gap after title
    float currentY = titleY + titleGapAfter;
    
    
    
    // Update current Y position with fixed spacing
    currentY += optionSpacing * 0.6f;
    
    // Create Lobby option - with connected lines for animation
    game->GetHUD().addElement("createLobby", "Create Lobby", 30, 
                             sf::Vector2f(centerX - 100.0f, currentY), 
                             GameState::MainMenu, 
                             HUD::RenderMode::ScreenSpace, true,
                             "topLine", "middleLine1");
    
    // Update current Y position with fixed spacing
    currentY += optionSpacing;
    
    // Middle separator line 1
    game->GetHUD().addGradientLine("middleLine1", 
                                  lineStartX,
                                  currentY, 
                                  lineWidth, 
                                  lineThickness,
                                  sf::Color::Black, 
                                  GameState::MainMenu,
                                  HUD::RenderMode::ScreenSpace,
                                  30);
    
    // Update current Y position with fixed spacing
    currentY += optionSpacing * 0.6f;
    
    // Search Lobbies option - with connected lines for animation
    game->GetHUD().addElement("searchLobby", "Search for lobby", 30, 
                             sf::Vector2f(centerX - 100.0f, currentY), 
                             GameState::MainMenu, 
                             HUD::RenderMode::ScreenSpace, true,
                             "middleLine1", "middleLine2");
    
    // Update current Y position with fixed spacing
    currentY += optionSpacing;
    
    // Middle separator line 2
    game->GetHUD().addGradientLine("middleLine2", 
                                  lineStartX,
                                  currentY, 
                                  lineWidth, 
                                  lineThickness,
                                  sf::Color::Black, 
                                  GameState::MainMenu,
                                  HUD::RenderMode::ScreenSpace,
                                  30);
    
    // Update current Y position with fixed spacing
    currentY += optionSpacing * 0.6f;
    
    // Settings option - with connected lines for animation
    game->GetHUD().addElement("settings", "Settings", 30, 
                             sf::Vector2f(centerX - 100.0f, currentY), 
                             GameState::MainMenu, 
                             HUD::RenderMode::ScreenSpace, true,
                             "middleLine2", "bottomLine");
    
    // Update current Y position with fixed spacing
    currentY += optionSpacing;
    
    // Bottom separator line
    game->GetHUD().addGradientLine("bottomLine", 
                                  lineStartX,
                                  currentY, 
                                  lineWidth, 
                                  lineThickness,
                                  sf::Color::Black, 
                                  GameState::MainMenu,
                                  HUD::RenderMode::ScreenSpace,
                                  30);
                                  
    // Update current Y position with fixed spacing
    currentY += optionSpacing * 0.6f;
    
    // Exit Game option - with connected lines for animation
    game->GetHUD().addElement("exitGame", "Exit Game", 30, 
                             sf::Vector2f(centerX - 100.0f, currentY), 
                             GameState::MainMenu, 
                             HUD::RenderMode::ScreenSpace, true,
                             "bottomLine", "exitLine");
                             
    // Update current Y position with fixed spacing
    currentY += optionSpacing;
    
    
}

void MainMenuState::Update(float dt) {
    if (!game->IsSteamInitialized()) {
        game->GetHUD().updateText("title", "LOADING...");
    } else {
        game->GetHUD().updateText("title", "Main Menu");
    }
    
    // Update HUD animations - this is critical for the line shake effect
    game->GetHUD().update(game->GetWindow(), GameState::MainMenu, dt);
}

void MainMenuState::Render() {
    // Clear with background color
    game->GetWindow().clear(MAIN_BACKGROUND_COLOR);
    
    // Use UI view for menu elements
    game->GetWindow().setView(game->GetUIView());
    game->GetHUD().render(game->GetWindow(), game->GetUIView(), GameState::MainMenu);
    
    game->GetWindow().display();
}

// Fixed implementation - merged both versions and removed the override keyword
void MainMenuState::ProcessEvent(const sf::Event& event) {
    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::Num1 && game->IsSteamInitialized()) {
            game->SetCurrentState(GameState::LobbyCreation);
            game->GetLobbyNameInput().clear();
        } else if (event.key.code == sf::Keyboard::Num2 && game->IsSteamInitialized()) {
            game->SetCurrentState(GameState::LobbySearch);
        } else if (event.key.code == sf::Keyboard::Num3) {
            game->SetCurrentState(GameState::Settings);
        } else if (event.key.code == sf::Keyboard::Num4) {
            game->GetWindow().close();
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
                if (element.hoverable && element.visibleState == GameState::MainMenu) {
                    // Create a copy of the text to check bounds
                    sf::Text textCopy = element.text;
                    textCopy.setPosition(element.pos);
                    sf::FloatRect bounds = textCopy.getGlobalBounds();
                    
                    if (bounds.contains(mouseUIPos)) {
                        if (id == "createLobby" && game->IsSteamInitialized()) {
                            game->SetCurrentState(GameState::LobbyCreation);
                            game->GetLobbyNameInput().clear();
                        } else if (id == "searchLobby" && game->IsSteamInitialized()) {
                            game->SetCurrentState(GameState::LobbySearch);
                        } else if (id == "settings") {
                            game->SetCurrentState(GameState::Settings);
                        } else if (id == "exitGame") {
                            game->GetWindow().close();
                        }
                    }
                }
            }
        }
    }
}

// Optional implementation of the addSeparatorLine method if you need it
void MainMenuState::addSeparatorLine(Game* game, const std::string& id, float yPos, float windowWidth) {
    // Implementation goes here if needed
}