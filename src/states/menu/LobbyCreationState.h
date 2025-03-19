#ifndef LOBBYCREATIONSTATE_H
#define LOBBYCREATIONSTATE_H

#include "../../core/Game.h"  // Assuming Game.h includes necessary dependencies
#include <steam/steam_api.h>
#include "../base/State.h"

class Game;

class LobbyCreationState : public State {
public:
    LobbyCreationState(Game* game);
    void Update(float dt) override;
    void Render() override;
    void ProcessEvent(const sf::Event& event) override;
    
    void Enter();
    void Exit();
    void CreateLobby();
    bool isInputActive = false;
    float messageTimer = 0.0f;
    std::string previousStatusText = "";
    bool m_creationInProgress = false;
    float retryTimer = 0.0f;  // Timer for retry delay
    int retryCount = 0;       // Number of retries attempted
    const int maxRetries = 3; // Maximum number of retries
    const float retryDelay = 2.0f; // Delay between retries in seconds
private:
    void ProcessEvents(const sf::Event& event); // Note: This is unused in your current implementation

    
};

#endif // LOBBYCREATIONSTATE_H