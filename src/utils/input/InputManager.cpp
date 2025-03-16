#include "InputManager.h"
#include <iostream>

InputManager::InputManager() {
    InitializeDefaultBindings();
}

void InputManager::InitializeDefaultBindings() {
    // Set default key bindings
    keyBindings[GameAction::MoveUp] = sf::Keyboard::W;
    keyBindings[GameAction::MoveDown] = sf::Keyboard::S;
    keyBindings[GameAction::MoveLeft] = sf::Keyboard::A;
    keyBindings[GameAction::MoveRight] = sf::Keyboard::D;
    keyBindings[GameAction::Shoot] = sf::Keyboard::Space;
    keyBindings[GameAction::ToggleReady] = sf::Keyboard::R;
    keyBindings[GameAction::ToggleGrid] = sf::Keyboard::G;
    keyBindings[GameAction::ToggleCursorLock] = sf::Keyboard::L;
    keyBindings[GameAction::ShowLeaderboard] = sf::Keyboard::Tab;
    keyBindings[GameAction::OpenMenu] = sf::Keyboard::Escape;
}

sf::Keyboard::Key InputManager::GetKeyBinding(GameAction action) const {
    auto it = keyBindings.find(action);
    if (it != keyBindings.end()) {
        return it->second;
    }
    
    // Return an invalid key if the action is not found
    std::cerr << "[ERROR] Key binding for action " << static_cast<int>(action) << " not found!" << std::endl;
    return sf::Keyboard::Unknown;
}

void InputManager::SetKeyBinding(GameAction action, sf::Keyboard::Key key) {
    keyBindings[action] = key;
    std::cout << "[INPUT] Set key binding for action " << static_cast<int>(action) 
              << " to key " << static_cast<int>(key) << std::endl;
}

void InputManager::ResetToDefaults() {
    keyBindings.clear();
    InitializeDefaultBindings();
    std::cout << "[INPUT] Reset all key bindings to defaults" << std::endl;
}

bool InputManager::IsActionTriggered(GameAction action, const sf::Event& event) const {
    if (event.type != sf::Event::KeyPressed) {
        return false;
    }
    
    auto it = keyBindings.find(action);
    if (it != keyBindings.end()) {
        return event.key.code == it->second;
    }
    
    return false;
}

bool InputManager::IsKeyPressed(GameAction action) const {
    auto it = keyBindings.find(action);
    if (it != keyBindings.end()) {
        return sf::Keyboard::isKeyPressed(it->second);
    }
    
    return false;
}