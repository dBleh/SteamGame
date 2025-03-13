// SteamHelpers.cpp
#include "SteamHelpers.h"
#include "../Game.h"
#include "../states/PlayingState.h"

PlayingState* GetPlayingState(Game* game) {
    if (game->GetCurrentState() == GameState::Playing) {
        return dynamic_cast<PlayingState*>(game->GetState());
    }
    return nullptr;
}