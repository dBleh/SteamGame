#include "SettingsState.h"
#include "../core/Game.h"
#include "../../utils/input/InputManager.h"

SettingsState::SettingsState(Game* game)
    : State(game) {
    // Get the settings manager from the game
    settingsManager = game->GetSettingsManager();
    
    // Make a deep copy of the current settings for editing
    // This assumes GameSettings has proper copy construction
    currentSettings = GameSettings(settingsManager->GetSettings());
    
    // Set up UI elements
    float centerX = BASE_WIDTH / 2.0f;
    float titleCenterX = BASE_WIDTH / 2.0f;
    float contentCenterX = BASE_WIDTH / 2.0f - 300.0f;
    
    // Title - centered at the top with larger font (similar to main menu)
    game->GetHUD().addElement("settings_title", "Settings", 48, 
                             sf::Vector2f(titleCenterX - 80.0f, 30.0f), 
                             GameState::Settings, 
                             HUD::RenderMode::ScreenSpace, false);
    
    // Add category headers using the HUD system
    float currentY = 100.0f;  // Start position after title - reduced from original 120.0f
    
    // Controls Category Header
    game->GetHUD().addElement("controls_header", "Controls", 24, 
                             sf::Vector2f(contentCenterX - 100.0f, currentY), 
                             GameState::Settings, 
                             HUD::RenderMode::ScreenSpace, false);
    
    // Add separator line after header
    currentY += 30.0f; // Reduced from original 40.0f
    float lineWidth = 500.0f;
    float lineThickness = 2.0f;
    float lineStartX = contentCenterX - (lineWidth / 2.0f);
    
    game->GetHUD().addGradientLine("controls_header_line", 
                                  lineStartX,
                                  currentY, 
                                  lineWidth, 
                                  lineThickness,
                                  sf::Color::Black, 
                                  GameState::Settings,
                                  HUD::RenderMode::ScreenSpace,
                                  30);
    
    // Set the start Y position for settings and reduce setting height and offset
    settingsStartY = currentY + 15.0f;  // Reduced padding after header line
    settingHeight = 24.0f;  // Reduced from original 30.0f
    settingOffset = 30.0f;  // Reduced from original 40.0f
    
    // Initialize buttons for Save, Cancel, and Reset
    // Bottom buttons need to be positioned absolutely to avoid overlap
    InitializeButtons(lineWidth, contentCenterX - (lineWidth / 2.0f) - 900.0f);
    
    // Initialize settings list
    InitializeSettings();
}

void SettingsState::InitializeButtons() {
    // This empty implementation is kept for compatibility
    // The actual button initialization is done in the overloaded version
}

