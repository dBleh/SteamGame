#ifndef SETTINGS_STATE_H
#define SETTINGS_STATE_H

#include "State.h"
#include "../utils/Config.h"
#include "../utils/SettingsManager.h"
#include <SFML/Graphics.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <functional>

// Forward declarations
class Game;
class SettingsManager;
struct GameSettings;

enum class SettingType {
    KeyBinding,
    Toggle,
    Slider
};

struct Setting {
    std::string id;
    std::string displayName;
    SettingType type;
    std::function<std::string()> getValue;
    std::function<void(const std::string&)> setValue;
    std::string currentValue;
    bool isWaitingForInput = false;
    sf::FloatRect mouseRect;       // Area for mouse interaction
    sf::FloatRect sliderLeftRect;  // Area for slider left arrow
    sf::FloatRect sliderRightRect; // Area for slider right arrow
    int min = 0;        // For sliders
    int max = 100;      // For sliders
    int step = 5;       // For sliders
};

struct UIButton {
    sf::RectangleShape shape;
    sf::Text text;
    bool isHovered = false;
};

class SettingsState : public State {
public:
    SettingsState(Game* game);
    
    void Update(float dt) override;
    void Render() override;
    void ProcessEvent(const sf::Event& event) override;
    
private:
    // Constants
    const float settingsStartY = 100.0f;
    const float settingHeight = 30.0f;
    const float settingOffset = 40.0f;
    
    // Methods
    void InitializeSettings();
    void InitializeButtons();
    
    void AddKeySetting(const std::string& id, const std::string& displayName, 
                      const std::function<sf::Keyboard::Key()>& getKey,
                      const std::function<void(sf::Keyboard::Key)>& setKey);
    
    void AddToggleSetting(const std::string& id, const std::string& displayName,
                         const std::function<bool()>& getBool,
                         const std::function<void(bool)>& setBool);
    
    void AddSliderSetting(const std::string& id, const std::string& displayName,
                         const std::function<int()>& getInt,
                         const std::function<void(int)>& setInt,
                         int min, int max, int step);
    
    void DrawSettings();
    void DrawSelectedIndicator(float yPos);
    void DrawSlider(const Setting& setting, float yPos);
    
    void SaveAndExit();
    void CancelAndExit();
    void ResetToDefaults();
    
    // UI Elements
    sf::Text titleText;
    sf::RectangleShape panelBackground;
    sf::RectangleShape headerBar;
    
    // Buttons
    UIButton saveButton;
    UIButton cancelButton;
    UIButton resetButton;
    
    // Settings data
    std::shared_ptr<SettingsManager> settingsManager;
    GameSettings currentSettings;
    std::vector<Setting> settings;
    
    // State
    int selectedIndex = 0;
    bool waitingForKeyInput = false;
};

#endif // SETTINGS_STATE_H