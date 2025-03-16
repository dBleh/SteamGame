#include "LobbyCreationState.h"
#include "../../utils/config/Config.h"
#include <steam/steam_api.h>
#include <iostream>

// LobbyCreationState constructor
LobbyCreationState::LobbyCreationState(Game* game) : State(game) {
    // Set up UI elements
    float centerX = BASE_WIDTH / 2.0f;
    float titleY = 50.0f;
    
    // Title
    game->GetHUD().addElement("title", "Create Lobby", 36, 
                             sf::Vector2f(centerX - 100.0f, titleY), 
                             GameState::LobbyCreation, 
                             HUD::RenderMode::ScreenSpace, false);
    
    // Name input field
    float inputY = titleY + 100.0f;
    game->GetHUD().addElement("nameLabel", "Lobby Name:", 24, 
                             sf::Vector2f(centerX - 200.0f, inputY), 
                             GameState::LobbyCreation, 
                             HUD::RenderMode::ScreenSpace, false);
    
    game->GetHUD().addElement("nameInput", "< Enter Lobby Name >", 24, 
                             sf::Vector2f(centerX, inputY), 
                             GameState::LobbyCreation, 
                             HUD::RenderMode::ScreenSpace, false);
    
    // Create button
    float buttonY = inputY + 80.0f;
    game->GetHUD().addElement("createLobbyButton", "Create Lobby", 24, 
                             sf::Vector2f(centerX - 60.0f, buttonY), 
                             GameState::LobbyCreation, 
                             HUD::RenderMode::ScreenSpace, true);
    
    // Back button
    float backY = buttonY + 60.0f;
    game->GetHUD().addElement("backButton", "Back", 24, 
                             sf::Vector2f(centerX - 30.0f, backY), 
                             GameState::LobbyCreation, 
                             HUD::RenderMode::ScreenSpace, true);
    
    // Status text for feedback messages
    game->GetHUD().addElement("statusText", "", 18, 
                             sf::Vector2f(centerX - 150.0f, backY + 60.0f), 
                             GameState::LobbyCreation, 
                             HUD::RenderMode::ScreenSpace, false);
    
    // Set the initial focus on the name input
    isInputActive = true;
    
    // Initialize timer
    messageTimer = 0.0f;
    previousStatusText = "";
}

void LobbyCreationState::Update(float dt) {
    // Update message timer
    if (messageTimer > 0) {
        messageTimer -= dt;
        if (messageTimer <= 0) {
            game->GetHUD().updateText("statusText", previousStatusText);
        }
    }
    
    // Update UI based on Steam initialization status
    if (!game->IsSteamInitialized()) {
        game->GetHUD().updateText("createLobbyButton", "Waiting for Steam...");
        game->GetHUD().updateBaseColor("createLobbyButton", sf::Color(150, 150, 150)); // Grayed out
    } else {
        game->GetHUD().updateText("createLobbyButton", "Create Lobby");
        game->GetHUD().updateBaseColor("createLobbyButton", sf::Color::White); // Normal color
    }
    
    // Update input field
    if (isInputActive) {
        game->GetHUD().updateText("nameInput", game->GetLobbyNameInput() + "_");
    } else {
        game->GetHUD().updateText("nameInput", game->GetLobbyNameInput());
    }
    
    // Update other HUD elements
    game->GetHUD().update(game->GetWindow(), GameState::LobbyCreation, dt);
}

void LobbyCreationState::Render() {
    // Clear with background color
    game->GetWindow().clear(MAIN_BACKGROUND_COLOR);
    
    // Use UI view
    game->GetWindow().setView(game->GetUIView());
    game->GetHUD().render(game->GetWindow(), game->GetUIView(), GameState::LobbyCreation);
    
    game->GetWindow().display();
}