void SettingsState::InitializeButtons(float lineWidth, float lineStartX) {
    float centerX = BASE_WIDTH / 2.0f;
    
    // Button positions from the bottom of the screen
    float buttonY = BASE_HEIGHT - 220.0f; 
    float buttonSpacing = 40.0f;
    
   
                                  
    // Update current Y position with fixed spacing
    buttonY += buttonSpacing * 0.5f;
    
    // Save button - with connected line for animation
    game->GetHUD().addElement("save_button", "Save Changes", 24, 
                             sf::Vector2f(centerX - 100.0f, buttonY), 
                             GameState::Settings, 
                             HUD::RenderMode::ScreenSpace, true,
                             "button_top_line", "button_mid_line");
    
    // Update current Y position with fixed spacing
    buttonY += buttonSpacing;
    
 
    // Update current Y position with fixed spacing
    buttonY += buttonSpacing * 0.5f;
    
    // Reset to defaults button - with connected line for animation
    game->GetHUD().addElement("reset_button", "Reset to Defaults", 24, 
                             sf::Vector2f(centerX - 100.0f, buttonY), 
                             GameState::Settings, 
                             HUD::RenderMode::ScreenSpace, true,
                             "button_mid_line", "button_bottom_line");
    
    // Update current Y position with fixed spacing
    buttonY += buttonSpacing;
    
    
    // Update current Y position with fixed spacing
    buttonY += buttonSpacing * 0.5f;
    
    // Return to Main Menu button - with connected line for animation
    game->GetHUD().addElement("return_button", "Return to Main Menu", 24, 
                             sf::Vector2f(centerX - 100.0f, buttonY), 
                             GameState::Settings, 
                             HUD::RenderMode::ScreenSpace, true,
                             "button_bottom_line", "");
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
    
    AddKeySetting("toggleReady", "Toggle Ready Status", 
                 [this]() { return currentSettings.toggleReady; },
                 [this](sf::Keyboard::Key key) { currentSettings.toggleReady = key; });
    
    // Initialize separator lines for categories with better positioning
    float centerX = BASE_WIDTH / 2.0f;
    float contentCenterX = BASE_WIDTH / 2.0f;
    float lineWidth = 500.0f;
    float lineStartX = contentCenterX - (lineWidth / 2.0f);
    
    // Calculate the position for Other Settings category - based on the number of control settings
    // 10 control settings + space for header
    float currentY = settingsStartY + (10 * settingOffset) + 20.0f;
    
    
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
    float contentCenterX = BASE_WIDTH / 2.0f - 380.0f;
    float yPos = settingsStartY;
    
    // Update all settings with their current values and mouse rectangles
    for (size_t i = 0; i < settings.size(); ++i) {
        auto& setting = settings[i];
        
        // Add space for "Other Settings" category header
        if (i == 10) { // Index where Other settings start (after toggleReady)
            // Skip to the position after the "Other Settings" header, calculated in InitializeSettings
            float otherSettingsY = settingsStartY + (10 * settingOffset) + 65.0f; // 10 previous settings + header + line + spacing
            yPos = otherSettingsY;
        }
        
        // Don't update current value if waiting for input
        if (!setting.isWaitingForInput) {
            setting.currentValue = setting.getValue();
        }
        
        // Update the mouse click area for this setting
        setting.mouseRect = sf::FloatRect(contentCenterX - 50.0f, yPos, 700.0f, settingHeight);
        
        // For sliders, add clickable areas for left/right adjustments with proper spacing
        if (setting.type == SettingType::Slider) {
            setting.sliderLeftRect = sf::FloatRect(centerX + 30.0f, yPos, 30.0f, settingHeight);
            setting.sliderRightRect = sf::FloatRect(centerX + 120.0f, yPos, 30.0f, settingHeight);
        }
        
        yPos += settingOffset;
    }
    
    // Update HUD animations for line shake effect
    game->GetHUD().update(game->GetWindow(), GameState::Settings, dt);
}

void SettingsState::DrawSelectedIndicator(float yPos) {
    sf::RectangleShape indicator(sf::Vector2f(8.0f, 8.0f)); // Smaller indicator
    indicator.setFillColor(sf::Color::Yellow);
    indicator.setPosition(250.0f, yPos + settingHeight / 2.0f - 4.0f);
    game->GetWindow().draw(indicator);
}

void SettingsState::DrawSlider(const Setting& setting, float yPos) {
    if (setting.type != SettingType::Slider) return;
    
    float centerX = BASE_WIDTH / 2.0f;
    float sliderWidth = 150.0f;
    float sliderHeight = 8.0f; // Smaller slider
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
    sf::CircleShape handle(6.0f); // Smaller handle
    handle.setFillColor(sf::Color::White);
    handle.setOrigin(6.0f, 6.0f);
    handle.setPosition(sliderX + sliderWidth * fillPercent, sliderY + sliderHeight / 2.0f);
    game->GetWindow().draw(handle);
    
    // Draw arrow indicators for adjusting the slider with better positioning
    float arrowOffset = 12.0f; // Smaller offset for better spacing
    
    sf::ConvexShape leftArrow;
    leftArrow.setPointCount(3);
    leftArrow.setPoint(0, sf::Vector2f(sliderX - arrowOffset - 4.0f, sliderY + sliderHeight / 2.0f));
    leftArrow.setPoint(1, sf::Vector2f(sliderX - arrowOffset + 4.0f, sliderY - 4.0f));
    leftArrow.setPoint(2, sf::Vector2f(sliderX - arrowOffset + 4.0f, sliderY + sliderHeight + 4.0f));
    leftArrow.setFillColor(sf::Color(180, 180, 200));
    game->GetWindow().draw(leftArrow);
    
    sf::ConvexShape rightArrow;
    rightArrow.setPointCount(3);
    rightArrow.setPoint(0, sf::Vector2f(sliderX + sliderWidth + arrowOffset + 4.0f, sliderY + sliderHeight / 2.0f));
    rightArrow.setPoint(1, sf::Vector2f(sliderX + sliderWidth + arrowOffset - 4.0f, sliderY - 4.0f));
    rightArrow.setPoint(2, sf::Vector2f(sliderX + sliderWidth + arrowOffset - 4.0f, sliderY + sliderHeight + 4.0f));
    rightArrow.setFillColor(sf::Color(180, 180, 200));
    game->GetWindow().draw(rightArrow);
}

