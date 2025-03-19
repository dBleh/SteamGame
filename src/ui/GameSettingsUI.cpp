#include "GameSettingsUI.h"
#include "../core/Game.h"
#include "../states/GameSettingsManager.h"
#include "../network/messages/SettingsMessageHandler.h"
#include "../states/PlayingState.h"  
#include <steam/steam_api.h>
#include <iostream>
#include <iomanip>
#include <sstream>

GameSettingsUI::GameSettingsUI(Game* game, GameSettingsManager* settingsManager)
    : game(game), 
      settingsManager(settingsManager),
      isVisible(false) {
          
    // Check if local player is host
    isHostPlayer = IsLocalPlayerHost();
    
    InitializeUI();
}


float GameSettingsUI::MapPositionToValue(const Slider& slider, float position) {
    float valueRange = slider.maxValue - slider.minValue;
    float trackWidth = slider.track.getSize().x;
    float handleSize = slider.handle.getSize().x;
    float trackLeft = slider.track.getPosition().x;
    float trackRight = trackLeft + trackWidth - handleSize;
    
    // Clamp position to track bounds
    position = std::max(trackLeft, std::min(trackRight, position));
    
    // Calculate normalized position within track
    float normalizedPosition = (position - trackLeft) / (trackWidth - handleSize);
    
    // Convert to actual value
    float value = slider.minValue + (normalizedPosition * valueRange);
    
    // Round to integer if needed
    if (slider.isIntegerOnly) {
        value = std::round(value);
    }
    
    return value;
}

void GameSettingsUI::UpdateSliderAppearance(Slider& slider, bool isActive) {
    if (isHostPlayer) {
        if (isActive) {
            slider.handle.setFillColor(sf::Color(0, 200, 255)); // Bright blue when active
            slider.handle.setOutlineColor(sf::Color::White);
            slider.handle.setOutlineThickness(2.0f);
        } else {
            slider.handle.setFillColor(sf::Color(0, 150, 255)); // Normal blue
            slider.handle.setOutlineColor(sf::Color::White);
            slider.handle.setOutlineThickness(1.0f);
        }
    } else {
        // Non-host players see gray sliders
        slider.handle.setFillColor(sf::Color(100, 100, 100));
    }
}

bool GameSettingsUI::IsSliderClicked(const Slider& slider, sf::Vector2f mousePos) {
    // Check if mouse is on handle or track
    return slider.handle.getGlobalBounds().contains(mousePos) || 
           slider.track.getGlobalBounds().contains(mousePos);
}

void GameSettingsUI::UpdateSliderValues() {
    // Update all sliders with current values from settings manager
    for (auto& slider : sliders) {
        GameSetting* setting = settingsManager->GetSetting(slider.settingName);
        if (setting) {
            slider.value = setting->value;
            
            // Update handle position
            float handleX = MapValueToPosition(slider, setting->value);
            slider.handle.setPosition(handleX, slider.handle.getPosition().y);
            
            // Update value text
            std::ostringstream valueStr;
            if (setting->isIntegerOnly) {
                valueStr << setting->GetIntValue();
            } else {
                valueStr << std::fixed << std::setprecision(1) << setting->GetFloatValue();
            }
            slider.valueText.setString(valueStr.str());
        }
    }
}

void GameSettingsUI::Update(float dt) {
    if (!isVisible) return;
    
    // Get mouse position and convert to UI coordinates
    sf::Vector2i mousePos = sf::Mouse::getPosition(game->GetWindow());
    sf::Vector2f mouseUiPos = game->WindowToUICoordinates(mousePos);
    
    // Update button hover states
    for (auto& [name, button] : buttons) {
        bool wasHovered = button.isHovered;
        button.isHovered = button.shape.getGlobalBounds().contains(mouseUiPos);
        
        // Update button appearance on hover change
        if (button.isHovered != wasHovered) {
            UpdateButtonAppearance(button, name);
        }
    }
}

