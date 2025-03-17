#ifndef PLAYING_STATE_H
#define PLAYING_STATE_H

#include "base/State.h"
#include "../entities/Grid.h"
#include "../entities/enemies/EnemyManager.h"
#include "../entities/enemies/Enemy.h"
#include <memory>
#include <algorithm>  // For std::sort
#include <SFML/Graphics.hpp>

class Game;
class PlayerManager;
class PlayerRenderer;
class HostNetwork;
class ClientNetwork;
class Shop; // Forward declaration for Shop

class PlayingState : public State {
public:
    PlayingState(Game* game);
    ~PlayingState();  // Added destructor for cleanup

    void Update(float dt) override;
    void Render() override;
    void ProcessEvent(const sf::Event& event) override;
    bool IsFullyLoaded();
    
    // Enemy management methods
    EnemyManager* GetEnemyManager() { return enemyManager.get(); }
    void StartWave(int enemyCount);
    
    // Static method to access the shop instance
    static Shop* GetShopInstance() { return shopInstance; }

private:
    // Static shop instance
    static Shop* shopInstance;
    
    void AttemptShoot(int mouseX, int mouseY);
    void ProcessEvents(const sf::Event& event);
    void UpdatePlayerStats();
    void UpdateLeaderboard();
    void CheckBulletEnemyCollisions();
    void UpdateWaveInfo();
    bool isPointInRect(const sf::Vector2f& point, const sf::FloatRect& rect);
    void updateButtonHoverState(sf::RectangleShape& button, const sf::Vector2f& mousePos, bool& isHovered);
    
    std::unique_ptr<PlayerManager> playerManager;
    std::unique_ptr<PlayerRenderer> playerRenderer;
    std::unique_ptr<HostNetwork> hostNetwork;
    std::unique_ptr<ClientNetwork> clientNetwork;
    std::unique_ptr<EnemyManager> enemyManager;
    
    // Wave management
    float waveTimer;
    bool waitingForNextWave;
    
    sf::Text menuTitle;
    sf::RectangleShape continueButton;
    sf::Text continueButtonText;
    
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
    sf::Text deathTimerText;
    bool isDeathTimerVisible;   
    // Leaderboard
    bool showLeaderboard;

    // Shop system
    std::unique_ptr<Shop> shop;
    bool showShop;
};

#endif // PLAYING_STATE_H