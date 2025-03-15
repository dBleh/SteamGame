#ifndef LOBBYSTATE_H
#define LOBBYSTATE_H

#include "State.h"
#include "../Game.h"
#include "../entities/Player.h"
#include "../utils/MessageHandler.h"
#include "../network/Host.h"
#include "../network/Client.h"
#include "../entities/Grid.h"
#include <steam/steam_api.h>
#include <SFML/Graphics.hpp>
#include <array>
#include <memory>
#include <unordered_map>

class Game;
class PlayerRenderer;
class LobbyState : public State {
public:
    LobbyState(Game* game);
    ~LobbyState();
    bool IsFullyLoaded();
    void Update(float dt) override;
    void Render() override;
    void ProcessEvent(const sf::Event& event) override;
    
private:
    bool mouseHeld = false;
    float shootTimer = 0.f;
    void AttemptShoot(int mouseX, int mouseY);
    void UpdateRemotePlayers();
    void BroadcastPlayersList();
    void ProcessEvents(const sf::Event& event);
    void UpdateLobbyMembers(); // Kept as stub for compatibility
    bool AllPlayersReady();    // Kept as stub for compatibility

    Player localPlayer;         // The player's cube representation.
    bool playerLoaded;          // True when the local player has finished "loading".
    float loadingTimer;  
    std::string chatMessages;   // Store chat history
    sf::Text localPlayerName;

    // Networking objects: only one is active based on role.
    std::unique_ptr<HostNetwork> hostNetwork;
    std::unique_ptr<ClientNetwork> clientNetwork;

    std::unique_ptr<PlayerManager> playerManager;
    std::unique_ptr<PlayerRenderer> playerRenderer;
    std::chrono::steady_clock::time_point lastReadyToggle; // For debounce
    static constexpr float READY_TOGGLE_COOLDOWN = 0.2f;
    bool connectionSent = false;

    //For the grid
    Grid grid;
    bool showGrid;
};

#endif // LOBBYSTATE_H
