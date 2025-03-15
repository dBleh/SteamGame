#ifndef LOBBYCREATIONSTATE_H
#define LOBBYCREATIONSTATE_H

#include "../Game.h"  // Assuming Game.h includes necessary dependencies
#include <steam/steam_api.h>
#include "State.h"

class Game;

class LobbyCreationState : public State {
public:
    LobbyCreationState(Game* game);
    void Update(float dt) override;
    void Render() override;
    void ProcessEvent(const sf::Event& event) override;
    
    // Added state lifecycle methods
    void Enter();
    void Exit();

    // Function to handle lobby entry logic after creation, without player references
    //void onLobbyEnter(CSteamID lobbyID);
    
    // Added callback for when lobby creation fails
    void onLobbyCreationFailed();
    void CreateLobby();

private:
    void ProcessEvents(const sf::Event& event);

    bool isInputActive = false;
    float messageTimer = 0.0f;
    std::string previousStatusText = "";
    
    // Flag to track if lobby creation is in progress
    bool m_creationInProgress;
};

#endif // LOBBYCREATIONSTATE_H