#include "PlayingStateUI.h"
#include "../core/Game.h"
#include "../utils/config/Config.h"
#include "../entities/PlayerManager.h"
#include "../entities/enemies/EnemyManager.h"
#include "../network/messages/MessageHandler.h"
#include "../network/messages/PlayerMessageHandler.h"
#include "../network/messages/EnemyMessageHandler.h"
#include "../network/messages/StateMessageHandler.h"
#include "../network/messages/SystemMessageHandler.h"
#include <iostream>
#include <algorithm>
#include <cmath>

PlayingStateUI::PlayingStateUI(Game* game, PlayerManager* playerManager, EnemyManager* enemyManager)
    : m_game(game), m_playerManager(playerManager), m_enemyManager(enemyManager),
      m_continueHovered(false), m_returnHovered(false), m_isHost(false) {
    
    InitializeUI();
    
    // Check if we're the host
    CSteamID myID = SteamUser()->GetSteamID();
    CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(m_game->GetLobbyID());
    m_isHost = (myID == hostID);
}

PlayingStateUI::~PlayingStateUI() {
}

void PlayingStateUI::InitializeUI() {
    // Use fixed dimensions based on BASE_WIDTH and BASE_HEIGHT
    float centerX = BASE_WIDTH / 2.0f;
    
    // Fixed positions instead of percentages
    float topBarY = 40.0f; // Fixed pixel position
    float bottomBarY = BASE_HEIGHT - 70.0f; // Fixed position from bottom
    float lineWidth = 1000.0f; // Fixed line width
    float lineThickness = 2.0f; // Fixed thickness
    
    // Calculate line start X position to center the line
    float lineStartX = centerX - (lineWidth / 2.0f);
    
    HUD& hud = m_game->GetHUD();
    
    // ===== TOP BAR UI =====
    // Top separator line
    hud.addGradientLine("topBarLine", 
                        lineStartX,
                        topBarY, 
                        lineWidth, 
                        lineThickness,
                        sf::Color::Black, 
                        GameState::Playing, 
                        HUD::RenderMode::ScreenSpace,
                        30);
    
    // Game header (centered with wave info)
    hud.addElement("gameHeader", "WAVE 1", 32, 
                  sf::Vector2f(centerX - 60.0f, topBarY - 40.0f), 
                  GameState::Playing, 
                  HUD::RenderMode::ScreenSpace, false,
                  "", "topBarLine");
    hud.updateBaseColor("gameHeader", sf::Color::Black);
    
    // Player stats (left aligned)
    hud.addElement("playerStats", "HP: 100 | Kills: 0 | Money: 0", 18, 
                  sf::Vector2f(30.0f, topBarY + 15.0f), 
                  GameState::Playing, 
                  HUD::RenderMode::ScreenSpace, false);
    hud.updateBaseColor("playerStats", sf::Color::Black);
    
    // Wave info (right aligned)
    hud.addElement("waveInfo", "Enemies: 0", 18, 
                  sf::Vector2f(BASE_WIDTH - 280.0f, topBarY + 15.0f), 
                  GameState::Playing, 
                  HUD::RenderMode::ScreenSpace, false);
    hud.updateBaseColor("waveInfo", sf::Color::Black);
    
    // Loading message (centered, temporary)
    hud.addElement("playerLoading", "Loading game...", 24, 
                  sf::Vector2f(centerX - 80.0f, BASE_HEIGHT / 2.0f), 
                  GameState::Playing, 
                  HUD::RenderMode::ScreenSpace, false);
    hud.updateBaseColor("playerLoading", sf::Color::Black);
    
    // ===== BOTTOM BAR UI =====
    // Bottom separator line
    hud.addGradientLine("bottomBarLine", 
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
    hud.addElement("tabHint", "TAB - Show Leaderboard", 16, 
                  sf::Vector2f(30.0f, controlsY), 
                  GameState::Playing, 
                  HUD::RenderMode::ScreenSpace, false,
                  "bottomBarLine", "");
    hud.updateBaseColor("tabHint", sf::Color::Black);
    
    // Grid toggle
    hud.addElement("gridToggle", "G - Toggle Grid", 16, 
                  sf::Vector2f(30.0f + spacing, controlsY), 
                  GameState::Playing, 
                  HUD::RenderMode::ScreenSpace, true,
                  "bottomBarLine", "");
    hud.updateBaseColor("gridToggle", sf::Color::Black);
    
    // Death timer text (initially hidden)
    hud.addElement("deathTimer", "", 36, 
                  sf::Vector2f(centerX - 100.0f, BASE_HEIGHT / 2.0f - 50.0f), 
                  GameState::Playing, 
                  HUD::RenderMode::ScreenSpace, false);
    hud.updateBaseColor("deathTimer", sf::Color::Red);
    
    // Cursor lock toggle
    hud.addElement("cursorLockHint", "L - Toggle Cursor Lock", 16, 
                  sf::Vector2f(30.0f + spacing * 2, controlsY), 
                  GameState::Playing, 
                  HUD::RenderMode::ScreenSpace, true,
                  "bottomBarLine", "");
    hud.updateBaseColor("cursorLockHint", sf::Color::Black);
    
    // Escape menu hint
    hud.addElement("escHint", "ESC - Menu", 16, 
                  sf::Vector2f(30.0f + spacing * 3, controlsY), 
                  GameState::Playing, 
                  HUD::RenderMode::ScreenSpace, false,
                  "bottomBarLine", "");
    hud.updateBaseColor("escHint", sf::Color::Black);
    
    // Shop hint
    hud.addElement("shopHint", "B - Open Shop", 16, 
                  sf::Vector2f(30.0f + spacing * 4, controlsY), 
                  GameState::Playing, 
                  HUD::RenderMode::ScreenSpace, true,
                  "bottomBarLine", "");
    hud.updateBaseColor("shopHint", sf::Color::Black);

    // Force field hint
    hud.addElement("forceFieldHint", "F - Toggle Force Field", 16, 
        sf::Vector2f(30.0f + spacing * 5, controlsY), 
        GameState::Playing, 
        HUD::RenderMode::ScreenSpace, true,
        "bottomBarLine", "");
    hud.updateBaseColor("forceFieldHint", sf::Color::Black);

    // ===== LEADERBOARD (initially hidden) =====
    hud.addElement("leaderboard", "", 20, 
                  sf::Vector2f(centerX - 180.0f, BASE_HEIGHT * 0.25f), 
                  GameState::Playing, 
                  HUD::RenderMode::ScreenSpace, false);
    hud.updateBaseColor("leaderboard", sf::Color::White);
    
    // ===== ESCAPE MENU SETUP =====
    // Background panel with modern styling
    m_menuBackground.setSize(sf::Vector2f(400.f, 300.f));
    m_menuBackground.setFillColor(sf::Color(40, 40, 40, 230)); // Dark, semi-transparent
    m_menuBackground.setOutlineColor(sf::Color(100, 100, 255, 150)); // Subtle blue glow
    m_menuBackground.setOutlineThickness(2.f);

    // Menu title
    m_menuTitle.setFont(m_game->GetFont());
    m_menuTitle.setString("Game Paused");
    m_menuTitle.setCharacterSize(28);
    m_menuTitle.setFillColor(sf::Color(220, 220, 220));
    m_menuTitle.setStyle(sf::Text::Bold);

    // Return button with modern styling
    m_returnButton.setSize(sf::Vector2f(220.f, 50.f));
    m_returnButton.setFillColor(sf::Color(60, 60, 60, 230));
    m_returnButton.setOutlineColor(sf::Color(120, 120, 200));
    m_returnButton.setOutlineThickness(1.5f);

    m_returnButtonText.setFont(m_game->GetFont());
    m_returnButtonText.setString("Return to Main Menu");
    m_returnButtonText.setCharacterSize(20);
    m_returnButtonText.setFillColor(sf::Color(220, 220, 220));
    
    // Return to lobby button (only for host)
    m_returnToLobbyButton.setSize(sf::Vector2f(220.f, 50.f));
    m_returnToLobbyButton.setFillColor(sf::Color(60, 60, 60, 230));
    m_returnToLobbyButton.setOutlineColor(sf::Color(200, 120, 120));
    m_returnToLobbyButton.setOutlineThickness(1.5f);

    m_returnToLobbyButtonText.setFont(m_game->GetFont());
    m_returnToLobbyButtonText.setString("Return All to Lobby");
    m_returnToLobbyButtonText.setCharacterSize(20);
    m_returnToLobbyButtonText.setFillColor(sf::Color(220, 220, 220));

    // Continue button
    m_continueButton.setSize(sf::Vector2f(220.f, 50.f));
    m_continueButton.setFillColor(sf::Color(60, 60, 60, 230));
    m_continueButton.setOutlineColor(sf::Color(120, 200, 120));
    m_continueButton.setOutlineThickness(1.5f);

    m_continueButtonText.setFont(m_game->GetFont());
    m_continueButtonText.setString("Continue Playing");
    m_continueButtonText.setCharacterSize(20);
    m_continueButtonText.setFillColor(sf::Color(220, 220, 220));
    
    // Position the escape menu elements
    PositionEscapeMenuElements();
}

void PlayingStateUI::Update(float dt) {
    // Get mouse position in window coordinates
    sf::Vector2i mousePos = sf::Mouse::getPosition(m_game->GetWindow());
    sf::Vector2f mouseUIPos = m_game->WindowToUICoordinates(mousePos);
    
    // Update HUD animations
    m_game->GetHUD().update(m_game->GetWindow(), GameState::Playing, dt);
    
    // Update UI element colors based on hover/active state
    if (mouseUIPos.x >= 0 && mouseUIPos.y >= 0) {
        const auto& elements = m_game->GetHUD().getElements();
        
        for (const auto& [id, element] : elements) {
            if (element.hoverable && element.visibleState == GameState::Playing) {
                // Create a text copy to check bounds
                sf::Text textCopy = element.text;
                textCopy.setPosition(element.pos);
                sf::FloatRect bounds = textCopy.getGlobalBounds();
                
                if (bounds.contains(mouseUIPos)) {
                    // Hovering over element
                    m_game->GetHUD().updateBaseColor(id, sf::Color(100, 100, 100)); // Darker when hovered
                } else {
                    // Not hovering - set appropriate color based on state
                    if (id == "gridToggle") {
                        bool showGrid = true; // This would need to be passed in 
                        m_game->GetHUD().updateBaseColor(id, showGrid ? sf::Color::Black : sf::Color(150, 150, 150));
                    } else if (id == "cursorLockHint") {
                        bool cursorLocked = true; // This would need to be passed in
                        m_game->GetHUD().updateBaseColor(id, cursorLocked ? sf::Color::Black : sf::Color(150, 150, 150));
                    } else {
                        m_game->GetHUD().updateBaseColor(id, sf::Color::Black);
                    }
                }
            }
        }
    }
}

void PlayingStateUI::UpdatePlayerStats() {
    if (!m_playerManager) return;
    
    // Get the local player
    const auto& localPlayer = m_playerManager->GetLocalPlayer();
    
    // Format stats string
    std::string statsText = "HP: " + std::to_string(localPlayer.player.GetHealth()) + 
                          " | Kills: " + std::to_string(localPlayer.kills) + 
                          " | Money: " + std::to_string(localPlayer.money);
    
    // Update the HUD
    m_game->GetHUD().updateText("playerStats", statsText);
    
    // Update color based on health
    int health = localPlayer.player.GetHealth();
    if (health < 30) {
        m_game->GetHUD().updateBaseColor("playerStats", sf::Color::Red);
    } else if (health < 70) {
        m_game->GetHUD().updateBaseColor("playerStats", sf::Color(255, 165, 0)); // Orange
    } else {
        m_game->GetHUD().updateBaseColor("playerStats", sf::Color::Black);
    }
}

void PlayingStateUI::UpdateLeaderboard(bool showLeaderboard) {
    if (!m_playerManager) return;
    
    // If leaderboard isn't showing, clear the text and return
    if (!showLeaderboard) {
        m_game->GetHUD().updateText("leaderboard", "");
        return;
    }
    
    // Get all players
    auto& players = m_playerManager->GetPlayers();
    
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
    m_game->GetHUD().updateText("leaderboard", leaderboardText);
}

void PlayingStateUI::UpdateWaveInfo() {
    if (!m_enemyManager) return;
    
    // Update wave info on HUD
    std::string waveInfo = "Wave: " + std::to_string(m_enemyManager->GetCurrentWave()) + 
                          " | Enemies: " + std::to_string(m_enemyManager->GetEnemyCount());
    m_game->GetHUD().updateText("waveInfo", waveInfo);
    
    // Update the game header with current wave
    std::string headerText = "WAVE " + std::to_string(m_enemyManager->GetCurrentWave());
    m_game->GetHUD().updateText("gameHeader", headerText);
}

void PlayingStateUI::UpdateDeathTimer(bool& isDeathTimerVisible) {
    if (!m_playerManager) return;
    
    auto& localPlayer = m_playerManager->GetLocalPlayer();
    if (localPlayer.player.IsDead() && localPlayer.respawnTimer > 0.0f) {
        // Format timer text
        int seconds = static_cast<int>(std::ceil(localPlayer.respawnTimer));
        std::string timerText = "Respawning in " + std::to_string(seconds) + "...";
        
        // Update the HUD element
        m_game->GetHUD().updateText("deathTimer", timerText);
        
        if (!isDeathTimerVisible) {
            isDeathTimerVisible = true;
        }
    } else if (isDeathTimerVisible) {
        // Hide timer when player is alive
        m_game->GetHUD().updateText("deathTimer", "");
        isDeathTimerVisible = false;
    }
}

void PlayingStateUI::SetWaveCompleteMessage(int currentWave, float timer) {
    // Display a message about the next wave
    std::string waveMsg = "Wave " + std::to_string(currentWave) + 
                         " complete! Next wave in " + std::to_string(static_cast<int>(std::ceil(timer))) + "...";
    m_game->GetHUD().updateText("waveInfo", waveMsg);
}

void PlayingStateUI::RenderEscapeMenu(sf::RenderWindow& window) {
    // Draw a semi-transparent overlay for the entire screen
    sf::RectangleShape overlay;
    overlay.setSize(sf::Vector2f(BASE_WIDTH, BASE_HEIGHT));
    overlay.setFillColor(sf::Color(0, 0, 0, 150));
    window.draw(overlay);
    float centerX = BASE_WIDTH / 2.0f;
    float centerY = BASE_HEIGHT / 2.0f;
    // Position the menu elements
    PositionEscapeMenuElements();
    if (m_isHost) {
        m_returnToLobbyButton.setPosition(
            centerX - m_returnToLobbyButton.getSize().x / 2.0f,
            centerY + 100.0f
        );
        
        // Position the return to lobby button text
        CenterTextInButton(m_returnToLobbyButtonText, m_returnToLobbyButton);
    }
    // Draw the menu components
    window.draw(m_menuBackground);
    window.draw(m_menuTitle);
    window.draw(m_continueButton);
    window.draw(m_continueButtonText);
    window.draw(m_returnButton);
    window.draw(m_returnButtonText);
    if (m_isHost) {
        window.draw(m_returnToLobbyButton);
        window.draw(m_returnToLobbyButtonText);
    }
}
void PlayingStateUI::SetHostStatus(bool isHost) {
    m_isHost = isHost;
}

bool PlayingStateUI::ProcessUIEvent(const sf::Event& event, bool& showEscapeMenu, bool& showGrid, 
                                 bool& cursorLocked, bool& showLeaderboard, bool& returnToMainMenu) {
    bool uiEventProcessed = false;
    
    if (event.type == sf::Event::KeyPressed) {
        // Use InputManager for keybindings
        if (m_game->GetInputManager().IsActionTriggered(GameAction::ToggleGrid, event)) {
            // Toggle grid visibility
            showGrid = !showGrid;
            uiEventProcessed = true;
        }
        else if (m_game->GetInputManager().IsActionTriggered(GameAction::ShowLeaderboard, event)) {
            // Toggle leaderboard visibility
            showLeaderboard = true;
            UpdateLeaderboard(true);
            uiEventProcessed = true;
        }
        else if (m_game->GetInputManager().IsActionTriggered(GameAction::ToggleCursorLock, event)) {
            // Toggle cursor lock
            cursorLocked = !cursorLocked;
            m_game->GetWindow().setMouseCursorGrabbed(cursorLocked);
            uiEventProcessed = true;
        }
        
        else if (m_game->GetInputManager().IsActionTriggered(GameAction::OpenMenu, event)) {
            // Toggle escape menu
            showEscapeMenu = !showEscapeMenu;
            
            if (showEscapeMenu) {
                // Show menu, release cursor, and make system cursor visible
                cursorLocked = false;
                m_game->GetWindow().setMouseCursorGrabbed(false);
            } else {
                // Hide menu, restore cursor lock, and hide system cursor
                cursorLocked = true;
                m_game->GetWindow().setMouseCursorGrabbed(true);
            }
            uiEventProcessed = true;
        }
    } 
    else if (event.type == sf::Event::KeyReleased) {
        if (event.key.code == m_game->GetInputManager().GetKeyBinding(GameAction::ShowLeaderboard)) {
            // Hide leaderboard when Tab is released
            showLeaderboard = false;
            m_game->GetHUD().updateText("leaderboard", "");
            uiEventProcessed = true;
        }
    }
    else if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        // Get mouse position in window coordinates
        sf::Vector2i mouseWindowPos(event.mouseButton.x, event.mouseButton.y);
        
        // Convert to UI coordinates for UI interaction
        sf::Vector2f mouseUIPos = m_game->WindowToUICoordinates(mouseWindowPos);
        
        // Only process if within UI viewport
        if (mouseUIPos.x >= 0 && mouseUIPos.y >= 0) {
            if (showEscapeMenu) {
                // Position the escape menu elements
                PositionEscapeMenuElements();
                
                // Check if continue button was clicked
                sf::FloatRect continueBounds = m_continueButton.getGlobalBounds();
                if (continueBounds.contains(mouseUIPos)) {
                    // Continue button clicked - close menu and resume game
                    showEscapeMenu = false;
                    cursorLocked = true;
                    m_game->GetWindow().setMouseCursorGrabbed(true);
                    uiEventProcessed = true;
                    return uiEventProcessed;
                }
                
                // Check if return button was clicked
                sf::FloatRect returnBounds = m_returnButton.getGlobalBounds();
                if (returnBounds.contains(mouseUIPos)) {
                    // Return to main menu
                    returnToMainMenu = true;
                    uiEventProcessed = true;
                    return uiEventProcessed;
                }
                if (m_isHost) {
                    sf::FloatRect returnToLobbyBounds = m_returnToLobbyButton.getGlobalBounds();
                    if (returnToLobbyBounds.contains(mouseUIPos)) {
                        // Send return to lobby command to all players
                        CSteamID myID = SteamUser()->GetSteamID();
                        std::string myIDStr = std::to_string(myID.ConvertToUint64());
                        std::string returnToLobbyMsg = StateMessageHandler::FormatReturnToLobbyMessage(myIDStr);
                        m_game->GetNetworkManager().BroadcastMessage(returnToLobbyMsg);
                        
                        // Also apply to ourselves
                        m_game->SetCurrentState(GameState::Lobby);
                        uiEventProcessed = true;
                        return uiEventProcessed;
                    }
                }
            } else {
                // Check for UI element clicks
                const auto& elements = m_game->GetHUD().getElements();
                for (const auto& [id, element] : elements) {
                    if (element.hoverable && element.visibleState == GameState::Playing) {
                        sf::Text textCopy = element.text;
                        textCopy.setPosition(element.pos);
                        sf::FloatRect bounds = textCopy.getGlobalBounds();
                        
                        if (bounds.contains(mouseUIPos)) {
                            // UI element was clicked
                            uiEventProcessed = true;
                            
                            if (id == "gridToggle") {
                                showGrid = !showGrid;
                                break;
                            }
                            else if (id == "cursorLockHint") {
                                cursorLocked = !cursorLocked;
                                m_game->GetWindow().setMouseCursorGrabbed(cursorLocked);
                                break;
                            }
                            else if (id == "forceFieldHint") {
                                // Toggle force field for local player
                                auto& localPlayer = m_playerManager->GetLocalPlayer();
                                
                                // Initialize force field if not already done
                                if (!localPlayer.player.HasForceField()) {
                                    localPlayer.player.InitializeForceField();
                                }
                                
                                // Toggle the force field
                                localPlayer.player.EnableForceField(!localPlayer.player.HasForceField());
                                
                                // Update UI
                                SetButtonState("forceFieldHint", localPlayer.player.HasForceField());
                                break;
                            }
                            // Handle other UI elements...
                        }
                    }
                }
            }
        }
    }
    
    return uiEventProcessed;
}

