#include "InputHandler.h" // Include the settings manager header
#include "../config/SettingsManager.h" // Include the settings manager header
InputHandler::InputHandler(std::shared_ptr<SettingsManager> settingsManager)
    : settingsManager(settingsManager), currentSettings(&settingsManager->GetSettings()) {
    // Initialize key states
    UpdateKeyBindings();
}

void InputHandler::UpdateKeyBindings() {
    // Update the settings reference
    currentSettings = &settingsManager->GetSettings();
    
    // Clear the key states when bindings change
    keyState.clear();
    previousKeyState.clear();
    mouseState.clear();
    previousMouseState.clear();
    
    // Initialize key states for all bound keys
    // We want to track all keys used in bindings
    keyState[currentSettings->moveUp] = false;
    keyState[currentSettings->moveDown] = false;
    keyState[currentSettings->moveLeft] = false;
    keyState[currentSettings->moveRight] = false;
    
    // Only add the shoot key if it's a keyboard key
    if (currentSettings->shoot != sf::Keyboard::Unknown) {
        keyState[currentSettings->shoot] = false;
    }
    keyState[currentSettings->showShop] = false;
    keyState[currentSettings->showLeaderboard] = false;
    keyState[currentSettings->showMenu] = false;
    keyState[currentSettings->toggleGrid] = false;
    keyState[currentSettings->toggleCursorLock] = false;
    
    // Set up the same keys in the previous state map
    previousKeyState = keyState;
    
    // Initialize mouse states - always track primary buttons
    mouseState[sf::Mouse::Left] = false;
    mouseState[sf::Mouse::Right] = false;
    mouseState[sf::Mouse::Middle] = false;
    
    // Set up the same buttons in the previous state map
    previousMouseState = mouseState;
}

void InputHandler::ProcessEvent(const sf::Event& event) {
    // Process keyboard events
    if (event.type == sf::Event::KeyPressed) {
        auto it = keyState.find(event.key.code);
        if (it != keyState.end()) {
            it->second = true;
        }
    } else if (event.type == sf::Event::KeyReleased) {
        auto it = keyState.find(event.key.code);
        if (it != keyState.end()) {
            it->second = false;
        }
    }
    
    // Process mouse events
    else if (event.type == sf::Event::MouseButtonPressed) {
        auto it = mouseState.find(event.mouseButton.button);
        if (it != mouseState.end()) {
            it->second = true;
        }
    } else if (event.type == sf::Event::MouseButtonReleased) {
        auto it = mouseState.find(event.mouseButton.button);
        if (it != mouseState.end()) {
            it->second = false;
        }
    }
}

void InputHandler::Update() {
    // Trigger callbacks for newly activated actions
    for (const auto& [action, callback] : actionCallbacks) {
        if (IsActionTriggered(action)) {
            callback();
        }
    }
    
    // Update previous states
    previousKeyState = keyState;
    previousMouseState = mouseState;
}

bool InputHandler::IsActionActive(InputAction action) const {
    // Special case for shoot, which can be a key OR mouse button, but not both
    if (action == InputAction::Shoot) {
        // First, check if shoot is bound to a keyboard key
        if (currentSettings->shoot != sf::Keyboard::Unknown) {
            auto it = keyState.find(currentSettings->shoot);
            if (it != keyState.end()) {
                return it->second;
            }
            return false; // Key not found in state map
        }
        
        // If no keyboard key is bound, default to mouse left button
        else {
            auto it = mouseState.find(sf::Mouse::Left);
            if (it != mouseState.end()) {
                return it->second;
            }
            return false; // Mouse button not found in state map
        }
    }
    
    // Regular key-based actions
    sf::Keyboard::Key key = GetKeyForAction(action);
    auto it = keyState.find(key);
    if (it != keyState.end()) {
        return it->second;
    }
    
    return false;
}


