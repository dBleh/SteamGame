#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include <SFML/Graphics.hpp>
#include <unordered_map>
#include <string>

enum class GameAction {
    MoveUp,
    MoveDown,
    MoveLeft,
    MoveRight,
    Shoot,
    ToggleReady,
    ToggleGrid,
    ToggleCursorLock,
    ShowLeaderboard,
    OpenMenu,
    // Add more actions as needed
};

class InputManager {
public:
    InputManager();
    ~InputManager() = default;

    // Get the key for a specific action
    sf::Keyboard::Key GetKeyBinding(GameAction action) const;
    
    // Set a new key binding for an action
    void SetKeyBinding(GameAction action, sf::Keyboard::Key key);
    
    // Reset all key bindings to defaults
    void ResetToDefaults();

    // Check if an action is triggered by a key press
    bool IsActionTriggered(GameAction action, const sf::Event& event) const;
    
    // Check if a key is currently pressed
    bool IsKeyPressed(GameAction action) const;

private:
    // Map of actions to key bindings
    std::unordered_map<GameAction, sf::Keyboard::Key> keyBindings;
    
    // Initialize default bindings
    void InitializeDefaultBindings();
};

#endif // INPUT_MANAGER_H