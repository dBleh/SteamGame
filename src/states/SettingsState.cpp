#include "SettingsState.h"
#include "../Game.h"

SettingsState::SettingsState(Game* game)
    : State(game) {
    // Get the settings manager from the game
    settingsManager = game->GetSettingsManager();
    
    // Make a deep copy of the current settings for editing
    // This assumes GameSettings has proper copy construction
    currentSettings = GameSettings(settingsManager->GetSettings());
    
    // Set up UI elements
    float centerX = BASE_WIDTH / 2.0f;
    
    // Title and header bar
    titleText.setFont(game->GetFont());
    titleText.setString("Settings");
    titleText.setCharacterSize(36);
    titleText.setFillColor(sf::Color::White);
    titleText.setStyle(sf::Text::Bold);
    
    sf::FloatRect textBounds = titleText.getLocalBounds();
    titleText.setOrigin(textBounds.width / 2.0f, textBounds.height / 2.0f);
    titleText.setPosition(centerX, 50.0f);
    
    // Setup panel background
    panelBackground.setSize(sf::Vector2f(800.0f, 600.0f));
    panelBackground.setFillColor(sf::Color(30, 30, 50, 220));
    panelBackground.setOutlineColor(sf::Color(100, 100, 200, 150));
    panelBackground.setOutlineThickness(2.0f);
    panelBackground.setPosition(centerX - 400.0f, 20.0f);
    
    // Setup header bar
    headerBar.setSize(sf::Vector2f(800.0f, 60.0f));
    headerBar.setFillColor(sf::Color(50, 50, 80, 230));
    headerBar.setPosition(centerX - 400.0f, 20.0f);
    
    // Setup buttons for Save, Cancel, and Reset
    InitializeButtons();
    
    // Initialize settings list
    InitializeSettings();
}

void SettingsState::InitializeButtons() {
    float centerX = BASE_WIDTH / 2.0f;
    float buttonY = BASE_HEIGHT - 60.0f;
    float buttonWidth = 150.0f;
    float buttonHeight = 40.0f;
    float buttonSpacing = 30.0f;
    
    // Save button
    saveButton.shape.setSize(sf::Vector2f(buttonWidth, buttonHeight));
    saveButton.shape.setFillColor(sf::Color(50, 100, 50, 220));
    saveButton.shape.setOutlineColor(sf::Color(100, 200, 100, 150));
    saveButton.shape.setOutlineThickness(2.0f);
    saveButton.shape.setPosition(centerX - buttonWidth - buttonSpacing, buttonY);
    
    saveButton.text.setFont(game->GetFont());
    saveButton.text.setString("Save");
    saveButton.text.setCharacterSize(20);
    saveButton.text.setFillColor(sf::Color::White);
    sf::FloatRect saveBounds = saveButton.text.getLocalBounds();
    saveButton.text.setOrigin(saveBounds.width / 2.0f, saveBounds.height / 2.0f);
    saveButton.text.setPosition(
        saveButton.shape.getPosition().x + buttonWidth / 2.0f,
        saveButton.shape.getPosition().y + buttonHeight / 2.0f - 5.0f
    );
    
    // Cancel button
    cancelButton.shape.setSize(sf::Vector2f(buttonWidth, buttonHeight));
    cancelButton.shape.setFillColor(sf::Color(100, 50, 50, 220));
    cancelButton.shape.setOutlineColor(sf::Color(200, 100, 100, 150));
    cancelButton.shape.setOutlineThickness(2.0f);
    cancelButton.shape.setPosition(centerX + buttonSpacing, buttonY);
    
    cancelButton.text.setFont(game->GetFont());
    cancelButton.text.setString("Cancel");
    cancelButton.text.setCharacterSize(20);
    cancelButton.text.setFillColor(sf::Color::White);
    sf::FloatRect cancelBounds = cancelButton.text.getLocalBounds();
    cancelButton.text.setOrigin(cancelBounds.width / 2.0f, cancelBounds.height / 2.0f);
    cancelButton.text.setPosition(
        cancelButton.shape.getPosition().x + buttonWidth / 2.0f,
        cancelButton.shape.getPosition().y + buttonHeight / 2.0f - 5.0f
    );
    
    // Reset button
    resetButton.shape.setSize(sf::Vector2f(buttonWidth, buttonHeight));
    resetButton.shape.setFillColor(sf::Color(80, 80, 100, 220));
    resetButton.shape.setOutlineColor(sf::Color(150, 150, 200, 150));
    resetButton.shape.setOutlineThickness(2.0f);
    resetButton.shape.setPosition(centerX - buttonWidth / 2.0f, buttonY + buttonHeight + 10.0f);
    
    resetButton.text.setFont(game->GetFont());
    resetButton.text.setString("Reset to Defaults");
    resetButton.text.setCharacterSize(18);
    resetButton.text.setFillColor(sf::Color::White);
    sf::FloatRect resetBounds = resetButton.text.getLocalBounds();
    resetButton.text.setOrigin(resetBounds.width / 2.0f, resetBounds.height / 2.0f);
    resetButton.text.setPosition(
        resetButton.shape.getPosition().x + buttonWidth / 2.0f,
        resetButton.shape.getPosition().y + buttonHeight / 2.0f - 5.0f
    );
}

