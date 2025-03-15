#include "LobbyCreationState.h"
#include "../utils/Config.h"
#include <steam/steam_api.h>
#include <iostream>

LobbyCreationState::LobbyCreationState(Game* game) : State(game) {
    // Reset any previous lobby creation state
    game->GetLobbyNameInput().clear();
    
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
    game->GetHUD().addElement("createTitle", "Create Lobby", 48, 
                             sf::Vector2f(centerX - 140.0f, titleY), 
                             GameState::LobbyCreation, 
                             HUD::RenderMode::ScreenSpace, false);
    
    // Current Y position tracker
    float currentY = titleY + titleGapAfter;
    
    // Top separator line
    game->GetHUD().addGradientLine("createTopLine", 
                                  lineStartX,
                                  currentY, 
                                  lineWidth, 
                                  lineThickness,
                                  sf::Color::Black, 
                                  GameState::LobbyCreation, 
                                  HUD::RenderMode::ScreenSpace,
                                  30);
    
    // Update Y position
    currentY += elementSpacing * 0.6f;
    
    // Instructions text
    game->GetHUD().addElement("instructions", "Enter a name for your lobby:", 24, 
                             sf::Vector2f(centerX - 170.0f, currentY), 
                             GameState::LobbyCreation, 
                             HUD::RenderMode::ScreenSpace, false,
                             "createTopLine", "createMiddleLine");
    
    // Update Y position
    currentY += elementSpacing;
    
    // Middle separator line
    game->GetHUD().addGradientLine("createMiddleLine", 
                                  lineStartX,
                                  currentY, 
                                  lineWidth, 
                                  lineThickness,
                                  sf::Color::Black, 
                                  GameState::LobbyCreation, 
                                  HUD::RenderMode::ScreenSpace,
                                  30);
    
    // Update Y position
    currentY += elementSpacing * 0.6f;
    
    // Input field (text will be updated in real-time)
    game->GetHUD().addElement("inputField", "> " + game->GetLobbyNameInput(), 30, 
                             sf::Vector2f(centerX - 150.0f, currentY), 
                             GameState::LobbyCreation, 
                             HUD::RenderMode::ScreenSpace, false,
                             "createMiddleLine", "createBottomLine");
    
    // Update Y position
    currentY += elementSpacing;
    
    // Bottom separator line
    game->GetHUD().addGradientLine("createBottomLine", 
                                  lineStartX,
                                  currentY, 
                                  lineWidth, 
                                  lineThickness,
                                  sf::Color::Black, 
                                  GameState::LobbyCreation, 
                                  HUD::RenderMode::ScreenSpace,
                                  30);
    
    // Update Y position
    currentY += elementSpacing * 0.6f;
    
    // Controls information
    game->GetHUD().addElement("controls", "Press Enter to create | Esc to cancel", 20, 
                             sf::Vector2f(centerX - 180.0f, currentY), 
                             GameState::LobbyCreation, 
                             HUD::RenderMode::ScreenSpace, false,
                             "createBottomLine", "");
    
    // Add buttons as interactive elements
    currentY += elementSpacing * 1.2f;
    
    // Create button
    game->GetHUD().addElement("createButton", "Create", 24, 
                             sf::Vector2f(centerX - 120.0f, currentY), 
                             GameState::LobbyCreation, 
                             HUD::RenderMode::ScreenSpace, true,
                             "", "");
    
    // Cancel button
    game->GetHUD().addElement("cancelButton", "Cancel", 24, 
                             sf::Vector2f(centerX + 70.0f, currentY), 
                             GameState::LobbyCreation, 
                             HUD::RenderMode::ScreenSpace, true,
                             "", "");
    
    // Initialize creation state flag
    m_creationInProgress = false;
}