void GameSettingsUI::Render(sf::RenderWindow& window) {
    if (!isVisible) return;
    
    // Save current view
    sf::View currentView = window.getView();
    
    // Set UI view for rendering settings UI
    window.setView(game->GetUIView());
    
    // Draw panel background
    window.draw(panelBackground);
    
    // Draw title
    window.draw(titleText);
    
    // Draw sliders
    for (const auto& slider : sliders) {
        window.draw(slider.track);
        window.draw(slider.handle);
        window.draw(slider.label);
        window.draw(slider.valueText);
    }
    
    // Draw buttons
    for (const auto& [name, button] : buttons) {
        window.draw(button.shape);
        window.draw(button.text);
    }
    
    // Draw "View Only" text for non-host
    if (!isHostPlayer) {
        sf::Text viewOnlyText;
        viewOnlyText.setFont(game->GetFont());
        viewOnlyText.setString("View Only - Only host can modify settings");
        viewOnlyText.setCharacterSize(18);
        viewOnlyText.setFillColor(sf::Color(255, 150, 150));
        
        float x = panelBackground.getPosition().x + 
                 (panelBackground.getSize().x - viewOnlyText.getLocalBounds().width) / 2.0f;
        float y = titleText.getPosition().y + 40.0f;
        
        viewOnlyText.setPosition(x, y);
        window.draw(viewOnlyText);
    }
    
    // Restore the original view
    window.setView(currentView);
}

bool GameSettingsUI::ProcessEvent(const sf::Event& event) {
    if (!isVisible) return false;
    
    // Handle mouse button events
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        // Convert mouse coordinates to UI view space
        sf::Vector2i mousePos(event.mouseButton.x, event.mouseButton.y);
        sf::Vector2f mouseUiPos = game->WindowToUICoordinates(mousePos);
        
        // Check if any button was clicked
        for (auto& [name, button] : buttons) {
            if (button.isHovered) {
                // Only trigger non-close buttons for host
                if (isHostPlayer || name == "close") {
                    button.onClick();
                }
                return true;
            }
        }
        
        // Check if any slider was clicked (only for host)
        if (isHostPlayer) {
            for (auto& slider : sliders) {
                if (IsSliderClicked(slider, mouseUiPos)) {
                    slider.isDragging = true;
                    UpdateSliderAppearance(slider, true);
                    
                    // Update value based on click position
                    float newValue = MapPositionToValue(slider, mouseUiPos.x);
                    slider.value = newValue;
                    
                    // Update handle position
                    slider.handle.setPosition(
                        MapValueToPosition(slider, newValue),
                        slider.handle.getPosition().y
                    );
                    
                    // Update value text
                    std::ostringstream valueStr;
                    if (slider.isIntegerOnly) {
                        valueStr << static_cast<int>(newValue);
                    } else {
                        valueStr << std::fixed << std::setprecision(1) << newValue;
                    }
                    slider.valueText.setString(valueStr.str());
                    
                    return true;
                }
            }
        }
        
        // Check if clicked inside the panel (consume event to prevent interactions behind)
        if (panelBackground.getGlobalBounds().contains(mouseUiPos)) {
            return true;
        }
    }
    else if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
        // Stop dragging any sliders
        for (auto& slider : sliders) {
            if (slider.isDragging) {
                slider.isDragging = false;
                UpdateSliderAppearance(slider, false);
                
                // Update the actual setting with the new value
                settingsManager->UpdateSetting(slider.settingName, slider.value);
            }
        }
    }
    else if (event.type == sf::Event::MouseMoved) {
        // Convert mouse coordinates to UI view space
        sf::Vector2i mousePos(event.mouseMove.x, event.mouseMove.y);
        sf::Vector2f mouseUiPos = game->WindowToUICoordinates(mousePos);
        
        // Update button hover states
        for (auto& [name, button] : buttons) {
            bool wasHovered = button.isHovered;
            button.isHovered = button.shape.getGlobalBounds().contains(mouseUiPos);
            
            // Update button appearance based on hover state
            if (button.isHovered != wasHovered) {
                UpdateButtonAppearance(button, name);
            }
        }
        
        // Update dragging sliders
        for (auto& slider : sliders) {
            if (slider.isDragging) {
                // Update value based on drag position
                float newValue = MapPositionToValue(slider, mouseUiPos.x);
                slider.value = newValue;
                
                // Update handle position
                slider.handle.setPosition(
                    MapValueToPosition(slider, newValue),
                    slider.handle.getPosition().y
                );
                
                // Update value text
                std::ostringstream valueStr;
                if (slider.isIntegerOnly) {
                    valueStr << static_cast<int>(newValue);
                } else {
                    valueStr << std::fixed << std::setprecision(1) << newValue;
                }
                slider.valueText.setString(valueStr.str());
            }
        }
    }
    else if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
        // Close the settings panel
        Hide();
        return true;
    }
    
    return false;
}