void SettingsState::InitializeSettings() {
    // Movement Controls
    AddKeySetting("moveUp", "Move Up", 
                 [this]() { return currentSettings.moveUp; },
                 [this](sf::Keyboard::Key key) { currentSettings.moveUp = key; });
    
    AddKeySetting("moveDown", "Move Down", 
                 [this]() { return currentSettings.moveDown; },
                 [this](sf::Keyboard::Key key) { currentSettings.moveDown = key; });
    
    AddKeySetting("moveLeft", "Move Left", 
                 [this]() { return currentSettings.moveLeft; },
                 [this](sf::Keyboard::Key key) { currentSettings.moveLeft = key; });
    
    AddKeySetting("moveRight", "Move Right", 
                 [this]() { return currentSettings.moveRight; },
                 [this](sf::Keyboard::Key key) { currentSettings.moveRight = key; });
    
    // Action Controls
    AddKeySetting("shoot", "Shoot", 
                 [this]() { return currentSettings.shoot; },
                 [this](sf::Keyboard::Key key) { currentSettings.shoot = key; });
    
    AddKeySetting("showLeaderboard", "Show Leaderboard", 
                 [this]() { return currentSettings.showLeaderboard; },
                 [this](sf::Keyboard::Key key) { currentSettings.showLeaderboard = key; });
    
    AddKeySetting("showMenu", "Show Menu", 
                 [this]() { return currentSettings.showMenu; },
                 [this](sf::Keyboard::Key key) { currentSettings.showMenu = key; });
    
    AddKeySetting("toggleGrid", "Toggle Grid", 
                 [this]() { return currentSettings.toggleGrid; },
                 [this](sf::Keyboard::Key key) { currentSettings.toggleGrid = key; });
    
    AddKeySetting("toggleCursorLock", "Toggle Cursor Lock", 
                 [this]() { return currentSettings.toggleCursorLock; },
                 [this](sf::Keyboard::Key key) { currentSettings.toggleCursorLock = key; });
    
    // Other Settings
    AddToggleSetting("showFPS", "Show FPS", 
                    [this]() { return currentSettings.showFPS; },
                    [this](bool value) { currentSettings.showFPS = value; });
    
    AddSliderSetting("volumeLevel", "Volume", 
                    [this]() { return currentSettings.volumeLevel; },
                    [this](int value) { currentSettings.volumeLevel = value; },
                    0, 100, 5);
}

void SettingsState::AddKeySetting(const std::string& id, const std::string& displayName, 
                                 const std::function<sf::Keyboard::Key()>& getKey,
                                 const std::function<void(sf::Keyboard::Key)>& setKey) {
    Setting setting;
    setting.id = id;
    setting.displayName = displayName;
    setting.type = SettingType::KeyBinding;
    
    setting.getValue = [getKey]() { 
        return SettingsManager::KeyToString(getKey());
    };
    
    setting.setValue = [setKey](const std::string& value) {
        setKey(SettingsManager::StringToKey(value));
    };
    
    setting.currentValue = setting.getValue();
    setting.mouseRect = sf::FloatRect(); // Will be set in Update
    settings.push_back(setting);
}

