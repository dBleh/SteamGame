#include "Game.h"
#include "states/MainMenuState.h"
#include "states/LobbyCreationState.h"
#include "states/LobbySearchState.h"
#include "states/LobbyState.h"
#include <steam/steam_api.h>
#include <iostream>

Game::Game() : hud(font) {
    // Initialize window
    window.create(sf::VideoMode(1280, 720), "SteamGame");
    window.setFramerateLimit(60);

    // Load font
    if (!font.loadFromFile("Roboto-Regular.ttf")) {
        if (!font.loadFromFile("../../Roboto-Regular.ttf")) {
            std::cerr << "[ERROR] Failed to load font!" << std::endl;
            std::exit(1);
        }
    }

    // Initialize Steam
    if (SteamAPI_Init()) {
        steamInitialized = true;
        localSteamID = SteamUser()->GetSteamID();
    } else {
        std::cerr << "[ERROR] Steam API initialization failed!" << std::endl;
    }

    // Initialize NetworkManager
    networkManager = std::make_unique<NetworkManager>(this);

    // Set initial state
    state = std::make_unique<MainMenuState>(this);

    // Initialize camera
    camera.setSize(1280.f, 720.f);  // Match window size
    camera.setCenter(640.f, 360.f);  // Center of 1280x720
    window.setView(camera);
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

        switch (currentState) {
            case GameState::MainMenu:
                if (!dynamic_cast<MainMenuState*>(state.get()))
                    state = std::make_unique<MainMenuState>(this);
                break;
            case GameState::LobbyCreation:
                if (!dynamic_cast<LobbyCreationState*>(state.get()))
                    state = std::make_unique<LobbyCreationState>(this);
                break;
            case GameState::LobbySearch:
                if (!dynamic_cast<LobbySearchState*>(state.get()))
                    state = std::make_unique<LobbySearchState>(this);
                break;
            case GameState::Lobby:
                if (!dynamic_cast<LobbyState*>(state.get()))
                    state = std::make_unique<LobbyState>(this);  // Assume client for simplicity; adjust as needed
                break;
            default:
                break;
        }

        // Rendering
        if (state) state->Render();
        window.display();
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
    window.setView(camera);
}