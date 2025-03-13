#ifndef PLAYING_STATE_H
#define PLAYING_STATE_H

#include "State.h"
#include "../entities/Grid.h"
#include "../network/Host.h"
#include "../network/Client.h"
#include <memory>
#include <algorithm>  // For std::sort
#include <SFML/Graphics.hpp>

class Game;
class PlayerManager;
class PlayerRenderer;

class PlayingState : public State {
public:
    PlayingState(Game* game);
    ~PlayingState();  // Added destructor for cleanup

    void Update(float dt) override;
    void Render() override;
    void ProcessEvent(const sf::Event& event) override;
    bool IsFullyLoaded();

private:
    void AttemptShoot(int mouseX, int mouseY);
    void ProcessEvents(const sf::Event& event);
    void UpdatePlayerStats();
    void UpdateLeaderboard();

    std::unique_ptr<PlayerManager> playerManager;
    std::unique_ptr<PlayerRenderer> playerRenderer;
    std::unique_ptr<HostNetwork> hostNetwork;
    std::unique_ptr<ClientNetwork> clientNetwork;
    
    Grid grid;
    bool showGrid;
    bool playerLoaded;
    float loadingTimer;
    bool connectionSent;
    bool mouseHeld;
    float shootTimer;
    
    // Custom cursor shapes
    sf::CircleShape cursorOuterCircle;
    sf::CircleShape cursorCenterDot;
    sf::RectangleShape cursorHorizontalLine;
    sf::RectangleShape cursorVerticalLine;
    
    // Cursor locking
    bool cursorLocked;
    sf::Vector2i windowCenter;
    
    // Leaderboard
    bool showLeaderboard;
};

#endif // PLAYING_STATE_H