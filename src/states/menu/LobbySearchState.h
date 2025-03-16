#ifndef LOBBY_SEARCH_STATE_H
#define LOBBY_SEARCH_STATE_H

#include "../base/State.h"
#include <steam/isteammatchmaking.h>
#include <vector>

class LobbySearchState : public State {
public:
    explicit LobbySearchState(Game* game);
    void Update(float dt) override;
    void Render() override;
    void ProcessEvent(const sf::Event& event) override;

private:
    void ProcessEvents(const sf::Event& event);
    void SearchLobbies();
    void UpdateLobbyListDisplay();
    void JoinLobby(CSteamID lobby);
    void JoinLobbyByIndex(int index);
    void InitializeLobbyButtons(Game* game, float centerX, float startY, float spacing);
    
    std::vector<std::pair<CSteamID, std::string>> lobbyList;
    std::vector<std::pair<CSteamID, std::string>> localLobbyList;
};

#endif // LOBBY_SEARCH_STATE_H