void SettingsState::DrawSettings() {
    float contentCenterX = BASE_WIDTH / 2.0f - 380.0f;
    float yPos = settingsStartY;
    float centerX = BASE_WIDTH / 2.0f;
    
    // Draw settings
    for (size_t i = 0; i < settings.size(); ++i) {
        const auto& setting = settings[i];
        
        // Handle category transition
        if (i == 10) { // Index where Other settings start (after toggleReady)
            // Skip to the position after the "Other Settings" header, calculated in Update
            float otherSettingsY = settingsStartY + (10 * settingOffset) + 65.0f; // 10 previous settings + header + spacing
            yPos = otherSettingsY;
        }
        
        // Setting name
        sf::Text nameText;
        nameText.setFont(game->GetFont());
        nameText.setString(setting.displayName);
        nameText.setCharacterSize(18); // Smaller text
        nameText.setFillColor(sf::Color::Black);
        nameText.setPosition(contentCenterX - 20.0f, yPos);
        
        // Setting value with improved positioning
        sf::Text valueText;
        valueText.setFont(game->GetFont());
        
        if (setting.isWaitingForInput) {
            valueText.setString("Press any key or click...");
            valueText.setFillColor(sf::Color::Yellow);
        } else {
            valueText.setString(setting.currentValue);
            valueText.setFillColor(sf::Color::Black);
        }
        
        valueText.setCharacterSize(18); // Smaller text
        
        // Adjust value text position based on setting type
        if (setting.type == SettingType::Slider) {
            // Position to the left of the slider for better layout
            valueText.setPosition(centerX, yPos);
        } else {
            // Normal positioning for other setting types
            valueText.setPosition(centerX + 50.0f, yPos);
        }
        
        // Highlight selected setting
        if (i == static_cast<size_t>(selectedIndex)) {
            nameText.setFillColor(sf::Color::White);
            if (!setting.isWaitingForInput) {
                valueText.setFillColor(sf::Color::White);
            }
            //DrawSelectedIndicator(yPos);
        }
        
        game->GetWindow().draw(nameText);
        game->GetWindow().draw(valueText);
        
        // Draw sliders for slider settings
        if (setting.type == SettingType::Slider) {
            DrawSlider(setting, yPos);
        }
        
        yPos += settingOffset;
    }
    
    // Draw controls hint at the bottom
    float controlsY = BASE_HEIGHT - 20.0f; // Position closer to bottom
    
    sf::Text controlsText;
    controlsText.setFont(game->GetFont());
    controlsText.setCharacterSize(14); // Smaller hint text
    controlsText.setFillColor(sf::Color::White);
    controlsText.setString("Up/Down: Navigate | Enter/Click: Change");
    
    sf::FloatRect bounds = controlsText.getLocalBounds();
    controlsText.setPosition(centerX - bounds.width / 2.0f - 500, controlsY);
    
    game->GetWindow().draw(controlsText);
}

void SettingsState::Render() {
    // Clear the window with the same background color as MainMenuState
    game->GetWindow().clear(MAIN_BACKGROUND_COLOR);
    
    // Use UI view for settings screen
    game->GetWindow().setView(game->GetUIView());
    
    // Draw all settings
    DrawSettings();
    
    // Draw HUD elements
    game->GetHUD().render(game->GetWindow(), game->GetUIView(), GameState::Settings);
    
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
    
    // Handle mouse clicks on HUD elements
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        // Get window mouse position
        sf::Vector2i mouseWindowPos(event.mouseButton.x, event.mouseButton.y);
        
        // Convert to UI coordinates
        sf::Vector2f mouseUIPos = game->WindowToUICoordinates(mouseWindowPos);
        
        // Only process clicks within the UI viewport
        if (mouseUIPos.x >= 0 && mouseUIPos.y >= 0) {
            const auto& elements = game->GetHUD().getElements();
            
            for (const auto& [id, element] : elements) {
                if (element.hoverable && element.visibleState == GameState::Settings) {
                    // Create a copy of the text to check bounds
                    sf::Text textCopy = element.text;
                    textCopy.setPosition(element.pos);
                    sf::FloatRect bounds = textCopy.getGlobalBounds();
                    
                    if (bounds.contains(mouseUIPos)) {
                        if (id == "save_button") {
                            SaveAndExit();
                            return;
                        } else if (id == "reset_button") {
                            ResetToDefaults();
                            return;
                        } else if (id == "return_button") {
                            CancelAndExit();
                            return;
                        }
                    }
                }
            }
        }
        
        // Check if a setting was clicked
        sf::Vector2f mousePosView = game->GetWindow().mapPixelToCoords(mouseWindowPos, game->GetUIView());
        
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
                
            case sf::Keyboard::S:  // Shortcut for Save
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::LControl) ||
                    sf::Keyboard::isKeyPressed(sf::Keyboard::RControl)) {
                    SaveAndExit();
                }
                break;
                
            case sf::Keyboard::Escape:  // Shortcut for Cancel
                CancelAndExit();
                break;
        }
    }
}


