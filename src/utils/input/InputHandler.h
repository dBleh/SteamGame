#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>  // Add this for mouse input
#include <SFML/Window/Event.hpp>  // Add this for event handling
#include <unordered_map>
#include <functional>
#include <string>
#include <memory>

// Forward declarations
class SettingsManager;
struct GameSettings;

// Enum for different input actions
enum class InputAction {
    MoveUp,
    MoveDown,
    MoveLeft,
    MoveRight,
    Shoot,
    ShowLeaderboard,
    ShowMenu,
    ToggleGrid,
    ToggleCursorLock,
    ShowShop,  
};

// Class to handle input and process it according to the current game settings
class InputHandler {
public:
    InputHandler(std::shared_ptr<SettingsManager> settingsManager);
    ~InputHandler() = default;
    
    // Process an SFML event
    void ProcessEvent(const sf::Event& event);
    
    // Update method to be called each frame
    void Update();
    
    // Check if a specific action is being performed
    bool IsActionActive(InputAction action) const;
    
    // Check if a specific action was just triggered this frame
    bool IsActionTriggered(InputAction action) const;
    
    // Register a callback function to be called when an action is triggered
    void RegisterActionCallback(InputAction action, std::function<void()> callback);
    bool IsActionBoundToMouseButton(InputAction action, sf::Mouse::Button button) const;
    // Update key bindings from settings
    void UpdateKeyBindings();
    sf::Keyboard::Key GetKeyForAction(InputAction action) const;
    
    // Helper to convert InputAction to string for error messages
    std::string ActionToString(InputAction action) const;
private:
    std::shared_ptr<SettingsManager> settingsManager;
    const GameSettings* currentSettings;
    
    // State tracking for keyboard keys
    std::unordered_map<sf::Keyboard::Key, bool> keyState;
    std::unordered_map<sf::Keyboard::Key, bool> previousKeyState;
    
    // State tracking for mouse buttons
    std::unordered_map<sf::Mouse::Button, bool> mouseState;
    std::unordered_map<sf::Mouse::Button, bool> previousMouseState;
    
    // Action callbacks
    std::unordered_map<InputAction, std::function<void()>> actionCallbacks;
    
    // Function to get the key bound to a specific action
    
};

#endif // INPUT_HANDLER_H