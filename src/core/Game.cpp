#include "Game.h"
#include "../states/menu/MainMenuState.h"
#include "../states/menu/LobbyCreationState.h"
#include "../states/menu/LobbySearchState.h"
#include "../states/menu/SettingsState.h"
#include "../states/PlayingState.h"
#include "../states/menu/LobbyState.h"
#include "../states/menu/LoadingState.h"
#include "../states/GameSettingsManager.h"
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

    // Robust Steam initialization check
    // In Game.cpp constructor - replace the current Steam initialization with this more robust version

// Robust Steam initialization check
if (!SteamAPI_Init()) {
    std::cerr << "[ERROR] Steam API initialization failed!" << std::endl;
    steamInitialized = false;
} else {
    // Wait for Steam to be fully ready
    const int maxAttempts = 50; // ~5 seconds at 100ms per attempt
    int attempts = 0;
    bool friendsNetworkReady = false;
    
    std::cout << "[GAME] Starting Steam initialization process...\n";
    while (attempts < maxAttempts) {
        SteamAPI_RunCallbacks(); // Process Steam callbacks
        
        // Check core Steam components
        bool coreInitialized = SteamUser() && SteamUser()->BLoggedOn() && SteamMatchmaking();
        
        // Check friends network specifically
        bool friendsAvailable = SteamFriends() && SteamFriends()->GetFriendCount(k_EFriendFlagImmediate) >= 0;
        
        // Additional check to ensure friends network is properly connected
        if (coreInitialized && friendsAvailable) {
            if (!friendsNetworkReady) {
                // Wait for at least one more callback cycle for the friends network to fully populate
                friendsNetworkReady = true;
                std::cout << "[GAME] Core Steam services connected, waiting for friends network...\n";
                sf::sleep(sf::milliseconds(200)); // Extra delay for network stabilization
                SteamAPI_RunCallbacks(); // Process additional callbacks
                continue;
            }
            
            // Additional validation - attempt to get persona name as final check
            const char* personaName = SteamFriends()->GetPersonaName();
            if (personaName && strlen(personaName) > 0) {
                steamInitialized = true;
                localSteamID = SteamUser()->GetSteamID();
                std::cout << "[GAME] Steam fully initialized and connected as: " << personaName << "\n";
                
                // Test friends list functionality
                int friendCount = SteamFriends()->GetFriendCount(k_EFriendFlagImmediate);
                std::cout << "[GAME] Friends network ready with " << friendCount << " friends\n";
                break;
            }
        }
        
        if (attempts % 10 == 0) {
            std::cout << "[GAME] Waiting for Steam initialization... (" << attempts << "/" << maxAttempts << ")\n";
        }
        
        sf::sleep(sf::milliseconds(100)); // Brief delay to avoid tight loop
        attempts++;
    }
    
    if (!steamInitialized) {
        std::cerr << "[ERROR] Steam failed to connect after " << maxAttempts << " attempts\n";
        std::cerr << "[ERROR] Friends network ready: " << (friendsNetworkReady ? "Yes" : "No") << "\n";
        
        // Additional diagnostics
        if (SteamUser() && !SteamUser()->BLoggedOn()) {
            std::cerr << "[ERROR] Steam user not logged on\n";
        }
        
        if (!SteamFriends()) {
            std::cerr << "[ERROR] SteamFriends interface not available\n";
        }
        
        // Set a flag to show error in LoadingState
        steamConnectionError = true;
    }
}

    networkManager = std::make_unique<NetworkManager>(this);
    state = std::make_unique<LoadingState>(this); // Start with LoadingState
    currentState = GameState::Loading;

    settingsManager = std::make_shared<SettingsManager>();
    inputHandler = std::make_shared<InputHandler>(settingsManager);
    gameSettingsManager = std::make_unique<GameSettingsManager>(this);

    // Initialize camera for game world
    camera.setSize(BASE_WIDTH, BASE_HEIGHT);
    camera.setCenter(BASE_WIDTH / 2.f, BASE_HEIGHT / 2.f);
    currentZoom = DEFAULT_ZOOM;

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

        bool stateChanged = false;
        switch (currentState) {
            case GameState::Loading:
                if (!dynamic_cast<LoadingState*>(state.get())) {
                    state = std::make_unique<LoadingState>(this);
                    stateChanged = true;
                }
                break;
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

// In Game.cpp - modify the SetCurrentState method:
void Game::SetCurrentState(GameState newState) {
    // Don't do anything if we're already in the requested state
    if (currentState == newState) {
        std::cout << "[GAME] Already in state " << static_cast<int>(newState) << ", ignoring transition request.\n";
        return;
    }
    // First clear the message handler to prevent callbacks to deleted objects
    if ((currentState == GameState::Lobby || currentState == GameState::Playing) && 
        newState == GameState::MainMenu) {
        // Clear message handler first before any objects are destroyed
        networkManager->SetMessageHandler(nullptr);
    }
    
    // If we're leaving a lobby state to go to the main menu
    if ((currentState == GameState::Lobby || currentState == GameState::Playing) && 
        newState == GameState::MainMenu && inLobby) {
        
        // Leave the Steam lobby first
        if (SteamMatchmaking() && currentLobby != k_steamIDNil) {
            std::cout << "[GAME] Leaving Steam lobby: " << currentLobby.ConvertToUint64() << std::endl;
            SteamMatchmaking()->LeaveLobby(currentLobby);
        } else {
            std::cout << "[GAME] No valid lobby to leave" << std::endl;
        }
        
        // Reset lobby state
        inLobby = false;
        currentLobby = k_steamIDNil;
        
        // Also reset the NetworkManager state
        networkManager->ResetLobbyState();
        
        std::cout << "[GAME] Left lobby and reset lobby state" << std::endl;
    }
    
    // Handle exiting the current state if needed
    if (state && currentState != newState) {
        // Allow the state to clean up resources before transitioning
        if (auto exitableState = dynamic_cast<LobbyCreationState*>(state.get())) {
            exitableState->Exit();
        } else if (auto lobbyState = dynamic_cast<LobbyState*>(state.get())) {
            // Clear any references that could cause use-after-free
        }
    }
    
    // Set the new state
    currentState = newState;
    std::cout << "[GAME] Switched to state: " << static_cast<int>(currentState) << std::endl;
}

// In Game.cpp - modify the ProcessEvents method to handle mouse wheel events
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



// In Game.cpp - modify AdjustViewToWindow method
void Game::AdjustViewToWindow() {
    sf::Vector2u windowSize = window.getSize();
    
    // Game world camera - shows more of the world when window is larger
    // Apply zoom factor to the camera size
    camera.setSize(static_cast<float>(windowSize.x) / currentZoom, 
                  static_cast<float>(windowSize.y) / currentZoom);
    camera.setCenter(camera.getCenter()); // Preserve current center
    
    // UI view - fixed size that doesn't scale - NOT affected by zoom
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