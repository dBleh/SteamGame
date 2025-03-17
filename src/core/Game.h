#ifndef GAME_H
#define GAME_H

#include <SFML/Graphics.hpp>
#include "../ui/hud/Hud.h"
#include "../network/NetworkManager.h"
#include "../utils/config/SettingsManager.h"  
#include "../utils/input/InputHandler.h" 
#include "../utils/input/InputManager.h"  
#include "../utils/config/Config.h"
#include "GameState.h"
#include <memory>
#include <steam/steam_api.h>

class State;
class NetworkManager;

class Game {
public:
    Game();
    ~Game();

    void Run();
    void SetCurrentState(GameState state);
    GameState GetCurrentState() const { return currentState; }
    sf::RenderWindow& GetWindow() { return window; }
    HUD& GetHUD() { return hud; }
    NetworkManager& GetNetworkManager() { return *networkManager; }
    std::shared_ptr<SettingsManager> GetSettingsManager() const { return settingsManager; }
    std::shared_ptr<InputHandler> GetInputHandler() const {return inputHandler; }
    std::string& GetLobbyNameInput() { return lobbyNameInput; }
    bool IsSteamInitialized() const { return steamInitialized; }
    
    void SetLocalSteamID(const CSteamID& id) { localSteamID = id; }
    CSteamID GetLocalSteamID() const { return localSteamID; }
    CSteamID GetLobbyID() const { return networkManager->GetCurrentLobbyID(); }
    sf::Font& GetFont() { return font; }
    sf::View& GetCamera() { return camera; }  // Access to game world camera
    sf::View& GetUIView(); // Access to the UI view
    sf::Vector2f GetUIScale() const; // Get UI scaling factors
    sf::Vector2f WindowToUICoordinates(sf::Vector2i windowPos) const; // Convert window to UI coordinates
    State* GetState() { return state.get(); }
    float GetDeltaTime() const { return deltaTime; }
        
    InputManager& GetInputManager() { return inputManager; }
    bool IsInLobby() const { return inLobby; }
    void SetInLobby(bool status) { 
        inLobby = status; 
        if (status) {
            currentLobby = networkManager->GetCurrentLobbyID();
        }
    }
private:
    // Add zoom-related variables
    float currentZoom = DEFAULT_ZOOM;
    void HandleZoom(float delta);
    void ProcessEvents(sf::Event& event);
    void AdjustViewToWindow();
    float deltaTime = 0.f;
    sf::RenderWindow window;
    sf::Font font;
    sf::View camera;  // Camera for game world
    sf::View uiView;  // View for UI elements
    HUD hud;
    std::unique_ptr<State> state;
    std::unique_ptr<NetworkManager> networkManager;
    std::shared_ptr<SettingsManager> settingsManager;
    std::shared_ptr<InputHandler> inputHandler;   
    InputManager inputManager;
    GameState currentState{GameState::MainMenu};
    bool steamInitialized{false};
    bool inLobby{false};
    CSteamID currentLobby{k_steamIDNil};
    std::string lobbyNameInput;
    CSteamID localSteamID{k_steamIDNil};
};

#endif // GAME_H