void PlayingStateUI::SetButtonState(const std::string& id, bool active) {
    if (id == "gridToggle" || id == "cursorLockHint" || id == "shopHint") {
        m_game->GetHUD().updateBaseColor(id, active ? sf::Color::Black : sf::Color(150, 150, 150));
    }
    else if (id == "forceFieldHint") {
        // Use a special color for the force field when active
        m_game->GetHUD().updateBaseColor(id, active ? sf::Color(100, 100, 255) : sf::Color(150, 150, 150));
    }
}

void PlayingStateUI::SetMenuState(bool showEscapeMenu) {
    // Position the escape menu elements
    PositionEscapeMenuElements();
    
    // Update state variables based on the escape menu visibility
    if (showEscapeMenu) {
        m_continueHovered = false;
        m_returnHovered = false;
    }
}

void PlayingStateUI::AnimateMenuLine(const std::string& lineId, float intensity) {
    m_game->GetHUD().animateLine(lineId, intensity);
}

bool PlayingStateUI::IsPointInRect(const sf::Vector2f& point, const sf::FloatRect& rect) {
    return point.x >= rect.left && point.x <= rect.left + rect.width &&
           point.y >= rect.top && point.y <= rect.top + rect.height;
}

void PlayingStateUI::UpdateButtonHoverState(sf::RectangleShape& button, const sf::Vector2f& mousePos, bool& isHovered) {
    bool hovering = IsPointInRect(mousePos, button.getGlobalBounds());
    
    if (hovering != isHovered) {
        isHovered = hovering;
        if (isHovered) {
            button.setFillColor(sf::Color(80, 80, 100, 230)); // Lighter when hovered
        } else {
            button.setFillColor(sf::Color(60, 60, 60, 230)); // Default color
        }
    }
}