void LobbyCreationState::ProcessEvent(const sf::Event& event) {
    // Check if Steam is initialized
    bool steamReady = game->IsSteamInitialized();
    
    // Handle keyboard events
    if (event.type == sf::Event::KeyPressed) {
        // Handle Enter key press - create lobby if Steam is ready
        if (event.key.code == sf::Keyboard::Return || event.key.code == sf::Keyboard::Enter) {
            if (!steamReady) {
                // Show message that Steam isn't ready
                previousStatusText = game->GetHUD().getElements().count("statusText") ? 
                                    game->GetHUD().getElements().at("statusText").text.getString() : "";
                game->GetHUD().updateText("statusText", "Steam is still initializing. Please wait...");
                messageTimer = 3.0f;
                return;
            }
            
            if (isInputActive && !game->GetLobbyNameInput().empty()) {
                CreateLobby();
            }
        }
        // Handle escape key - return to main menu
        else if (event.key.code == sf::Keyboard::Escape) {
            game->SetCurrentState(GameState::MainMenu);
        }
        // Handle backspace
        else if (event.key.code == sf::Keyboard::BackSpace && isInputActive) {
            if (!game->GetLobbyNameInput().empty()) {
                std::string currentInput = game->GetLobbyNameInput();
                currentInput.pop_back();
                game->GetLobbyNameInput() = currentInput;
            }
        }
    }
    // Handle text entered
    else if (event.type == sf::Event::TextEntered && isInputActive) {
        if (event.text.unicode < 128 && event.text.unicode != '\b' && event.text.unicode != '\r' && event.text.unicode != '\n') {
            if (game->GetLobbyNameInput().size() < 20) { // Limit name length
                game->GetLobbyNameInput() += static_cast<char>(event.text.unicode);
            }
        }
    }
    // Handle mouse click
    else if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2i mouseWindowPos(event.mouseButton.x, event.mouseButton.y);
        sf::Vector2f mouseUIPos = game->WindowToUICoordinates(mouseWindowPos);
        
        if (mouseUIPos.x >= 0 && mouseUIPos.y >= 0) {
            const auto& elements = game->GetHUD().getElements();
            
            // Check if clicked on name input
            if (elements.count("nameInput")) {
                sf::Text textCopy = elements.at("nameInput").text;
                textCopy.setPosition(elements.at("nameInput").pos);
                sf::FloatRect nameBounds = textCopy.getGlobalBounds();
                
                if (nameBounds.contains(mouseUIPos)) {
                    isInputActive = true;
                } else {
                    isInputActive = false;
                }
            }
            
            // Check if clicked on create button
            if (elements.count("createLobbyButton")) {
                sf::Text createButtonCopy = elements.at("createLobbyButton").text;
                createButtonCopy.setPosition(elements.at("createLobbyButton").pos);
                sf::FloatRect createBounds = createButtonCopy.getGlobalBounds();
                
                if (createBounds.contains(mouseUIPos)) {
                    if (steamReady) {
                        if (!game->GetLobbyNameInput().empty()) {
                            CreateLobby();
                        } else {
                            previousStatusText = elements.count("statusText") ? 
                                               elements.at("statusText").text.getString() : "";
                            game->GetHUD().updateText("statusText", "Please enter a lobby name!");
                            messageTimer = 3.0f;
                        }
                    } else {
                        previousStatusText = elements.count("statusText") ? 
                                           elements.at("statusText").text.getString() : "";
                        game->GetHUD().updateText("statusText", "Steam is still initializing. Please wait...");
                        messageTimer = 3.0f;
                    }
                }
            }
            
            // Check if clicked on back button
            if (elements.count("backButton")) {
                sf::Text backButtonCopy = elements.at("backButton").text;
                backButtonCopy.setPosition(elements.at("backButton").pos);
                sf::FloatRect backBounds = backButtonCopy.getGlobalBounds();
                
                if (backBounds.contains(mouseUIPos)) {
                    game->SetCurrentState(GameState::MainMenu);
                }
            }
        }
    }
}

void LobbyCreationState::CreateLobby() {
    // Double-check Steam is initialized
    if (!game->IsSteamInitialized()) {
        game->GetHUD().updateText("statusText", "Steam is not initialized yet!");
        return;
    }
    
    // Verify lobby name isn't empty
    if (game->GetLobbyNameInput().empty()) {
        game->GetHUD().updateText("statusText", "Please enter a lobby name!");
        return;
    }
    
    // Create the lobby
    SteamAPICall_t call = SteamMatchmaking()->CreateLobby(k_ELobbyTypePublic, 4);
    
    // Check if call succeeded
    if (call == k_uAPICallInvalid) {
        game->GetHUD().updateText("statusText", "Failed to create lobby: Invalid API call");
        return;
    }
    
    // Show creating status
    game->GetHUD().updateText("statusText", "Creating lobby...");
}

void LobbyCreationState::Exit() {
    // Reset input active state when leaving this state
    isInputActive = false;
}