void GameSettingsUI::UpdateButtonAppearance(Button& button, const std::string& name) {
    if (button.isHovered) {
        button.shape.setOutlineThickness(2.0f);
        
        // Don't brighten disabled buttons for non-hosts
        if (isHostPlayer || name == "close") {
            sf::Color color = button.shape.getFillColor();
            button.shape.setFillColor(sf::Color(
                std::min(255, color.r + 30),
                std::min(255, color.g + 30),
                std::min(255, color.b + 30)
            ));
        }
    } else {
        button.shape.setOutlineThickness(1.0f);
        
        // Restore original color
        if (name == "apply") {
            button.shape.setFillColor(isHostPlayer ? sf::Color(0, 150, 0) : sf::Color(100, 100, 100));
        } else if (name == "reset") {
            button.shape.setFillColor(isHostPlayer ? sf::Color(150, 150, 0) : sf::Color(100, 100, 100));
        } else if (name == "close") {
            button.shape.setFillColor(sf::Color(150, 0, 0));
        }
    }
}

void GameSettingsUI::RefreshUI() {
    // Update sliders with current values from settings manager
    UpdateSliderValues();
}

void GameSettingsUI::ApplyChanges() {
    if (!isHostPlayer) return;
    
    // Apply all slider values to settings
    for (const auto& slider : sliders) {
        settingsManager->UpdateSetting(slider.settingName, slider.value);
    }
    
    // Apply settings to game
    settingsManager->ApplySettings();
    
    // Send settings update over network
    std::string settingsData = settingsManager->SerializeSettings();
    std::string updateMsg = SettingsMessageHandler::FormatSettingsUpdateMessage(settingsData);
    
    // Send via network
    if (game->GetNetworkManager().IsInitialized()) {
        game->GetNetworkManager().BroadcastMessage(updateMsg);
    }
    
    // Instead of trying to access the PlayingState directly, just send a custom message
    // that will be handled by the right components
    if (game->GetCurrentState() == GameState::Playing) {
        // Create a settings-update message
        std::string updateApplyMsg = SettingsMessageHandler::FormatSettingsApplyMessage();
        
        // Broadcast it to all clients
        if (game->GetNetworkManager().IsInitialized()) {
            game->GetNetworkManager().BroadcastMessage(updateApplyMsg);
        }
    }
}

bool GameSettingsUI::IsLocalPlayerHost() const {
    CSteamID myID = SteamUser()->GetSteamID();
    CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
    return myID == hostID;
}


