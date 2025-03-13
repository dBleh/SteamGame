#ifndef PLAYING_STATE_H
#define PLAYING_STATE_H

#include "State.h"
#include "../entities/Grid.h"
#include "../network/Host.h"
#include "../network/Client.h"
#include <memory>
#include <SFML/Graphics.hpp>

class Game;
class PlayerManager;
class PlayerRenderer;

class PlayingState : public State {
public:
    PlayingState(Game* game);
    ~PlayingState() = default;

    void Update(float dt) override;
    void Render() override;
    void ProcessEvent(const sf::Event& event) override;
    bool IsFullyLoaded();

private:
    void AttemptShoot(int mouseX, int mouseY);
    void ProcessEvents(const sf::Event& event);

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
};

#endif // PLAYING_STATE_H