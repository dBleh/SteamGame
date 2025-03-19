// LoadingState.cpp
#include "LoadingState.h"
#include "../core/Game.h"
#include "../utils/config/Config.h"
#include <steam/steam_api.h>

LoadingState::LoadingState(Game* game) : State(game) {
    game->GetHUD().addElement("loadingText", "Loading Steam...", 36,
                              sf::Vector2f(BASE_WIDTH / 2.0f - 150.0f, BASE_HEIGHT / 2.0f),
                              GameState::Loading, HUD::RenderMode::ScreenSpace, false);
}

void LoadingState::Update(float dt) {
    static float elapsedTime = 0.0f;
    elapsedTime += dt;

    if (game->IsSteamInitialized()) {
        // Wait for SteamMatchmaking and test connectivity
        if (!SteamMatchmaking()) {
            if (elapsedTime > 10.0f) { // Timeout after 10 seconds
                game->GetHUD().updateText("loadingText", "Steam matchmaking unavailable. Please restart.");
            }
            return;
        }

        // Test connectivity by requesting lobby list (non-blocking)
        static bool lobbyListRequested = false;
        if (!lobbyListRequested) {
            SteamAPICall_t call = SteamMatchmaking()->RequestLobbyList();
            if (call != k_uAPICallInvalid) {
                lobbyListRequested = true;
            }
        }

        // Check if lobby list callback has been received (via NetworkManager::OnLobbyMatchList)
        if (game->GetNetworkManager().IsLobbyListUpdated()) {
            game->GetNetworkManager().ResetLobbyListUpdated();
            game->SetCurrentState(GameState::MainMenu);
        } else if (elapsedTime > 10.0f) {
            game->GetHUD().updateText("loadingText", "Steam connection failed. Please restart.");
        }
    } else if (elapsedTime > 10.0f) {
        game->GetHUD().updateText("loadingText", "Steam failed to initialize. Please restart.");
    }
}

void LoadingState::Render() {
    game->GetWindow().clear(MAIN_BACKGROUND_COLOR);
    game->GetWindow().setView(game->GetUIView());
    game->GetHUD().render(game->GetWindow(), game->GetUIView(), GameState::Loading);
    game->GetWindow().display();
}

void LoadingState::ProcessEvent(const sf::Event& event) {
    if (event.type == sf::Event::Closed) {
        game->GetWindow().close();
    }
}