void SettingsState::AddToggleSetting(const std::string& id, const std::string& displayName,
                                    const std::function<bool()>& getBool,
                                    const std::function<void(bool)>& setBool) {
    Setting setting;
    setting.id = id;
    setting.displayName = displayName;
    setting.type = SettingType::Toggle;
    
    setting.getValue = [getBool]() { 
        return getBool() ? "On" : "Off";
    };
    
    setting.setValue = [setBool](const std::string& value) {
        setBool(value == "On");
    };
    
    setting.currentValue = setting.getValue();
    setting.mouseRect = sf::FloatRect(); // Will be set in Update
    settings.push_back(setting);
}

void SettingsState::AddSliderSetting(const std::string& id, const std::string& displayName,
                                    const std::function<int()>& getInt,
                                    const std::function<void(int)>& setInt,
                                    int min, int max, int step) {
    Setting setting;
    setting.id = id;
    setting.displayName = displayName;
    setting.type = SettingType::Slider;
    setting.min = min;
    setting.max = max;
    setting.step = step;
    
    setting.getValue = [getInt]() { 
        return std::to_string(getInt());
    };
    
    setting.setValue = [setInt, min, max](const std::string& value) {
        int intValue = std::stoi(value);
        if (intValue < min) intValue = min;
        if (intValue > max) intValue = max;
        setInt(intValue);
    };
    
    setting.currentValue = setting.getValue();
    setting.mouseRect = sf::FloatRect(); // Will be set in Update
    settings.push_back(setting);
}

void SettingsState::Update(float dt) {
    float centerX = BASE_WIDTH / 2.0f;
    float yPos = settingsStartY;
    
    // Add spacing for category header
    yPos += 40.0f;
    
    // Update all settings with their current values and mouse rectangles
    for (size_t i = 0; i < settings.size(); ++i) {
        auto& setting = settings[i];
        
        // Add space for category header
        if (i == 9) { // Index where Other settings start
            yPos += 60.0f; // Add space for category header
        }
        
        // Don't update current value if waiting for input
        if (!setting.isWaitingForInput) {
            setting.currentValue = setting.getValue();
        }
        
        // Update the mouse click area for this setting
        setting.mouseRect = sf::FloatRect(centerX - 350.0f, yPos, 700.0f, settingHeight);
        
        // For sliders, add clickable areas for left/right adjustments
        if (setting.type == SettingType::Slider) {
            setting.sliderLeftRect = sf::FloatRect(centerX + 30.0f, yPos, 30.0f, settingHeight);
            setting.sliderRightRect = sf::FloatRect(centerX + 120.0f, yPos, 30.0f, settingHeight);
        }
        
        yPos += settingOffset;
    }
    
    // Check if mouse is hovering over buttons
    sf::Vector2i mousePos = sf::Mouse::getPosition(game->GetWindow());
    sf::Vector2f mousePosView = game->GetWindow().mapPixelToCoords(mousePos, game->GetUIView());
    
    saveButton.isHovered = saveButton.shape.getGlobalBounds().contains(mousePosView);
    cancelButton.isHovered = cancelButton.shape.getGlobalBounds().contains(mousePosView);
    resetButton.isHovered = resetButton.shape.getGlobalBounds().contains(mousePosView);
    
    // Update button colors based on hover state
    saveButton.shape.setFillColor(saveButton.isHovered ? 
                                 sf::Color(80, 150, 80, 220) : 
                                 sf::Color(50, 100, 50, 220));
    
    cancelButton.shape.setFillColor(cancelButton.isHovered ? 
                                   sf::Color(150, 80, 80, 220) : 
                                   sf::Color(100, 50, 50, 220));
    
    resetButton.shape.setFillColor(resetButton.isHovered ? 
                                  sf::Color(100, 100, 130, 220) : 
                                  sf::Color(80, 80, 100, 220));
}

void SettingsState::DrawSelectedIndicator(float yPos) {
    sf::RectangleShape indicator(sf::Vector2f(10.0f, 10.0f));
    indicator.setFillColor(sf::Color::Yellow);
    indicator.setPosition(250.0f, yPos + settingHeight / 2.0f - 5.0f);
    game->GetWindow().draw(indicator);
}