void GameSettingsUI::InitializeUI() {
    // Calculate dimensions based on UI constants
    float panelWidth = 600.0f;
    float panelHeight = 600.0f;
    float panelX = (BASE_WIDTH - panelWidth) / 2.0f;
    float panelY = (BASE_HEIGHT - panelHeight) / 2.0f;
    
    // Set up the panel background
    panelBackground.setSize(sf::Vector2f(panelWidth, panelHeight));
    panelBackground.setPosition(sf::Vector2f(panelX, panelY));
    panelBackground.setFillColor(sf::Color(30, 30, 30, 230)); // Semi-transparent dark panel
    panelBackground.setOutlineColor(sf::Color(100, 100, 100));
    panelBackground.setOutlineThickness(2.0f);
    
    // Set up title text
    titleText.setFont(game->GetFont());
    titleText.setString("Game Settings");
    titleText.setCharacterSize(32);
    titleText.setFillColor(sf::Color::White);
    titleText.setPosition(
        panelX + (panelWidth - titleText.getLocalBounds().width) / 2.0f,
        panelY + 20.0f
    );
    
    // Create sliders for settings
    const auto& allSettings = settingsManager->GetAllSettings();
    float startY = panelY + 80.0f;
    float sliderSpacing = 50.0f;
    
    int sliderIndex = 0;
    for (const auto& [name, setting] : allSettings) {
        // Only add the first 8 settings to avoid overcrowding
        if (sliderIndex >= 8) break;
        
        CreateSlider(name, startY + sliderIndex * sliderSpacing);
        sliderIndex++;
    }
    
    // Add control buttons at the bottom
    float buttonWidth = 160.0f;
    float buttonHeight = 40.0f;
    float buttonY = panelY + panelHeight - buttonHeight - 20.0f;
    
    // Apply button
    Button applyButton;
    applyButton.shape.setSize(sf::Vector2f(buttonWidth, buttonHeight));
    applyButton.shape.setPosition(panelX + 30.0f, buttonY);
    applyButton.shape.setFillColor(sf::Color(0, 150, 0)); // Green
    applyButton.shape.setOutlineThickness(1.0f);
    applyButton.shape.setOutlineColor(sf::Color::White);
    
    applyButton.text.setFont(game->GetFont());
    applyButton.text.setString("Apply");
    applyButton.text.setCharacterSize(20);
    applyButton.text.setFillColor(sf::Color::White);
    
    // Center text in button
    sf::FloatRect bounds = applyButton.text.getLocalBounds();
    applyButton.text.setPosition(
        applyButton.shape.getPosition().x + (buttonWidth - bounds.width) / 2.0f,
        applyButton.shape.getPosition().y + (buttonHeight - bounds.height) / 2.0f - 5.0f
    );
    
    applyButton.onClick = [this]() {
        this->ApplyChanges();
    };
    
    buttons["apply"] = applyButton;
    
    // Reset button
    Button resetButton;
    resetButton.shape.setSize(sf::Vector2f(buttonWidth, buttonHeight));
    resetButton.shape.setPosition(panelX + (panelWidth - buttonWidth) / 2.0f, buttonY);
    resetButton.shape.setFillColor(sf::Color(150, 150, 0)); // Yellow
    resetButton.shape.setOutlineThickness(1.0f);
    resetButton.shape.setOutlineColor(sf::Color::White);
    
    resetButton.text.setFont(game->GetFont());
    resetButton.text.setString("Reset");
    resetButton.text.setCharacterSize(20);
    resetButton.text.setFillColor(sf::Color::White);
    
    // Center text in button
    bounds = resetButton.text.getLocalBounds();
    resetButton.text.setPosition(
        resetButton.shape.getPosition().x + (buttonWidth - bounds.width) / 2.0f,
        resetButton.shape.getPosition().y + (buttonHeight - bounds.height) / 2.0f - 5.0f
    );
    
    resetButton.onClick = [this]() {
        this->settingsManager->ResetToDefaults();
        this->RefreshUI();
    };
    
    buttons["reset"] = resetButton;
    
    // Close button
    Button closeButton;
    closeButton.shape.setSize(sf::Vector2f(buttonWidth, buttonHeight));
    closeButton.shape.setPosition(panelX + panelWidth - buttonWidth - 30.0f, buttonY);
    closeButton.shape.setFillColor(sf::Color(150, 0, 0)); // Red
    closeButton.shape.setOutlineThickness(1.0f);
    closeButton.shape.setOutlineColor(sf::Color::White);
    
    closeButton.text.setFont(game->GetFont());
    closeButton.text.setString("Close");
    closeButton.text.setCharacterSize(20);
    closeButton.text.setFillColor(sf::Color::White);
    
    // Center text in button
    bounds = closeButton.text.getLocalBounds();
    closeButton.text.setPosition(
        closeButton.shape.getPosition().x + (buttonWidth - bounds.width) / 2.0f,
        closeButton.shape.getPosition().y + (buttonHeight - bounds.height) / 2.0f - 5.0f
    );
    
    closeButton.onClick = [this]() {
        this->Hide();
    };
    
    buttons["close"] = closeButton;
    
    // Disable UI if not host
    if (!isHostPlayer) {
        for (auto& [name, button] : buttons) {
            if (name != "close") {
                button.shape.setFillColor(sf::Color(100, 100, 100)); // Gray out
            }
        }
    }
}

