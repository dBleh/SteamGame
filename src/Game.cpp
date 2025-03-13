#include "Game.h"
#include "states/MainMenuState.h"
#include "states/LobbyCreationState.h"
#include "states/LobbySearchState.h"
#include "states/PlayingState.h"
#include "states/LobbyState.h"
#include <steam/steam_api.h>
#include <iostream>

Game::Game() : hud(font) {
    window.create(sf::VideoMode(1280, 720), "SteamGame");
    window.setFramerateLimit(60);

    if (!font.loadFromFile("Roboto-Regular.ttf")) {
        if (!font.loadFromFile("../../Roboto-Regular.ttf")) {
            std::cerr << "[ERROR] Failed to load font!" << std::endl;
            std::exit(1);
        }
    }

    if (SteamAPI_Init()) {
        steamInitialized = true;
        localSteamID = SteamUser()->GetSteamID();
    } else {
        std::cerr << "[ERROR] Steam API initialization failed!" << std::endl;
    }

    networkManager = std::make_unique<NetworkManager>(this);
    state = std::make_unique<MainMenuState>(this);

    // Initialize camera for game world
    camera.setSize(1280.f, 720.f);
    camera.setCenter(640.f, 360.f);  // Initial center
}

Game::~Game() {
    if (inLobby) {
        SteamMatchmaking()->LeaveLobby(currentLobby);
    }
    SteamAPI_Shutdown();
}

void Game::Run() {
    sf::Clock clock;
    window.setKeyRepeatEnabled(false);
    while (window.isOpen()) {
        if (steamInitialized) {
            SteamAPI_RunCallbacks();
        }

        networkManager->ReceiveMessages();

        float dt = clock.restart().asSeconds();
       
        sf::Event event;
        while (window.pollEvent(event)) {
            ProcessEvents(event);
            if (state) state->ProcessEvent(event);
        }

        if (state) state->Update(dt);

        // Only create a new state if we don't have that state already
        bool stateChanged = false;
        switch (currentState) {
            case GameState::MainMenu:
                if (!dynamic_cast<MainMenuState*>(state.get())) {
                    state = std::make_unique<MainMenuState>(this);
                    stateChanged = true;
                }
                break;
            case GameState::LobbyCreation:
                if (!dynamic_cast<LobbyCreationState*>(state.get())) {
                    state = std::make_unique<LobbyCreationState>(this);
                    stateChanged = true;
                }
                break;
            case GameState::LobbySearch:
                if (!dynamic_cast<LobbySearchState*>(state.get())) {
                    state = std::make_unique<LobbySearchState>(this);
                    stateChanged = true;
                }
                break;
            case GameState::Lobby:
                if (!dynamic_cast<LobbyState*>(state.get())) {
                    state = std::make_unique<LobbyState>(this);
                    stateChanged = true;
                }
                break;
            case GameState::Playing:
                if (!dynamic_cast<PlayingState*>(state.get())) {
                    state = std::make_unique<PlayingState>(this);
                    stateChanged = true;
                }
                break;
            default:
                break;
        }

        // Print debug info if state changed
        if (stateChanged) {
            std::cout << "[INFO] Switched to state: " << static_cast<int>(currentState) << std::endl;
        }

        // Render game world with camera
        window.setView(camera);
        if (state) state->Render();
    }
}

void Game::SetCurrentState(GameState newState) {
    currentState = newState;
}

void Game::ProcessEvents(sf::Event& event) {
    if (event.type == sf::Event::Closed) {
        window.close();
    }
    if (event.type == sf::Event::Resized) {
        AdjustViewToWindow();
    }
    if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::R) {
        std::cout << "Triggering ready state" << std::endl;
    }
}

void Game::AdjustViewToWindow() {
    sf::Vector2u winSize = window.getSize();
    camera.setSize(static_cast<float>(winSize.x), static_cast<float>(winSize.y));
    // Note: Center is updated in LobbyState, so no need to reset it here
}