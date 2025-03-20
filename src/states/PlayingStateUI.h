#ifndef PLAYING_STATE_UI_H
#define PLAYING_STATE_UI_H

#include <SFML/Graphics.hpp>
#include <string>
#include <unordered_map>
#include "../core/GameState.h"
#include "../core/Game.h"
#include "../entities/player/PlayerManager.h"

class HUD;
class EnemyManager;

class PlayingStateUI {
public:
    PlayingStateUI(Game* game, PlayerManager* playerManager, EnemyManager* enemyManager);
    ~PlayingStateUI();

    // UI Setup
    void InitializeUI();
    
    // UI Updates
    void Update(float dt);
    void UpdatePlayerStats();
    void UpdateLeaderboard(bool showLeaderboard);
    void UpdateWaveInfo();
    void UpdateDeathTimer(bool& isDeathTimerVisible);
    
    // UI Rendering
    void RenderEscapeMenu(sf::RenderWindow& window);
    
    // UI Event Processing
    bool ProcessUIEvent(const sf::Event& event, bool& showEscapeMenu, bool& showGrid, 
                     bool& cursorLocked, bool& showLeaderboard, bool& returnToMainMenu);

    // UI State Management
    void SetButtonState(const std::string& id, bool active);
    void SetMenuState(bool showEscapeMenu);
    
    // UI Helpers
    bool IsPointInRect(const sf::Vector2f& point, const sf::FloatRect& rect);
    void UpdateButtonHoverState(sf::RectangleShape& button, const sf::Vector2f& mousePos, bool& isHovered);
    sf::Vector2f WindowToUICoordinates(const sf::RenderWindow& window, sf::Vector2i mousePos);

    // UI Animation
    void AnimateMenuLine(const std::string& lineId, float intensity);
    
    // UI Element Access
    sf::FloatRect GetButtonBounds(const std::string& buttonId);
    bool IsMouseOverUIElement(const std::string& elementId, const sf::Vector2f& mousePos);
    void SetWaveCompleteMessage(int currentWave, float timer);

private:
    Game* m_game;
    PlayerManager* m_playerManager;
    EnemyManager* m_enemyManager;
    
    // Escape menu elements
    sf::RectangleShape m_menuBackground;
    sf::Text m_menuTitle;
    sf::RectangleShape m_continueButton;
    sf::Text m_continueButtonText;
    sf::RectangleShape m_returnButton;
    sf::Text m_returnButtonText;
    
    // UI state tracking
    bool m_continueHovered;
    bool m_returnHovered;
    
    // Helper methods for UI positioning
    void PositionEscapeMenuElements();
    void CenterTextInButton(sf::Text& text, const sf::RectangleShape& button);
};

#endif // PLAYING_STATE_UI_H