void SettingsState::SaveAndExit() {
    // Update the settings in the game's settings manager with our modified copy
    *const_cast<GameSettings*>(&settingsManager->GetSettings()) = currentSettings;
    settingsManager->SaveSettings();
    
    // Update the input handler with new settings
    game->GetInputHandler()->UpdateKeyBindings();
    
    // Synchronize InputManager with the new settings
    InputManager& inputManager = game->GetInputManager();
    const GameSettings& settings = settingsManager->GetSettings();
    
    // Update each key binding in the InputManager
    inputManager.SetKeyBinding(GameAction::MoveUp, settings.moveUp);
    inputManager.SetKeyBinding(GameAction::MoveDown, settings.moveDown);
    inputManager.SetKeyBinding(GameAction::MoveLeft, settings.moveLeft);
    inputManager.SetKeyBinding(GameAction::MoveRight, settings.moveRight);
    inputManager.SetKeyBinding(GameAction::Shoot, settings.shoot);
    inputManager.SetKeyBinding(GameAction::ShowLeaderboard, settings.showLeaderboard);
    inputManager.SetKeyBinding(GameAction::OpenMenu, settings.showMenu);
    inputManager.SetKeyBinding(GameAction::ToggleGrid, settings.toggleGrid);
    inputManager.SetKeyBinding(GameAction::ToggleCursorLock, settings.toggleCursorLock);
    inputManager.SetKeyBinding(GameAction::ToggleReady, settings.toggleReady);
    
    // Animate the save button line when saving
    game->GetHUD().animateLine("button_top_line", 4.0f);
    game->GetHUD().animateLine("button_mid_line", 2.0f);
    
    // Print debug info about updated settings
    std::cout << "[SETTINGS] Settings saved. Key bindings updated:" << std::endl;
    std::cout << "  MoveUp: " << SettingsManager::KeyToString(settings.moveUp) << std::endl;
    std::cout << "  MoveDown: " << SettingsManager::KeyToString(settings.moveDown) << std::endl;
    std::cout << "  MoveLeft: " << SettingsManager::KeyToString(settings.moveLeft) << std::endl;
    std::cout << "  MoveRight: " << SettingsManager::KeyToString(settings.moveRight) << std::endl;
    std::cout << "  Shoot: " << SettingsManager::KeyToString(settings.shoot) << std::endl;
    
    // Return to the main menu after a short delay to show animation
    // In a real implementation, you might want to add a timer here
    // For simplicity, we'll just return immediately
    game->SetCurrentState(GameState::MainMenu);
}

void SettingsState::CancelAndExit() {
    // Discard changes and return to the main menu
    std::cout << "[SETTINGS] Settings changes cancelled" << std::endl;
    
    // Animate the return button line when canceling
    game->GetHUD().animateLine("button_bottom_line", 4.0f);
    
    game->SetCurrentState(GameState::MainMenu);
}

void SettingsState::ResetToDefaults() {
    // Reset to default settings
    currentSettings = GameSettings(); // Create a fresh default GameSettings object
    
    // Update all settings
    for (auto& setting : settings) {
        setting.currentValue = setting.getValue();
    }
    
    // Animate the reset button line
    game->GetHUD().animateLine("button_mid_line", 4.0f);
    game->GetHUD().animateLine("button_bottom_line", 2.0f);
    
    std::cout << "[SETTINGS] Settings reset to defaults" << std::endl;
}