void LobbyCreationState::Update(float dt) {
    // Update the input field text with current input
    game->GetHUD().updateText("inputField", "> " + game->GetLobbyNameInput());
    
    // Update HUD animations
    game->GetHUD().update(game->GetWindow(), GameState::LobbyCreation, dt);
    
    // Check for button hover states
    const auto& elements = game->GetHUD().getElements();
    sf::Vector2i mousePos = sf::Mouse::getPosition(game->GetWindow());
    sf::Vector2f mouseUIPos = game->WindowToUICoordinates(mousePos);
    
    for (const auto& [id, element] : elements) {
        if (element.hoverable && element.visibleState == GameState::LobbyCreation) {
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

void LobbyCreationState::Render() {
    // Clear with background color
    game->GetWindow().clear(MAIN_BACKGROUND_COLOR);
    
    // Use UI view for menu elements
    game->GetWindow().setView(game->GetUIView());
    game->GetHUD().render(game->GetWindow(), game->GetUIView(), GameState::LobbyCreation);
    
    game->GetWindow().display();
}

void LobbyCreationState::ProcessEvents(const sf::Event& event) {
    // Ignore input events if lobby creation is already in progress
    if (m_creationInProgress) {
        return;
    }
    
    if (event.type == sf::Event::TextEntered) {
        if (event.text.unicode == '\r' || event.text.unicode == '\n') {
            if (!game->GetLobbyNameInput().empty()) {
                CreateLobby(game->GetLobbyNameInput());
            }
        } else if (event.text.unicode == '\b' && !game->GetLobbyNameInput().empty()) {
            game->GetLobbyNameInput().pop_back();
            game->GetHUD().updateText("inputField", "> " + game->GetLobbyNameInput());
        } else if (event.text.unicode >= 32 && event.text.unicode < 128) {
            game->GetLobbyNameInput() += static_cast<char>(event.text.unicode);
            game->GetHUD().updateText("inputField", "> " + game->GetLobbyNameInput());
        }
    } else if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
        game->SetCurrentState(GameState::MainMenu);
    } else if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        // Get window mouse position
        sf::Vector2i mouseWindowPos(event.mouseButton.x, event.mouseButton.y);
        
        // Convert to UI coordinates
        sf::Vector2f mouseUIPos = game->WindowToUICoordinates(mouseWindowPos);
        
        // Only process clicks within the UI viewport
        if (mouseUIPos.x >= 0 && mouseUIPos.y >= 0) {
            const auto& elements = game->GetHUD().getElements();
            
            for (const auto& [id, element] : elements) {
                if (element.hoverable && element.visibleState == GameState::LobbyCreation) {
                    // Create a copy of the text to check bounds
                    sf::Text textCopy = element.text;
                    textCopy.setPosition(element.pos);
                    sf::FloatRect bounds = textCopy.getGlobalBounds();
                    
                    if (bounds.contains(mouseUIPos)) {
                        if (id == "createButton" && !game->GetLobbyNameInput().empty()) {
                            CreateLobby(game->GetLobbyNameInput());
                        } else if (id == "cancelButton") {
                            game->SetCurrentState(GameState::MainMenu);
                        }
                    }
                }
            }
        }
    }
}

void LobbyCreationState::ProcessEvent(const sf::Event& event) {
    ProcessEvents(event);
}

void LobbyCreationState::Enter() {
    // Reset the creation state when entering this state
    m_creationInProgress = false;
    game->GetLobbyNameInput().clear();
    game->GetHUD().updateText("inputField", "> " + game->GetLobbyNameInput());
}


void LobbyCreationState::CreateLobby(const std::string& lobbyName) {
    // Prevent multiple creation attempts
    if (m_creationInProgress) {
        std::cout << "[LOBBY] Lobby creation already in progress, ignoring request.\n";
        return;
    }
    
    // Make sure we're not already in a lobby
    if (game->IsInLobby()) {
        std::cout << "[LOBBY] Already in a lobby, ignoring request.\n";
        return;
    }
    
    std::cout << "[LOBBY] Lobby creation requested.\n";

    // Set the flag to prevent multiple creation attempts
    m_creationInProgress = true;

    // Update the lobby name input with the provided lobby name
    game->GetLobbyNameInput() = lobbyName;

    // Call Steam API to create a public lobby with up to 10 members
    SteamAPICall_t call = SteamMatchmaking()->CreateLobby(k_ELobbyTypePublic, 10);
    if (call == k_uAPICallInvalid) {
        std::cout << "[LOBBY] CreateLobby call failed immediately.\n";
        m_creationInProgress = false; // Reset the flag
        game->SetCurrentState(GameState::MainMenu);
    } else {
        std::cout << "[LOBBY] Lobby creation requested.\n";
        // The NetworkManager will handle the rest via its OnLobbyCreated callback
    }
}

void LobbyCreationState::Exit() {
    // Clean up any pending operations when leaving this state
    if (m_creationInProgress) {
        std::cout << "[LOBBY] Exiting LobbyCreationState while creation was in progress.\n";
        m_creationInProgress = false; // Reset the flag
    }
}

// Remove or comment out onLobbyEnter method entirely!
// We no longer need it as NetworkManager handles everything

/*
void LobbyCreationState::onLobbyEnter(CSteamID lobbyID) {
    // This method should be removed
}
*/



// Remove or comment out the onLobbyEnter method in LobbyCreationState - we won't use it anymore
// The NetworkManager's OnLobbyEnter will handle everything

//---------------------------------------------------------
// onLobbyEnter: Handle actions when entering a newly created lobby
//---------------------------------------------------------
/*void LobbyCreationState::onLobbyEnter(CSteamID lobbyID) {
    // Reset the creation flag
    m_creationInProgress = false;
    
    // Make sure we're still in LobbyCreation state
    if (game->GetCurrentState() != GameState::LobbyCreation) {
        std::cout << "[LOBBY] State changed during lobby creation, aborting lobby entry.\n";
        return;
    }
    
    if (!SteamMatchmaking()) {
        std::cerr << "[ERROR] SteamMatchmaking interface not available!" << std::endl;
        game->SetCurrentState(GameState::MainMenu);
        return;
    }

    // Transition to the Lobby state
    game->SetCurrentState(GameState::Lobby);
    std::cout << "on lobby enter" << std::endl;

    // Set lobby data
    SteamMatchmaking()->SetLobbyData(lobbyID, "name", game->GetLobbyNameInput().c_str());
    SteamMatchmaking()->SetLobbyData(lobbyID, "game_id", GAME_ID);
    CSteamID myID = SteamUser()->GetSteamID();
    std::string hostStr = std::to_string(myID.ConvertToUint64());
    SteamMatchmaking()->SetLobbyData(lobbyID, "host_steam_id", hostStr.c_str());
    SteamMatchmaking()->SetLobbyJoinable(lobbyID, true);

    // Add the local player to the NetworkManager's connected clients
    game->GetNetworkManager().AcceptSession(myID);

    // Log entry for debugging
    std::cout << "[LOBBY] Created and entered lobby " << lobbyID.ConvertToUint64() 
              << " as host" << std::endl;
}*/

// Handle failure of lobby creation
void LobbyCreationState::onLobbyCreationFailed() {
    std::cout << "[LOBBY] Lobby creation failed.\n";
    m_creationInProgress = false;
    game->SetCurrentState(GameState::MainMenu);
}
