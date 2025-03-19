#include "LobbyCreationState.h"
#include "../../utils/config/Config.h"
#include <steam/steam_api.h>
#include <iostream>

LobbyCreationState::LobbyCreationState(Game* game) : State(game) {
    float centerX = BASE_WIDTH / 2.0f;
    float titleY = 50.0f;
    
    game->GetHUD().addElement("title", "Create Lobby", 36, 
                             sf::Vector2f(centerX - 100.0f, titleY), 
                             GameState::LobbyCreation, 
                             HUD::RenderMode::ScreenSpace, false);
    
    float inputY = titleY + 100.0f;
    game->GetHUD().addElement("nameLabel", "Lobby Name:", 24, 
                             sf::Vector2f(centerX - 200.0f, inputY), 
                             GameState::LobbyCreation, 
                             HUD::RenderMode::ScreenSpace, false);
    
    game->GetHUD().addElement("nameInput", "< Enter Lobby Name >", 24, 
                             sf::Vector2f(centerX, inputY), 
                             GameState::LobbyCreation, 
                             HUD::RenderMode::ScreenSpace, false);
    
    float buttonY = inputY + 80.0f;
    game->GetHUD().addElement("createLobbyButton", "Create Lobby", 24, 
                             sf::Vector2f(centerX - 60.0f, buttonY), 
                             GameState::LobbyCreation, 
                             HUD::RenderMode::ScreenSpace, true);
    
    float backY = buttonY + 60.0f;
    game->GetHUD().addElement("backButton", "Back", 24, 
                             sf::Vector2f(centerX - 30.0f, backY), 
                             GameState::LobbyCreation, 
                             HUD::RenderMode::ScreenSpace, true);
    
    game->GetHUD().addElement("statusText", "", 18, 
                             sf::Vector2f(centerX - 150.0f, backY + 60.0f), 
                             GameState::LobbyCreation, 
                             HUD::RenderMode::ScreenSpace, false);
    
    isInputActive = true;
    messageTimer = 0.0f;
    previousStatusText = "";
    m_creationInProgress = false;
    retryTimer = 0.0f;
    retryCount = 0;
}

void LobbyCreationState::Update(float dt) {
    // Handle retry timer
    if (retryTimer > 0) {
        retryTimer -= dt;
        if (retryTimer <= 0 && retryCount < maxRetries) {
            CreateLobby();
        }
    }

    // Update message timer
    if (messageTimer > 0) {
        messageTimer -= dt;
        if (messageTimer <= 0) {
            game->GetHUD().updateText("statusText", previousStatusText);
        }
    }
    
    if (!game->IsSteamInitialized()) {
        game->GetHUD().updateText("createLobbyButton", "Waiting for Steam...");
        game->GetHUD().updateBaseColor("createLobbyButton", sf::Color(150, 150, 150));
    } else {
        game->GetHUD().updateText("createLobbyButton", "Create Lobby");
        game->GetHUD().updateBaseColor("createLobbyButton", sf::Color::White);
    }
    
    if (isInputActive) {
        game->GetHUD().updateText("nameInput", game->GetLobbyNameInput() + "_");
    } else {
        game->GetHUD().updateText("nameInput", game->GetLobbyNameInput());
    }
    
    game->GetHUD().update(game->GetWindow(), GameState::LobbyCreation, dt);
}

void LobbyCreationState::Render() {
    game->GetWindow().clear(MAIN_BACKGROUND_COLOR);
    game->GetWindow().setView(game->GetUIView());
    game->GetHUD().render(game->GetWindow(), game->GetUIView(), GameState::LobbyCreation);
    game->GetWindow().display();
}

void LobbyCreationState::ProcessEvent(const sf::Event& event) {
    bool steamReady = game->IsSteamInitialized();
    
    if (event.type == sf::Event::KeyPressed) {
        if ((event.key.code == sf::Keyboard::Return || event.key.code == sf::Keyboard::Enter) && !m_creationInProgress) {
            if (!steamReady) {
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
        else if (event.key.code == sf::Keyboard::Escape) {
            game->SetCurrentState(GameState::MainMenu);
        }
        else if (event.key.code == sf::Keyboard::BackSpace && isInputActive) {
            if (!game->GetLobbyNameInput().empty()) {
                std::string currentInput = game->GetLobbyNameInput();
                currentInput.pop_back();
                game->GetLobbyNameInput() = currentInput;
            }
        }
    }
    else if (event.type == sf::Event::TextEntered && isInputActive) {
        if (event.text.unicode < 128 && event.text.unicode != '\b' && event.text.unicode != '\r' && event.text.unicode != '\n') {
            if (game->GetLobbyNameInput().size() < 20) {
                game->GetLobbyNameInput() += static_cast<char>(event.text.unicode);
            }
        }
    }
    else if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left && !m_creationInProgress) {
        sf::Vector2i mouseWindowPos(event.mouseButton.x, event.mouseButton.y);
        sf::Vector2f mouseUIPos = game->WindowToUICoordinates(mouseWindowPos);
        
        if (mouseUIPos.x >= 0 && mouseUIPos.y >= 0) {
            const auto& elements = game->GetHUD().getElements();
            
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
    if (!game->IsSteamInitialized() || !SteamMatchmaking()) {
        game->GetHUD().updateText("statusText", "Steam is not ready. Please wait...");
        messageTimer = 3.0f;
        return;
    }
    
    if (game->GetLobbyNameInput().empty()) {
        game->GetHUD().updateText("statusText", "Please enter a lobby name!");
        messageTimer = 3.0f;
        return;
    }
    
    m_creationInProgress = true;
    SteamAPICall_t call = SteamMatchmaking()->CreateLobby(k_ELobbyTypePublic, 4);
    
    if (call == k_uAPICallInvalid) {
        game->GetHUD().updateText("statusText", "Failed to create lobby: Invalid API call");
        m_creationInProgress = false;
        return;
    }
    
    game->GetHUD().updateText("statusText", "Creating lobby...");
}

void LobbyCreationState::Enter() {
    // Optional: Reset state variables when entering
    retryTimer = 0.0f;
    retryCount = 0;
    m_creationInProgress = false;
}

void LobbyCreationState::Exit() {
    isInputActive = false;
    retryTimer = 0.0f;
    retryCount = 0;
    m_creationInProgress = false;
}

// Unused function in your current implementation
void LobbyCreationState::ProcessEvents(const sf::Event& event) {}