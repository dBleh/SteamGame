#include "GameSettingsUI.h"
#include "../core/Game.h"
#include "../states/GameSettingsManager.h"
#include "../network/messages/SettingsMessageHandler.h"
#include "../states/PlayingState.h"  
#include <steam/steam_api.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cctype> // for isdigit and isprint

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
    
    // Update value text boxes hover states
    for (auto& slider : sliders) {
        bool wasHovered = slider.valueTextHovered;
        slider.valueTextHovered = slider.valueTextBackground.getGlobalBounds().contains(mouseUiPos);
        
        // Update value text appearance on hover change
        if (slider.valueTextHovered != wasHovered) {
            if (slider.valueTextHovered && isHostPlayer) {
                slider.valueTextBackground.setOutlineColor(sf::Color::White);
                slider.valueTextBackground.setOutlineThickness(2.0f);
            } else {
                slider.valueTextBackground.setOutlineColor(sf::Color(120, 120, 120));
                slider.valueTextBackground.setOutlineThickness(1.0f);
            }
        }
    }
}



bool GameSettingsUI::ProcessEvent(const sf::Event& event) {
    if (!isVisible) return false;
    
    if (dialogMode != DialogMode::None) {
        return ProcessDialogEvent(event);
    }
    
    // The rest of the ProcessEvent method remains unchanged...
    // Handle text input for sliders
    if (event.type == sf::Event::TextEntered && isHostPlayer) {
        for (auto& slider : sliders) {
            if (slider.isEditing) {
                // Allow only valid numerical input
                if (event.text.unicode == 8) { // Backspace
                    if (!slider.editingText.isEmpty()) {
                        slider.editingText.erase(slider.editingText.getSize() - 1);
                    }
                }
                else if (event.text.unicode == 13 || event.text.unicode == 27) { // Enter or Escape
                    if (event.text.unicode == 13) { // Enter - apply value
                        ApplyTextInput(slider);
                    }
                    // Exit editing mode in both cases
                    slider.isEditing = false;
                    slider.valueTextBackground.setFillColor(sf::Color(60, 60, 60));
                }
                else if (event.text.unicode == 46 || (event.text.unicode >= 48 && event.text.unicode <= 57)) { // Decimal point or digits
                    // Only allow one decimal point
                    if (event.text.unicode == 46 && slider.isIntegerOnly) {
                        // Don't accept decimal point for integer values
                    }
                    else if (event.text.unicode == 46 && slider.editingText.toAnsiString().find('.') != std::string::npos) {
                        // Already has a decimal point
                    }
                    else {
                        slider.editingText += static_cast<char>(event.text.unicode);
                    }
                }
                else if (event.text.unicode == 45 && slider.editingText.isEmpty()) { // Minus sign at beginning only
                    if (slider.minValue < 0) { // Only allow minus if min value is negative
                        slider.editingText += '-';
                    }
                }
                
                // Update display text
                slider.valueText.setString(slider.editingText);
                return true;
            }
        }
    }
    
    // Handle key presses for text editing
    if (event.type == sf::Event::KeyPressed && isHostPlayer) {
        for (auto& slider : sliders) {
            if (slider.isEditing) {
                if (event.key.code == sf::Keyboard::Return) {
                    ApplyTextInput(slider);
                    slider.isEditing = false;
                    slider.valueTextBackground.setFillColor(sf::Color(60, 60, 60));
                    return true;
                }
                else if (event.key.code == sf::Keyboard::Escape) {
                    // Cancel editing, restore previous value
                    std::ostringstream valueStr;
                    if (slider.isIntegerOnly) {
                        valueStr << static_cast<int>(slider.value);
                    } else {
                        valueStr << std::fixed << std::setprecision(1) << slider.value;
                    }
                    slider.valueText.setString(valueStr.str());
                    slider.isEditing = false;
                    slider.valueTextBackground.setFillColor(sf::Color(60, 60, 60));
                    return true;
                }
            }
        }
    }
    
    // Handle mouse button events
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        // Convert mouse coordinates to UI view space
        sf::Vector2i mousePos(event.mouseButton.x, event.mouseButton.y);
        sf::Vector2f mouseUiPos = game->WindowToUICoordinates(mousePos);
        
        // Check if any text field was clicked (only for host)
        if (isHostPlayer) {
            bool valueTextClicked = false;
            for (auto& slider : sliders) {
                // First, clear any previous text editing
                if (slider.isEditing) {
                    // Apply the current value before switching
                    ApplyTextInput(slider);
                    slider.isEditing = false;
                    slider.valueTextBackground.setFillColor(sf::Color(60, 60, 60));
                }
                
                // Check if this slider's value text was clicked
                if (slider.valueTextBackground.getGlobalBounds().contains(mouseUiPos)) {
                    valueTextClicked = true;
                    slider.isEditing = true;
                    slider.valueTextBackground.setFillColor(sf::Color(90, 90, 90));
                    slider.editingText = slider.valueText.getString();
                    return true;
                }
            }
            
            // If a value text was clicked, don't process other UI elements
            if (valueTextClicked) {
                return true;
            }
        }
        
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
        
        if (prevPageButton.shape.getGlobalBounds().contains(mouseUiPos)) {
            if (currentPage > 0) {
                NavigateToPage(currentPage - 1);
            }
            return true;
        }
        
        if (nextPageButton.shape.getGlobalBounds().contains(mouseUiPos)) {
            if (currentPage < totalPages - 1) {
                NavigateToPage(currentPage + 1);
            }
            return true;
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
        
        // Update value text boxes hover states
        for (auto& slider : sliders) {
            bool wasHovered = slider.valueTextHovered;
            slider.valueTextHovered = slider.valueTextBackground.getGlobalBounds().contains(mouseUiPos);
            
            // Update value text appearance on hover change
            if (slider.valueTextHovered != wasHovered && !slider.isEditing) {
                if (slider.valueTextHovered && isHostPlayer) {
                    slider.valueTextBackground.setOutlineColor(sf::Color::White);
                    slider.valueTextBackground.setOutlineThickness(2.0f);
                } else {
                    slider.valueTextBackground.setOutlineColor(sf::Color(120, 120, 120));
                    slider.valueTextBackground.setOutlineThickness(1.0f);
                }
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
        // First check if any slider is in edit mode
        bool anyEditing = false;
        for (auto& slider : sliders) {
            if (slider.isEditing) {
                // Cancel editing, restore previous value
                std::ostringstream valueStr;
                if (slider.isIntegerOnly) {
                    valueStr << static_cast<int>(slider.value);
                } else {
                    valueStr << std::fixed << std::setprecision(1) << slider.value;
                }
                slider.valueText.setString(valueStr.str());
                slider.isEditing = false;
                slider.valueTextBackground.setFillColor(sf::Color(60, 60, 60));
                anyEditing = true;
            }
        }
        
        // If no slider was editing, close the panel
        if (!anyEditing) {
            Hide();
        }
        
        return true;
    }
    
    return false;
}

void GameSettingsUI::ApplyTextInput(Slider& slider) {
    std::string valueStr = slider.editingText.toAnsiString();
    if (valueStr.empty() || valueStr == "-" || valueStr == ".") {
        // Invalid input, reset to current value
        std::ostringstream validValueStr;
        if (slider.isIntegerOnly) {
            validValueStr << static_cast<int>(slider.value);
        } else {
            validValueStr << std::fixed << std::setprecision(1) << slider.value;
        }
        slider.valueText.setString(validValueStr.str());
        return;
    }
    
    try {
        float newValue;
        if (slider.isIntegerOnly) {
            int intValue = std::stoi(valueStr);
            newValue = static_cast<float>(intValue);
        } else {
            newValue = std::stof(valueStr);
        }
        
        // Clamp value to min/max
        newValue = std::max(slider.minValue, std::min(slider.maxValue, newValue));
        
        // Update slider with new value
        slider.value = newValue;
        
        // Format display value
        std::ostringstream formattedValueStr;
        if (slider.isIntegerOnly) {
            formattedValueStr << static_cast<int>(newValue);
        } else {
            formattedValueStr << std::fixed << std::setprecision(1) << newValue;
        }
        slider.valueText.setString(formattedValueStr.str());
        
        // Update handle position
        float handleX = MapValueToPosition(slider, newValue);
        slider.handle.setPosition(handleX, slider.handle.getPosition().y);
        
        // Update the actual setting
        settingsManager->UpdateSetting(slider.settingName, newValue);
    }
    catch (const std::exception& e) {
        // Invalid number format, reset to current value
        std::ostringstream validValueStr;
        if (slider.isIntegerOnly) {
            validValueStr << static_cast<int>(slider.value);
        } else {
            validValueStr << std::fixed << std::setprecision(1) << slider.value;
        }
        slider.valueText.setString(validValueStr.str());
    }
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
    // Clear existing sliders
    sliders.clear();
    
    // Get all settings
    const auto& allSettings = settingsManager->GetAllSettings();
    
    // Calculate start and end indices for the current page
    int startIndex = currentPage * settingsPerPage;
    int endIndex = std::min(startIndex + settingsPerPage, static_cast<int>(allSettings.size()));
    
    // Calculate starting Y position
    float panelY = panelBackground.getPosition().y;
    float startY = panelY + 80.0f;
    float sliderSpacing = 50.0f;
    
    // Convert map to vector for indexing
    std::vector<std::pair<std::string, GameSetting>> settingsVector;
    for (const auto& pair : allSettings) {
        settingsVector.push_back(pair);
    }
    
    // Create sliders for current page
    for (int i = startIndex; i < endIndex; i++) {
        if (i < settingsVector.size()) {
            CreateSlider(settingsVector[i].first, startY + (i - startIndex) * sliderSpacing);
        }
    }
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

    CalculateTotalPages();
    float navButtonWidth = 80.0f;
    float navButtonHeight = 30.0f;
    float navButtonY = panelY + panelHeight - navButtonHeight - 70.0f; // Above the main buttons
    
    // Previous page button
    prevPageButton.shape.setSize(sf::Vector2f(navButtonWidth, navButtonHeight));
    prevPageButton.shape.setPosition(panelX + 30.0f, navButtonY);
    prevPageButton.shape.setFillColor(sf::Color(50, 100, 200));
    prevPageButton.shape.setOutlineThickness(1.0f);
    prevPageButton.shape.setOutlineColor(sf::Color::White);
    
    prevPageButton.text.setFont(game->GetFont());
    prevPageButton.text.setString("< Prev");
    prevPageButton.text.setCharacterSize(16);
    prevPageButton.text.setFillColor(sf::Color::White);
    
    // Center text in button
    sf::FloatRect bounds = prevPageButton.text.getLocalBounds();
    prevPageButton.text.setPosition(
        prevPageButton.shape.getPosition().x + (navButtonWidth - bounds.width) / 2.0f,
        prevPageButton.shape.getPosition().y + (navButtonHeight - bounds.height) / 2.0f - 5.0f
    );
    
    prevPageButton.onClick = [this]() {
        if (currentPage > 0) {
            NavigateToPage(currentPage - 1);
        }
    };
    
    // Next page button
    nextPageButton.shape.setSize(sf::Vector2f(navButtonWidth, navButtonHeight));
    nextPageButton.shape.setPosition(panelX + panelWidth - navButtonWidth - 30.0f, navButtonY);
    nextPageButton.shape.setFillColor(sf::Color(50, 100, 200));
    nextPageButton.shape.setOutlineThickness(1.0f);
    nextPageButton.shape.setOutlineColor(sf::Color::White);
    
    nextPageButton.text.setFont(game->GetFont());
    nextPageButton.text.setString("Next >");
    nextPageButton.text.setCharacterSize(16);
    nextPageButton.text.setFillColor(sf::Color::White);
    
    // Center text in button
    bounds = nextPageButton.text.getLocalBounds();
    nextPageButton.text.setPosition(
        nextPageButton.shape.getPosition().x + (navButtonWidth - bounds.width) / 2.0f,
        nextPageButton.shape.getPosition().y + (navButtonHeight - bounds.height) / 2.0f - 5.0f
    );
    
    nextPageButton.onClick = [this]() {
        if (currentPage < totalPages - 1) {
            NavigateToPage(currentPage + 1);
        }
    };
    
    // Page indicator text
    pageIndicatorText.setFont(game->GetFont());
    pageIndicatorText.setCharacterSize(16);
    pageIndicatorText.setFillColor(sf::Color::White);
    pageIndicatorText.setPosition(
        panelX + (panelWidth - pageIndicatorText.getLocalBounds().width) / 2.0f,
        navButtonY + 5.0f
    );
    
    // Update page indicator
    UpdatePageIndicator();
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
    float buttonWidth = 100.0f; // Reduced width to fit more buttons
    float buttonHeight = 40.0f;
    float buttonY = panelY + panelHeight - buttonHeight - 20.0f;
    float buttonSpacing = 20.0f;
    float currentX = panelX + 30.0f;
    
    // Apply button
    Button applyButton;
    applyButton.shape.setSize(sf::Vector2f(buttonWidth, buttonHeight));
    applyButton.shape.setPosition(currentX, buttonY);
    applyButton.shape.setFillColor(sf::Color(0, 150, 0)); // Green
    applyButton.shape.setOutlineThickness(1.0f);
    applyButton.shape.setOutlineColor(sf::Color::White);
    
    applyButton.text.setFont(game->GetFont());
    applyButton.text.setString("Apply");
    applyButton.text.setCharacterSize(20);
    applyButton.text.setFillColor(sf::Color::White);
    
    // Center text in button
    bounds = applyButton.text.getLocalBounds();
    applyButton.text.setPosition(
        applyButton.shape.getPosition().x + (buttonWidth - bounds.width) / 2.0f,
        applyButton.shape.getPosition().y + (buttonHeight - bounds.height) / 2.0f - 5.0f
    );
    
    applyButton.onClick = [this]() {
        this->ApplyChanges();
    };
    
    buttons["apply"] = applyButton;
    currentX += buttonWidth + buttonSpacing;
    
    // Reset button
    Button resetButton;
    resetButton.shape.setSize(sf::Vector2f(buttonWidth, buttonHeight));
    resetButton.shape.setPosition(currentX, buttonY);
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
    currentX += buttonWidth + buttonSpacing;
    
    // Save button
    Button saveButton;
    saveButton.shape.setSize(sf::Vector2f(buttonWidth, buttonHeight));
    saveButton.shape.setPosition(currentX, buttonY);
    saveButton.shape.setFillColor(sf::Color(0, 120, 180)); // Blue
    saveButton.shape.setOutlineThickness(1.0f);
    saveButton.shape.setOutlineColor(sf::Color::White);
    
    saveButton.text.setFont(game->GetFont());
    saveButton.text.setString("Save");
    saveButton.text.setCharacterSize(20);
    saveButton.text.setFillColor(sf::Color::White);
    
    // Center text in button
    bounds = saveButton.text.getLocalBounds();
    saveButton.text.setPosition(
        saveButton.shape.getPosition().x + (buttonWidth - bounds.width) / 2.0f,
        saveButton.shape.getPosition().y + (buttonHeight - bounds.height) / 2.0f - 5.0f
    );
    
    saveButton.onClick = [this]() {
        ShowSaveDialog();
    };
    
    buttons["save"] = saveButton;
    currentX += buttonWidth + buttonSpacing;
    
    // Load button
    Button loadButton;
    loadButton.shape.setSize(sf::Vector2f(buttonWidth, buttonHeight));
    loadButton.shape.setPosition(currentX, buttonY);
    loadButton.shape.setFillColor(sf::Color(0, 150, 100)); // Green
    loadButton.shape.setOutlineThickness(1.0f);
    loadButton.shape.setOutlineColor(sf::Color::White);
    
    loadButton.text.setFont(game->GetFont());
    loadButton.text.setString("Load");
    loadButton.text.setCharacterSize(20);
    loadButton.text.setFillColor(sf::Color::White);
    
    // Center text in button
    bounds = loadButton.text.getLocalBounds();
    loadButton.text.setPosition(
        loadButton.shape.getPosition().x + (buttonWidth - bounds.width) / 2.0f,
        loadButton.shape.getPosition().y + (buttonHeight - bounds.height) / 2.0f - 5.0f
    );
    
    loadButton.onClick = [this]() {
        ShowLoadDialog();
    };
    
    buttons["load"] = loadButton;
    currentX += buttonWidth + buttonSpacing;
    
    // Close button
    Button closeButton;
    closeButton.shape.setSize(sf::Vector2f(buttonWidth, buttonHeight));
    closeButton.shape.setPosition(currentX, buttonY);
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
    
    // If not host, disable buttons that should only be available to host
    if (!isHostPlayer) {
        buttons["apply"].shape.setFillColor(sf::Color(100, 100, 100)); // Gray out
        buttons["reset"].shape.setFillColor(sf::Color(100, 100, 100)); // Gray out
        buttons["save"].shape.setFillColor(sf::Color(100, 100, 100)); // Gray out
        buttons["load"].shape.setFillColor(sf::Color(100, 100, 100)); // Gray out
    }
    
    // Initialize dialog UI elements
    float dialogWidth = 400.0f;
    float dialogHeight = 500.0f;
    float dialogX = (BASE_WIDTH - dialogWidth) / 2.0f;
    float dialogY = (BASE_HEIGHT - dialogHeight) / 2.0f;
    
    dialogBackground.setSize(sf::Vector2f(dialogWidth, dialogHeight));
    dialogBackground.setPosition(sf::Vector2f(dialogX, dialogY));
    dialogBackground.setFillColor(sf::Color(40, 40, 40, 250));
    dialogBackground.setOutlineColor(sf::Color(100, 100, 100));
    dialogBackground.setOutlineThickness(2.0f);
    
    dialogTitleText.setFont(game->GetFont());
    dialogTitleText.setCharacterSize(24);
    dialogTitleText.setFillColor(sf::Color::White);
    dialogTitleText.setPosition(
        dialogX + 20.0f,
        dialogY + 20.0f
    );
    
    // File name input box
    float inputBoxWidth = dialogWidth - 40.0f;
    float inputBoxHeight = 30.0f;
    float inputBoxX = dialogX + 20.0f;
    float inputBoxY = dialogY + 70.0f;
    
    fileNameBox.setSize(sf::Vector2f(inputBoxWidth, inputBoxHeight));
    fileNameBox.setPosition(sf::Vector2f(inputBoxX, inputBoxY));
    fileNameBox.setFillColor(sf::Color(60, 60, 60));
    fileNameBox.setOutlineColor(sf::Color(120, 120, 120));
    fileNameBox.setOutlineThickness(1.0f);
    
    fileNameText.setFont(game->GetFont());
    fileNameText.setCharacterSize(16);
    fileNameText.setFillColor(sf::Color::White);
    fileNameText.setPosition(
        inputBoxX + 10.0f,
        inputBoxY + 5.0f
    );
    
    // Dialog buttons
    float dialogButtonWidth = 120.0f;
    float dialogButtonHeight = 40.0f;
    float dialogButtonY = dialogY + dialogHeight - dialogButtonHeight - 20.0f;
    
    // Save button in dialog
    dialogSaveButton.shape.setSize(sf::Vector2f(dialogButtonWidth, dialogButtonHeight));
    dialogSaveButton.shape.setPosition(
        dialogX + 20.0f,
        dialogButtonY
    );
    dialogSaveButton.shape.setFillColor(sf::Color(0, 150, 0));
    dialogSaveButton.shape.setOutlineThickness(1.0f);
    dialogSaveButton.shape.setOutlineColor(sf::Color::White);
    
    dialogSaveButton.text.setFont(game->GetFont());
    dialogSaveButton.text.setString("Save");
    dialogSaveButton.text.setCharacterSize(18);
    dialogSaveButton.text.setFillColor(sf::Color::White);
    
    // Center text in button
    bounds = dialogSaveButton.text.getLocalBounds();
    dialogSaveButton.text.setPosition(
        dialogSaveButton.shape.getPosition().x + (dialogButtonWidth - bounds.width) / 2.0f,
        dialogSaveButton.shape.getPosition().y + (dialogButtonHeight - bounds.height) / 2.0f - 5.0f
    );
    
    // Load button in dialog
    dialogLoadButton.shape.setSize(sf::Vector2f(dialogButtonWidth, dialogButtonHeight));
    dialogLoadButton.shape.setPosition(
        dialogX + 20.0f,
        dialogButtonY
    );
    dialogLoadButton.shape.setFillColor(sf::Color(0, 150, 0));
    dialogLoadButton.shape.setOutlineThickness(1.0f);
    dialogLoadButton.shape.setOutlineColor(sf::Color::White);
    
    dialogLoadButton.text.setFont(game->GetFont());
    dialogLoadButton.text.setString("Load");
    dialogLoadButton.text.setCharacterSize(18);
    dialogLoadButton.text.setFillColor(sf::Color::White);
    
    // Center text in button
    bounds = dialogLoadButton.text.getLocalBounds();
    dialogLoadButton.text.setPosition(
        dialogLoadButton.shape.getPosition().x + (dialogButtonWidth - bounds.width) / 2.0f,
        dialogLoadButton.shape.getPosition().y + (dialogButtonHeight - bounds.height) / 2.0f - 5.0f
    );
    
    // Cancel button in dialog
    dialogCancelButton.shape.setSize(sf::Vector2f(dialogButtonWidth, dialogButtonHeight));
    dialogCancelButton.shape.setPosition(
        dialogX + dialogWidth - dialogButtonWidth - 20.0f,
        dialogButtonY
    );
    dialogCancelButton.shape.setFillColor(sf::Color(150, 0, 0));
    dialogCancelButton.shape.setOutlineThickness(1.0f);
    dialogCancelButton.shape.setOutlineColor(sf::Color::White);
    
    dialogCancelButton.text.setFont(game->GetFont());
    dialogCancelButton.text.setString("Cancel");
    dialogCancelButton.text.setCharacterSize(18);
    dialogCancelButton.text.setFillColor(sf::Color::White);
    
    // Center text in button
    bounds = dialogCancelButton.text.getLocalBounds();
    dialogCancelButton.text.setPosition(
        dialogCancelButton.shape.getPosition().x + (dialogButtonWidth - bounds.width) / 2.0f,
        dialogCancelButton.shape.getPosition().y + (dialogButtonHeight - bounds.height) / 2.0f - 5.0f
    );
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
    slider.isDragging = false;
    slider.isEditing = false;
    slider.valueTextHovered = false;
    
    // Calculate positions
    float panelX = panelBackground.getPosition().x;
    float panelWidth = panelBackground.getSize().x;
    
    float trackWidth = 300.0f;
    float trackHeight = 10.0f;
    float handleSize = 20.0f;
    float valueBoxWidth = 80.0f;
    float valueBoxHeight = 25.0f;
    
    // Set up label
    slider.label.setFont(game->GetFont());
    slider.label.setString(setting->name);
    slider.label.setCharacterSize(16);
    slider.label.setFillColor(sf::Color::White);
    slider.label.setPosition(panelX + 30.0f, y);
    
    // Set up value text box background
    slider.valueTextBackground.setSize(sf::Vector2f(valueBoxWidth, valueBoxHeight));
    slider.valueTextBackground.setPosition(
        panelX + panelWidth - valueBoxWidth - 30.0f,
        y
    );
    slider.valueTextBackground.setFillColor(sf::Color(60, 60, 60));
    slider.valueTextBackground.setOutlineThickness(1.0f);
    slider.valueTextBackground.setOutlineColor(sf::Color(120, 120, 120));
    
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
    
    // Center text in value box
    sf::FloatRect textBounds = slider.valueText.getLocalBounds();
    slider.valueText.setPosition(
        slider.valueTextBackground.getPosition().x + (valueBoxWidth - textBounds.width) / 2.0f,
        slider.valueTextBackground.getPosition().y + (valueBoxHeight - textBounds.height) / 2.0f - 5.0f
    );
    
    // Set up track
    slider.track.setSize(sf::Vector2f(trackWidth, trackHeight));
    slider.track.setPosition(panelX + 150.0f, y + 15.0f);
    slider.track.setFillColor(sf::Color(80, 80, 80));
    slider.track.setOutlineThickness(1.0f);
    slider.track.setOutlineColor(sf::Color(120, 120, 120));
    
    // Set up handle
    slider.handle.setSize(sf::Vector2f(handleSize, handleSize));
    float handleX = MapValueToPosition(slider, setting->value);
    slider.handle.setPosition(handleX, y + 10.0f);
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

void GameSettingsUI::CalculateTotalPages() {
    const auto& allSettings = settingsManager->GetAllSettings();
    int totalSettings = allSettings.size();
    totalPages = (totalSettings + settingsPerPage - 1) / settingsPerPage; // Ceiling division
    if (totalPages == 0) totalPages = 1; // At least one page even if empty
}

void GameSettingsUI::UpdatePageIndicator() {
    std::ostringstream pageText;
    pageText << "Page " << (currentPage + 1) << " of " << totalPages;
    pageIndicatorText.setString(pageText.str());
    
    // Recenter text
    float panelX = panelBackground.getPosition().x;
    float panelWidth = panelBackground.getSize().x;
    pageIndicatorText.setPosition(
        panelX + (panelWidth - pageIndicatorText.getLocalBounds().width) / 2.0f,
        pageIndicatorText.getPosition().y
    );
    
    // Update button states
    prevPageButton.shape.setFillColor(currentPage > 0 ? sf::Color(50, 100, 200) : sf::Color(100, 100, 100));
    nextPageButton.shape.setFillColor(currentPage < totalPages - 1 ? sf::Color(50, 100, 200) : sf::Color(100, 100, 100));
}

void GameSettingsUI::NavigateToPage(int page) {
    if (page >= 0 && page < totalPages) {
        currentPage = page;
        UpdatePageIndicator();
        RefreshUI();
    }
}


void GameSettingsUI::ShowSaveDialog() {
    if (!isHostPlayer) return;
    
    // Refresh the list of presets to ensure it's up-to-date
    settingsManager->RefreshPresets();
    
    dialogMode = DialogMode::Save;
    fileNameInput = "";
    fileNameText.setString(fileNameInput);
    selectedPresetIndex = -1;
    
    // Update dialog title
    dialogTitleText.setString("Save Settings");
    
    UpdatePresetList();
}


void GameSettingsUI::ShowLoadDialog() {
    if (!isHostPlayer) return;
    
    // Refresh the list of presets to ensure it's up-to-date
    settingsManager->RefreshPresets();
    
    dialogMode = DialogMode::Load;
    fileNameInput = "";
    fileNameText.setString(fileNameInput);
    selectedPresetIndex = -1;
    
    // Update dialog title
    dialogTitleText.setString("Load Settings");
    
    UpdatePresetList();
}

void GameSettingsUI::CloseDialog() {
    dialogMode = DialogMode::None;
}

void GameSettingsUI::UpdatePresetList() {
    presetButtons.clear();
    
    const auto& presets = settingsManager->GetPresets();
    float startY = dialogBackground.getPosition().y + 150.0f;
    float buttonY = startY;
    float buttonHeight = 30.0f;
    float buttonSpacing = 10.0f;
    float buttonWidth = dialogBackground.getSize().x - 60.0f;
    float buttonX = dialogBackground.getPosition().x + 30.0f;
    
    for (size_t i = 0; i < presets.size(); i++) {
        Button presetButton;
        presetButton.shape.setSize(sf::Vector2f(buttonWidth, buttonHeight));
        presetButton.shape.setPosition(buttonX, buttonY);
        
        if (static_cast<int>(i) == selectedPresetIndex) {
            presetButton.shape.setFillColor(sf::Color(70, 150, 200)); // Blue when selected
        } else {
            presetButton.shape.setFillColor(sf::Color(60, 60, 60));
        }
        presetButton.shape.setOutlineThickness(1.0f);
        presetButton.shape.setOutlineColor(sf::Color(120, 120, 120));
        
        presetButton.text.setFont(game->GetFont());
        presetButton.text.setString(presets[i].name);
        presetButton.text.setCharacterSize(16);
        presetButton.text.setFillColor(sf::Color::White);
        
        // Center text in button
        sf::FloatRect bounds = presetButton.text.getLocalBounds();
        presetButton.text.setPosition(
            presetButton.shape.getPosition().x + 10.0f,
            presetButton.shape.getPosition().y + (buttonHeight - bounds.height) / 2.0f - 5.0f
        );
        
        presetButton.onClick = [this, i]() {
            selectedPresetIndex = static_cast<int>(i);
            
            const auto& presets = settingsManager->GetPresets();
            if (i < presets.size()) {
                // If this is the load dialog, update the file name input
                if (dialogMode == DialogMode::Load) {
                    fileNameInput = presets[i].name;
                    fileNameText.setString(fileNameInput);
                }
                
                // Update button colors
                UpdatePresetList();
            }
        };
        
        presetButtons.push_back(presetButton);
        buttonY += buttonHeight + buttonSpacing;
    }
}

void GameSettingsUI::RenderDialog(sf::RenderWindow& window) {
    if (dialogMode == DialogMode::None) return;
    
    // Draw dialog background
    window.draw(dialogBackground);
    
    // Draw dialog title
    window.draw(dialogTitleText);
    
    // Draw file name box and text
    window.draw(fileNameBox);
    window.draw(fileNameText);
    
    // Draw action buttons
    if (dialogMode == DialogMode::Save) {
        window.draw(dialogSaveButton.shape);
        window.draw(dialogSaveButton.text);
    } else {
        window.draw(dialogLoadButton.shape);
        window.draw(dialogLoadButton.text);
    }
    
    window.draw(dialogCancelButton.shape);
    window.draw(dialogCancelButton.text);
    
    // Draw preset buttons
    for (const auto& button : presetButtons) {
        window.draw(button.shape);
        window.draw(button.text);
    }
}

bool GameSettingsUI::ProcessDialogEvent(const sf::Event& event) {
    if (dialogMode == DialogMode::None) return false;
    sf::Vector2i mousePos(event.mouseButton.x, event.mouseButton.y);
        sf::Vector2f mouseUiPos = game->WindowToUICoordinates(mousePos);
    // Handle text input for file name
    if (event.type == sf::Event::TextEntered) {
        // Allow only valid file name characters
        if (event.text.unicode == 8) { // Backspace
            if (!fileNameInput.isEmpty()) {
                fileNameInput.erase(fileNameInput.getSize() - 1);
            }
        }
        else if (event.text.unicode == 13) { // Enter
            // Process Enter as if Save/Load button was clicked
            if (dialogMode == DialogMode::Save) {
                if (dialogSaveButton.shape.getGlobalBounds().contains(mouseUiPos)) {
                    if (!fileNameInput.isEmpty()) {
                        if (settingsManager->SaveSettings(fileNameInput.toAnsiString())) {
                            // After successful save, refresh the presets list
                            settingsManager->RefreshPresets();
                            
                            // If we want to immediately show the updated list (e.g., for verification)
                            // we could briefly show the load dialog instead of closing
                            ShowLoadDialog();
                            return true;
                        }
                        CloseDialog();
                    }
                    return true;
                }
            } else if (dialogMode == DialogMode::Load) {
                if (selectedPresetIndex >= 0) {
                    const auto& presets = settingsManager->GetPresets();
                    if (selectedPresetIndex < static_cast<int>(presets.size())) {
                        if (presets[selectedPresetIndex].isDefault) {
                            settingsManager->ResetToDefaults();
                        } else {
                            settingsManager->LoadSettings(presets[selectedPresetIndex].filePath);
                        }
                        UpdateSliderValues();
                        CloseDialog();
                    }
                }
            }
        }
        else if (event.text.unicode == 27) { // Escape
            CloseDialog();
        }
        else if ((event.text.unicode >= 48 && event.text.unicode <= 57) || // 0-9
                 (event.text.unicode >= 65 && event.text.unicode <= 90) || // A-Z
                 (event.text.unicode >= 97 && event.text.unicode <= 122) || // a-z
                 event.text.unicode == 95 || event.text.unicode == 45 || // _ -
                 event.text.unicode == 32) { // space
            fileNameInput += static_cast<char>(event.text.unicode);
        }
        
        // Update display text
        fileNameText.setString(fileNameInput);
        return true;
    }
    
    // Handle mouse button presses
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        // Convert mouse coordinates to UI view space
        sf::Vector2i mousePos(event.mouseButton.x, event.mouseButton.y);
        sf::Vector2f mouseUiPos = game->WindowToUICoordinates(mousePos);
        
        // Check if Save/Load button was clicked
        if (dialogMode == DialogMode::Save) {
            if (dialogSaveButton.shape.getGlobalBounds().contains(mouseUiPos)) {
                if (!fileNameInput.isEmpty()) {
                    settingsManager->SaveSettings(fileNameInput.toAnsiString());
                    CloseDialog();
                }
                return true;
            }
        } else if (dialogMode == DialogMode::Load) {
            if (dialogLoadButton.shape.getGlobalBounds().contains(mouseUiPos)) {
                if (selectedPresetIndex >= 0) {
                    const auto& presets = settingsManager->GetPresets();
                    if (selectedPresetIndex < static_cast<int>(presets.size())) {
                        if (presets[selectedPresetIndex].isDefault) {
                            settingsManager->ResetToDefaults();
                        } else {
                            settingsManager->LoadSettings(presets[selectedPresetIndex].filePath);
                        }
                        UpdateSliderValues();
                        CloseDialog();
                    }
                }
                return true;
            }
        }
        
        // Check if Cancel button was clicked
        if (dialogCancelButton.shape.getGlobalBounds().contains(mouseUiPos)) {
            CloseDialog();
            return true;
        }
        
        // Check if any preset button was clicked
        for (size_t i = 0; i < presetButtons.size(); i++) {
            if (presetButtons[i].shape.getGlobalBounds().contains(mouseUiPos)) {
                presetButtons[i].onClick();
                return true;
            }
        }
        
        // Check if clicked inside dialog (consume event)
        if (dialogBackground.getGlobalBounds().contains(mouseUiPos)) {
            return true;
        }
    }
    
    // Handle key presses
    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::Escape) {
            CloseDialog();
            return true;
        }
    }
    
    return false;
}

// Modify the Render method to include dialog rendering
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
        window.draw(slider.valueTextBackground);
        window.draw(slider.valueText);
    }
    
    // Draw navigation buttons
    window.draw(prevPageButton.shape);
    window.draw(prevPageButton.text);
    window.draw(nextPageButton.shape);
    window.draw(nextPageButton.text);
    window.draw(pageIndicatorText);
    
    // Draw action buttons
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
    
    // Draw file dialog if open
    RenderDialog(window);
    
    // Restore the original view
    window.setView(currentView);
}