void GameSettingsUI::CreateSlider(const std::string& settingName, float y) {
    GameSetting* setting = settingsManager->GetSetting(settingName);
    if (!setting) return;
    
    Slider slider;
    slider.settingName = settingName;
    slider.minValue = setting->minValue;
    slider.maxValue = setting->maxValue;
    slider.value = setting->value;
    slider.isIntegerOnly = setting->isIntegerOnly;
    
    // Calculate positions
    float panelX = panelBackground.getPosition().x;
    float panelWidth = panelBackground.getSize().x;
    
    float trackWidth = 300.0f;
    float trackHeight = 10.0f;
    float handleSize = 20.0f;
    
    // Set up label
    slider.label.setFont(game->GetFont());
    slider.label.setString(setting->name);
    slider.label.setCharacterSize(16);
    slider.label.setFillColor(sf::Color::White);
    slider.label.setPosition(panelX + 30.0f, y);
    
    // Set up value text
    slider.valueText.setFont(game->GetFont());
    std::ostringstream valueStr;
    if (setting->isIntegerOnly) {
        valueStr << setting->GetIntValue();
    } else {
        valueStr << std::fixed << std::setprecision(1) << setting->GetFloatValue();
    }
    slider.valueText.setString(valueStr.str());
    slider.valueText.setCharacterSize(16);
    slider.valueText.setFillColor(sf::Color::White);
    slider.valueText.setPosition(
        panelX + panelWidth - 80.0f, 
        y
    );
    
    // Set up track
    slider.track.setSize(sf::Vector2f(trackWidth, trackHeight));
    slider.track.setPosition(panelX + 150.0f, y + 10.0f);
    slider.track.setFillColor(sf::Color(80, 80, 80));
    slider.track.setOutlineThickness(1.0f);
    slider.track.setOutlineColor(sf::Color(120, 120, 120));
    
    // Set up handle
    slider.handle.setSize(sf::Vector2f(handleSize, handleSize));
    float handleX = MapValueToPosition(slider, setting->value);
    slider.handle.setPosition(handleX, y + 5.0f);
    slider.handle.setFillColor(isHostPlayer ? sf::Color(0, 150, 255) : sf::Color(100, 100, 100));
    slider.handle.setOutlineThickness(1.0f);
    slider.handle.setOutlineColor(sf::Color::White);
    
    sliders.push_back(slider);
}

float GameSettingsUI::MapValueToPosition(const Slider& slider, float value) {
    float valueRange = slider.maxValue - slider.minValue;
    float trackWidth = slider.track.getSize().x;
    float handleSize = slider.handle.getSize().x;
    float trackLeft = slider.track.getPosition().x;
    
    // Calculate the position based on the value's proportion in the range
    float normalizedValue = (value - slider.minValue) / valueRange;
    float position = trackLeft + (normalizedValue * (trackWidth - handleSize));
    
    return position;
}