void SettingsState::DrawSlider(const Setting& setting, float yPos) {
    if (setting.type != SettingType::Slider) return;
    
    float centerX = BASE_WIDTH / 2.0f;
    float sliderWidth = 150.0f;
    float sliderHeight = 10.0f;
    float sliderX = centerX + 50.0f;
    float sliderY = yPos + (settingHeight - sliderHeight) / 2.0f;
    
    // Draw slider background
    sf::RectangleShape sliderBg(sf::Vector2f(sliderWidth, sliderHeight));
    sliderBg.setFillColor(sf::Color(60, 60, 80));
    sliderBg.setPosition(sliderX, sliderY);
    game->GetWindow().draw(sliderBg);
    
    // Draw slider fill based on value
    int value = std::stoi(setting.currentValue);
    float fillPercent = static_cast<float>(value - setting.min) / (setting.max - setting.min);
    sf::RectangleShape sliderFill(sf::Vector2f(sliderWidth * fillPercent, sliderHeight));
    sliderFill.setFillColor(sf::Color(100, 150, 255));
    sliderFill.setPosition(sliderX, sliderY);
    game->GetWindow().draw(sliderFill);
    
    // Draw slider handle
    sf::CircleShape handle(8.0f);
    handle.setFillColor(sf::Color::White);
    handle.setOrigin(8.0f, 8.0f);
    handle.setPosition(sliderX + sliderWidth * fillPercent, sliderY + sliderHeight / 2.0f);
    game->GetWindow().draw(handle);
    
    // Draw arrow indicators for adjusting the slider
    sf::ConvexShape leftArrow;
    leftArrow.setPointCount(3);
    leftArrow.setPoint(0, sf::Vector2f(sliderX - 20.0f, sliderY + sliderHeight / 2.0f));
    leftArrow.setPoint(1, sf::Vector2f(sliderX - 10.0f, sliderY - 5.0f));
    leftArrow.setPoint(2, sf::Vector2f(sliderX - 10.0f, sliderY + sliderHeight + 5.0f));
    leftArrow.setFillColor(sf::Color(180, 180, 200));
    game->GetWindow().draw(leftArrow);
    
    sf::ConvexShape rightArrow;
    rightArrow.setPointCount(3);
    rightArrow.setPoint(0, sf::Vector2f(sliderX + sliderWidth + 20.0f, sliderY + sliderHeight / 2.0f));
    rightArrow.setPoint(1, sf::Vector2f(sliderX + sliderWidth + 10.0f, sliderY - 5.0f));
    rightArrow.setPoint(2, sf::Vector2f(sliderX + sliderWidth + 10.0f, sliderY + sliderHeight + 5.0f));
    rightArrow.setFillColor(sf::Color(180, 180, 200));
    game->GetWindow().draw(rightArrow);
}