bool InputHandler::IsActionTriggered(InputAction action) const {
    // An action is triggered if it's active now but wasn't in the previous frame
    
    // Special case for shoot, which can be a key OR mouse button, but not both
    if (action == InputAction::Shoot) {
        // If shoot is bound to a key
        if (currentSettings->shoot != sf::Keyboard::Unknown) {
            auto currIt = keyState.find(currentSettings->shoot);
            auto prevIt = previousKeyState.find(currentSettings->shoot);
            
            if (currIt != keyState.end() && prevIt != previousKeyState.end()) {
                return currIt->second && !prevIt->second;
            }
            return false;
        }
        
        // Only use mouse left button if no key is bound
        else {
            auto currIt = mouseState.find(sf::Mouse::Left);
            auto prevIt = previousMouseState.find(sf::Mouse::Left);
            
            if (currIt != mouseState.end() && prevIt != previousMouseState.end()) {
                return currIt->second && !prevIt->second;
            }
            return false;
        }
    }
    
    // Regular key-based actions
    sf::Keyboard::Key key = GetKeyForAction(action);
    
    auto currIt = keyState.find(key);
    auto prevIt = previousKeyState.find(key);
    
    if (currIt != keyState.end() && prevIt != previousKeyState.end()) {
        return currIt->second && !prevIt->second;
    }
    
    return false;
}

void InputHandler::RegisterActionCallback(InputAction action, std::function<void()> callback) {
    actionCallbacks[action] = callback;
}

sf::Keyboard::Key InputHandler::GetKeyForAction(InputAction action) const {
    switch (action) {
        case InputAction::MoveUp:
            return currentSettings->moveUp;
        case InputAction::MoveDown:
            return currentSettings->moveDown;
        case InputAction::MoveLeft:
            return currentSettings->moveLeft;
        case InputAction::MoveRight:
            return currentSettings->moveRight;
        case InputAction::Shoot:
            return currentSettings->shoot;
        case InputAction::ShowLeaderboard:
            return currentSettings->showLeaderboard;
        case InputAction::ShowShop:
            return currentSettings->showShop;
        case InputAction::ShowMenu:
            return currentSettings->showMenu;
        case InputAction::ToggleGrid:
            return currentSettings->toggleGrid;
        case InputAction::ToggleCursorLock:
            return currentSettings->toggleCursorLock;
        default:
            return sf::Keyboard::Unknown;
    }
}
// Function to check if an action is bound to a specific mouse button
bool InputHandler::IsActionBoundToMouseButton(InputAction action, sf::Mouse::Button button) const {
    if (action == InputAction::Shoot) {
        sf::Keyboard::Key actionKey = GetKeyForAction(action);
        if (actionKey == sf::Keyboard::Unknown) {
            // The key is Unknown, which means it might be a mouse button
            std::string keyStr = settingsManager->KeyToString(actionKey);
            
            if (keyStr.substr(0, 5) == "Mouse") {
                std::string buttonStr = keyStr.substr(5);
                
                if (buttonStr == "Left" && button == sf::Mouse::Left) return true;
                if (buttonStr == "Right" && button == sf::Mouse::Right) return true;
                if (buttonStr == "Middle" && button == sf::Mouse::Middle) return true;
                
                // For other mouse buttons
                try {
                    int buttonNum = std::stoi(buttonStr);
                    return buttonNum == static_cast<int>(button);
                } catch (...) {
                    return false;
                }
            }
        }
    }
    return false;
}
std::string InputHandler::ActionToString(InputAction action) const {
    switch (action) {
        case InputAction::MoveUp:
            return "MoveUp";
        case InputAction::MoveDown:
            return "MoveDown";
        case InputAction::MoveLeft:
            return "MoveLeft";
        case InputAction::MoveRight:
            return "MoveRight";
        case InputAction::Shoot:
            return "Shoot";
        case InputAction::ShowLeaderboard:
            return "ShowLeaderboard";
        case InputAction::ShowMenu:
            return "ShowMenu";
        case InputAction::ToggleGrid:
            return "ToggleGrid";
        case InputAction::ToggleCursorLock:
            return "ToggleCursorLock";
        default:
            return "Unknown";
    }
}