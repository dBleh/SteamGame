#include "Game.h"
#include "states/MainMenuState.h"
#include "states/LobbyCreationState.h"
#include "states/LobbySearchState.h"
#include "states/SettingsState.h"
#include "states/PlayingState.h"
#include "states/LobbyState.h"
#include <steam/steam_api.h>
#include <iostream>



Game::Game() : hud(font) {
    window.create(sf::VideoMode(BASE_WIDTH, BASE_HEIGHT), "SteamGame");
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

    settingsManager = std::make_shared<SettingsManager>();
    inputHandler = std::make_shared<InputHandler>(settingsManager);

    // Initialize camera for game world
    camera.setSize(BASE_WIDTH, BASE_HEIGHT);
    camera.setCenter(BASE_WIDTH / 2.f, BASE_HEIGHT / 2.f);  // Initial center
    
    // Initialize UI view with base resolution
    uiView.setSize(BASE_WIDTH, BASE_HEIGHT);
    uiView.setCenter(BASE_WIDTH / 2.f, BASE_HEIGHT / 2.f);
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

        deltaTime = clock.restart().asSeconds();
       
        sf::Event event;
        while (window.pollEvent(event)) {
            inputHandler->ProcessEvent(event);
            ProcessEvents(event);
            if (state) state->ProcessEvent(event);
        }

        if (state) state->Update(deltaTime);

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
            case GameState::Settings:
                if (!dynamic_cast<SettingsState*>(state.get())) {
                    state = std::make_unique<SettingsState>(this);
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

        // Let the state handle rendering
        if (state) state->Render();
    }
}

void Game::SetCurrentState(GameState newState) {
    // Clean up resources when leaving a lobby state
    if ((currentState == GameState::Lobby || currentState == GameState::Playing) && 
        newState == GameState::MainMenu && inLobby) {
        // Leave the Steam lobby
        SteamMatchmaking()->LeaveLobby(currentLobby);
        
        // Reset lobby state
        inLobby = false;
        currentLobby = k_steamIDNil;
        
        // Also reset the NetworkManager state
        networkManager->ResetLobbyState();
        
        std::cout << "[GAME] Left lobby and reset lobby state" << std::endl;
    }
    
    // Set the new state
    currentState = newState;
}

void Game::ProcessEvents(sf::Event& event) {
    if (event.type == sf::Event::Closed) {
        window.close();
    }
    if (event.type == sf::Event::Resized) {
        AdjustViewToWindow();
    }
    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::F11) {
            // Toggle fullscreen with F11
            static bool isFullscreen = false;
            isFullscreen = !isFullscreen;
            
            if (isFullscreen) {
                window.create(sf::VideoMode::getDesktopMode(), "SteamGame", sf::Style::Fullscreen);
            } else {
                window.create(sf::VideoMode(BASE_WIDTH, BASE_HEIGHT), "SteamGame");
            }
            
            window.setFramerateLimit(60);
            AdjustViewToWindow();
        }
    }
    if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::R) {
        std::cout << "Triggering ready state" << std::endl;
    }
}

void Game::AdjustViewToWindow() {
    sf::Vector2u windowSize = window.getSize();
    
    // Game world camera - shows more of the world when window is larger
    camera.setSize(static_cast<float>(windowSize.x), static_cast<float>(windowSize.y));
    camera.setCenter(camera.getCenter()); // Preserve current center
    
    // UI view - fixed size that doesn't scale
    uiView.setSize(BASE_WIDTH, BASE_HEIGHT);
    uiView.setCenter(BASE_WIDTH / 2.0f, BASE_HEIGHT / 2.0f);
    
    // Set viewport to match window's aspect ratio while maintaining fixed element sizes
    float windowRatio = windowSize.x / static_cast<float>(windowSize.y);
    float baseRatio = BASE_WIDTH / static_cast<float>(BASE_HEIGHT);
    
    sf::FloatRect viewport(0.f, 0.f, 1.f, 1.f);
    
    if (windowRatio > baseRatio) {
        // Window is wider than our base ratio - add black bars on the sides
        viewport.width = baseRatio / windowRatio;
        viewport.left = (1.f - viewport.width) / 2.f;
    } else if (windowRatio < baseRatio) {
        // Window is taller than our base ratio - add black bars on top/bottom
        viewport.height = windowRatio / baseRatio;
        viewport.top = (1.f - viewport.height) / 2.f;
    }
    
    uiView.setViewport(viewport);
}

sf::View& Game::GetUIView() {
    return uiView;
}

sf::Vector2f Game::GetUIScale() const {
    // Get the current window size
    sf::Vector2u winSize = window.getSize();
    
    // Calculate the scale factors
    float scaleX = winSize.x / static_cast<float>(BASE_WIDTH);
    float scaleY = winSize.y / static_cast<float>(BASE_HEIGHT);
    
    return sf::Vector2f(scaleX, scaleY);
}

sf::Vector2f Game::WindowToUICoordinates(sf::Vector2i windowPos) const {
    // Get the viewport
    sf::FloatRect viewport = uiView.getViewport();
    sf::Vector2u winSize = window.getSize();
    
    // Calculate viewport in pixels
    float viewportLeft = viewport.left * winSize.x;
    float viewportTop = viewport.top * winSize.y;
    float viewportWidth = viewport.width * winSize.x;
    float viewportHeight = viewport.height * winSize.y;
    
    // Check if point is within viewport
    if (windowPos.x >= viewportLeft && 
        windowPos.x <= viewportLeft + viewportWidth &&
        windowPos.y >= viewportTop && 
        windowPos.y <= viewportTop + viewportHeight) {
        
        // Convert to normalized position within viewport (0-1)
        float normalizedX = (windowPos.x - viewportLeft) / viewportWidth;
        float normalizedY = (windowPos.y - viewportTop) / viewportHeight;
        
        // Convert to UI coordinates
        return sf::Vector2f(
            normalizedX * BASE_WIDTH,
            normalizedY * BASE_HEIGHT
        );
    }
    
    // Return (-1,-1) if outside viewport
    return sf::Vector2f(-1, -1);
}