void SettingsState::DrawSettings() {
    float centerX = BASE_WIDTH / 2.0f;
    float yPos = settingsStartY;
    
    // Draw title for settings categories
    sf::Text categoryText;
    categoryText.setFont(game->GetFont());
    categoryText.setCharacterSize(24);
    categoryText.setFillColor(sf::Color(200, 200, 255));
    
    // Controls category
    categoryText.setString("Controls");
    categoryText.setPosition(centerX - 350.0f, yPos);
    game->GetWindow().draw(categoryText);
    yPos += 40.0f;
    
    // Draw settings
    for (size_t i = 0; i < settings.size(); ++i) {
        const auto& setting = settings[i];
        
        // Draw category headers
        if (i == 9) { // Index where Other settings start
            yPos += 20.0f; // Add some space
            categoryText.setString("Other Settings");
            categoryText.setPosition(centerX - 350.0f, yPos);
            game->GetWindow().draw(categoryText);
            yPos += 40.0f;
        }
        
        // Setting name
        sf::Text nameText;
        nameText.setFont(game->GetFont());
        nameText.setString(setting.displayName);
        nameText.setCharacterSize(20);
        nameText.setFillColor(sf::Color::White);
        nameText.setPosition(centerX - 320.0f, yPos);
        
        // Setting value
        sf::Text valueText;
        valueText.setFont(game->GetFont());
        
        if (setting.isWaitingForInput) {
            valueText.setString("Press any key or click...");
            valueText.setFillColor(sf::Color::Yellow);
        } else {
            valueText.setString(setting.currentValue);
            valueText.setFillColor(sf::Color::White);
        }
        
        valueText.setCharacterSize(20);
        valueText.setPosition(centerX + 50.0f, yPos);
        
        // Highlight selected setting
        if (i == static_cast<size_t>(selectedIndex)) {
            nameText.setFillColor(sf::Color::Yellow);
            if (!setting.isWaitingForInput) {
                valueText.setFillColor(sf::Color::Yellow);
            }
            DrawSelectedIndicator(yPos);
        }
        
        game->GetWindow().draw(nameText);
        game->GetWindow().draw(valueText);
        
        // Draw sliders for slider settings
        if (setting.type == SettingType::Slider) {
            DrawSlider(setting, yPos);
        }
        
        yPos += settingOffset;
    }
    
    // Draw buttons
    game->GetWindow().draw(saveButton.shape);
    game->GetWindow().draw(saveButton.text);
    
    game->GetWindow().draw(cancelButton.shape);
    game->GetWindow().draw(cancelButton.text);
    
    game->GetWindow().draw(resetButton.shape);
    game->GetWindow().draw(resetButton.text);
    
    // Draw controls at the bottom
    float controlsY = BASE_HEIGHT - 100.0f;
    
    sf::Text controlsText;
    controlsText.setFont(game->GetFont());
    controlsText.setCharacterSize(18);
    controlsText.setFillColor(sf::Color::White);
    controlsText.setString("Up/Down: Navigate | Enter/Click: Change | Click buttons to Save/Cancel/Reset");
    
    sf::FloatRect bounds = controlsText.getLocalBounds();
    controlsText.setPosition(centerX - bounds.width / 2.0f, controlsY);
    
    game->GetWindow().draw(controlsText);
}

void SettingsState::Render() {
    // Clear the window with a dark background
    game->GetWindow().clear(sf::Color(20, 20, 30));
    
    // Use UI view for settings screen
    game->GetWindow().setView(game->GetUIView());
    
    // Draw panel background
    game->GetWindow().draw(panelBackground);
    
    // Draw header bar
    game->GetWindow().draw(headerBar);
    
    // Draw title
    game->GetWindow().draw(titleText);
    
    // Draw all settings
    DrawSettings();
    
    // Display the rendered frame
    game->GetWindow().display();
}