sf::FloatRect PlayingStateUI::GetButtonBounds(const std::string& buttonId) {
    if (buttonId == "continue") {
        return m_continueButton.getGlobalBounds();
    } else if (buttonId == "return") {
        return m_returnButton.getGlobalBounds();
    }
    
    // Return an empty rectangle if button not found
    return sf::FloatRect();
}

bool PlayingStateUI::IsMouseOverUIElement(const std::string& elementId, const sf::Vector2f& mousePos) {
    const auto& elements = m_game->GetHUD().getElements();
    auto it = elements.find(elementId);
    
    if (it != elements.end() && it->second.visibleState == GameState::Playing) {
        sf::Text textCopy = it->second.text;
        textCopy.setPosition(it->second.pos);
        sf::FloatRect bounds = textCopy.getGlobalBounds();
        
        return bounds.contains(mousePos);
    }
    
    return false;
}

void PlayingStateUI::PositionEscapeMenuElements() {
    // Position menu elements using fixed coordinates based on BASE_WIDTH and BASE_HEIGHT
    float centerX = BASE_WIDTH / 2.0f;
    float centerY = BASE_HEIGHT / 2.0f;
    
    // Update menu background position
    m_menuBackground.setPosition(
        centerX - m_menuBackground.getSize().x / 2.0f,
        centerY - m_menuBackground.getSize().y / 2.0f
    );
    
    // Position the title at the top of the menu
    sf::FloatRect titleBounds = m_menuTitle.getLocalBounds();
    m_menuTitle.setOrigin(
        titleBounds.left + titleBounds.width / 2.0f,
        titleBounds.top + titleBounds.height / 2.0f
    );
    m_menuTitle.setPosition(
        centerX,
        centerY - m_menuBackground.getSize().y / 2.0f + 40.0f
    );
    
    // Position the continue button
    m_continueButton.setPosition(
        centerX - m_continueButton.getSize().x / 2.0f,
        centerY - 40.0f
    );
    
    // Position the continue button text
    CenterTextInButton(m_continueButtonText, m_continueButton);
    
    // Position the return button below the continue button
    m_returnButton.setPosition(
        centerX - m_returnButton.getSize().x / 2.0f,
        centerY + 30.0f
    );
    
    // Position the return button text
    CenterTextInButton(m_returnButtonText, m_returnButton);
}

void PlayingStateUI::CenterTextInButton(sf::Text& text, const sf::RectangleShape& button) {
    sf::FloatRect textBounds = text.getLocalBounds();
    text.setOrigin(
        textBounds.left + textBounds.width / 2.0f,
        textBounds.top + textBounds.height / 2.0f
    );
    text.setPosition(
        button.getPosition().x + button.getSize().x / 2.0f,
        button.getPosition().y + button.getSize().y / 2.0f - 5.0f
    );
}

