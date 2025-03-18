#ifndef PLAYING_STATE_H
#define PLAYING_STATE_H

#include "base/State.h"
#include "../ui/Grid.h"
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
class PlayingStateUI; // Forward declaration for UI

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
    
    void ProcessGameplayEvents(const sf::Event& event);
    float currentZoom;
   
    
    // Zoom handling methods
    void HandleZoom(float delta);
    void AdjustViewToWindow();
    // Game components
    std::unique_ptr<PlayerManager> playerManager;
    std::unique_ptr<PlayerRenderer> playerRenderer;
    std::unique_ptr<HostNetwork> hostNetwork;
    std::unique_ptr<ClientNetwork> clientNetwork;
    std::unique_ptr<EnemyManager> enemyManager;
    std::unique_ptr<PlayingStateUI> ui;
    
    // Wave management
    float waveTimer;
    bool waitingForNextWave;
    
    // Game state
    Grid grid;
    bool showGrid;
    bool playerLoaded;
    float loadingTimer;
    bool connectionSent;
    bool mouseHeld;
    float shootTimer;
    bool showEscapeMenu;

    // Cursor locking
    bool cursorLocked;
    sf::Vector2i windowCenter;
    bool isDeathTimerVisible;
    
    // Leaderboard
    bool showLeaderboard;

    // Shop system
    std::unique_ptr<Shop> shop;
    bool showShop;
};

#endif // PLAYING_STATE_H