void SettingsState::ProcessEvent(const sf::Event& event) {
    // If waiting for key input
    if (waitingForKeyInput) {
        if (event.type == sf::Event::KeyPressed) {
            if (event.key.code == sf::Keyboard::Escape) {
                // Cancel key binding
                waitingForKeyInput = false;
                settings[selectedIndex].isWaitingForInput = false;
            } else {
                // Set the key binding
                settings[selectedIndex].setValue(SettingsManager::KeyToString(event.key.code));
                settings[selectedIndex].currentValue = settings[selectedIndex].getValue();
                waitingForKeyInput = false;
                settings[selectedIndex].isWaitingForInput = false;
            }
        } else if (event.type == sf::Event::MouseButtonPressed) {
            // Only for the "shoot" action, allow mouse button binding
            if (settings[selectedIndex].id == "shoot") {
                // Bind to mouse button (we'll represent this as a special string)
                std::string buttonStr = "Mouse";
                if (event.mouseButton.button == sf::Mouse::Left) {
                    buttonStr += "Left";
                } else if (event.mouseButton.button == sf::Mouse::Right) {
                    buttonStr += "Right";
                } else if (event.mouseButton.button == sf::Mouse::Middle) {
                    buttonStr += "Middle";
                } else {
                    buttonStr += std::to_string(event.mouseButton.button);
                }
                
                settings[selectedIndex].setValue(buttonStr);
                settings[selectedIndex].currentValue = buttonStr;
                waitingForKeyInput = false;
                settings[selectedIndex].isWaitingForInput = false;
            } else {
                // For other actions, cancel key binding on mouse click
                waitingForKeyInput = false;
                settings[selectedIndex].isWaitingForInput = false;
            }
        }
        return;
    }
    
    // Handle mouse clicks
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2i mousePos(event.mouseButton.x, event.mouseButton.y);
        sf::Vector2f mousePosView = game->GetWindow().mapPixelToCoords(mousePos, game->GetUIView());
        
        // Check if a button was clicked
        if (saveButton.shape.getGlobalBounds().contains(mousePosView)) {
            SaveAndExit();
            return;
        }
        
        if (cancelButton.shape.getGlobalBounds().contains(mousePosView)) {
            CancelAndExit();
            return;
        }
        
        if (resetButton.shape.getGlobalBounds().contains(mousePosView)) {
            ResetToDefaults();
            return;
        }
        
        // Check if a setting was clicked
        for (size_t i = 0; i < settings.size(); ++i) {
            if (settings[i].mouseRect.contains(mousePosView)) {
                selectedIndex = i;
                
                // Handle different setting types
                if (settings[i].type == SettingType::KeyBinding) {
                    waitingForKeyInput = true;
                    settings[i].isWaitingForInput = true;
                }
                else if (settings[i].type == SettingType::Toggle) {
                    if (settings[i].currentValue == "On") {
                        settings[i].setValue("Off");
                    } else {
                        settings[i].setValue("On");
                    }
                }
                else if (settings[i].type == SettingType::Slider) {
                    // Check for slider arrows
                    if (settings[i].sliderLeftRect.contains(mousePosView)) {
                        int value = std::stoi(settings[i].currentValue);
                        value = std::max(settings[i].min, value - settings[i].step);
                        settings[i].setValue(std::to_string(value));
                    }
                    else if (settings[i].sliderRightRect.contains(mousePosView)) {
                        int value = std::stoi(settings[i].currentValue);
                        value = std::min(settings[i].max, value + settings[i].step);
                        settings[i].setValue(std::to_string(value));
                    }
                }
                break;
            }
        }
    }
    
    // Normal keyboard input handling
    if (event.type == sf::Event::KeyPressed) {
        switch (event.key.code) {
            case sf::Keyboard::Up:
                selectedIndex = (selectedIndex - 1 + settings.size()) % settings.size();
                break;
                
            case sf::Keyboard::Down:
                selectedIndex = (selectedIndex + 1) % settings.size();
                break;
                
            case sf::Keyboard::Return:
                // Start waiting for input if this is a key binding
                if (settings[selectedIndex].type == SettingType::KeyBinding) {
                    waitingForKeyInput = true;
                    settings[selectedIndex].isWaitingForInput = true;
                }
                // Toggle if this is a toggle setting
                else if (settings[selectedIndex].type == SettingType::Toggle) {
                    if (settings[selectedIndex].currentValue == "On") {
                        settings[selectedIndex].setValue("Off");
                    } else {
                        settings[selectedIndex].setValue("On");
                    }
                }
                break;
                
            case sf::Keyboard::Left:
                // Decrease value if this is a slider
                if (settings[selectedIndex].type == SettingType::Slider) {
                    int value = std::stoi(settings[selectedIndex].currentValue);
                    value = std::max(settings[selectedIndex].min, value - settings[selectedIndex].step);
                    settings[selectedIndex].setValue(std::to_string(value));
                }
                break;
                
            case sf::Keyboard::Right:
                // Increase value if this is a slider
                if (settings[selectedIndex].type == SettingType::Slider) {
                    int value = std::stoi(settings[selectedIndex].currentValue);
                    value = std::min(settings[selectedIndex].max, value + settings[selectedIndex].step);
                    settings[selectedIndex].setValue(std::to_string(value));
                }
                break;
        }
    }
}

void SettingsState::SaveAndExit() {
    // Update the settings in the game's settings manager with our modified copy
    *const_cast<GameSettings*>(&settingsManager->GetSettings()) = currentSettings;
    settingsManager->SaveSettings();
    
    // Return to the main menu
    game->SetCurrentState(GameState::MainMenu);
}

void SettingsState::CancelAndExit() {
    // Discard changes and return to the main menu
    game->SetCurrentState(GameState::MainMenu);
}

void SettingsState::ResetToDefaults() {
    // Reset to default settings
    currentSettings = GameSettings(); // Create a fresh default GameSettings object
    
    // Update all settings
    for (auto& setting : settings) {
        setting.currentValue = setting.getValue();
    }
}