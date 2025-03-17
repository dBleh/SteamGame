#ifndef SETTINGS_MANAGER_H
#define SETTINGS_MANAGER_H

#include <SFML/Window/Keyboard.hpp>
#include <string>
#include <unordered_map>
#include <fstream>
#include <iostream>

// Structure to hold game settings
struct GameSettings {
    // Input key bindings
    sf::Keyboard::Key moveUp = sf::Keyboard::W;
    sf::Keyboard::Key moveDown = sf::Keyboard::S;
    sf::Keyboard::Key moveLeft = sf::Keyboard::A;
    sf::Keyboard::Key moveRight = sf::Keyboard::D;
    sf::Keyboard::Key shoot = sf::Keyboard::Unknown; // Mouse left button by default
    sf::Keyboard::Key showLeaderboard = sf::Keyboard::Tab;
    sf::Keyboard::Key showMenu = sf::Keyboard::Escape;
    sf::Keyboard::Key toggleGrid = sf::Keyboard::G;
    sf::Keyboard::Key toggleCursorLock = sf::Keyboard::L;
    sf::Keyboard::Key toggleReady = sf::Keyboard::R;
    sf::Keyboard::Key showShop = sf::Keyboard::B; 
    
    // Other settings can be added here in the future
    bool showFPS = true;
    int volumeLevel = 100;
};

class SettingsManager {
public:
    SettingsManager();
    ~SettingsManager() = default;
    
    // Load settings from file
    bool LoadSettings();
    
    // Save settings to file
    bool SaveSettings();
    
    // Get the current settings
    const GameSettings& GetSettings() const { return settings; }
    
    // Update a specific key binding
    void SetKeyBinding(const std::string& action, sf::Keyboard::Key key);
    
    // Convert between key codes and readable names
    static std::string KeyToString(sf::Keyboard::Key key);
    static sf::Keyboard::Key StringToKey(const std::string& keyString);
    
private:
    GameSettings settings;
    std::string settingsFilePath = "settings.cfg";
};

// Implementation



#endif // SETTINGS_MANAGER_H