#ifndef PLAYING_STATE_H
#define PLAYING_STATE_H

#include "State.h"
#include "../entities/Grid.h"
#include <memory>
#include <algorithm>  // For std::sort
#include <SFML/Graphics.hpp>

class Game;
class PlayerManager;
class PlayerRenderer;
class EnemyManager;
class TriangleEnemyManager;
class HostNetwork;
class ClientNetwork;

class PlayingState : public State {
public:
    PlayingState(Game* game);
    ~PlayingState();  // Added destructor for cleanup

    void Update(float dt) override;
    void Render() override;
    void ProcessEvent(const sf::Event& event) override;
    bool IsFullyLoaded();
    
    // Getter for EnemyManager
    EnemyManager* GetEnemyManager();
    
    // This method is kept for backward compatibility but returns nullptr
    // as all enemies are now handled by the unified EnemyManager
    TriangleEnemyManager* GetTriangleEnemyManager();

private:
    void AttemptShoot(int mouseX, int mouseY);
    void ProcessEvents(const sf::Event& event);
    void UpdatePlayerStats();
    void UpdateLeaderboard();
    void UpdateWaveInfo();
    void StartFirstWave();
    void StartNextWave();
    bool AreAllEnemiesDefeated();
    
    std::unique_ptr<PlayerManager> playerManager;
    std::unique_ptr<PlayerRenderer> playerRenderer;
    std::unique_ptr<EnemyManager> enemyManager;
    std::unique_ptr<HostNetwork> hostNetwork;
    std::unique_ptr<ClientNetwork> clientNetwork;
    sf::Text menuTitle;
    sf::RectangleShape continueButton;
    sf::Text continueButtonText;

    // Methods for handling button hovers
    void updateButtonHoverState(sf::RectangleShape& button, const sf::Vector2f& mousePos, bool& isHovered);
    bool isPointInRect(const sf::Vector2f& point, const sf::FloatRect& rect);
    Grid grid;
    bool showGrid;
    bool playerLoaded;
    float loadingTimer;
    bool connectionSent;
    bool mouseHeld;
    float shootTimer;

    bool showEscapeMenu;
    sf::RectangleShape menuBackground;
    sf::RectangleShape returnButton;
    sf::Text returnButtonText;
    
    // Cursor locking
    bool cursorLocked;
    sf::Vector2i windowCenter;
    
    // Leaderboard
    bool showLeaderboard;
};

#endif // PLAYING_STATE_H