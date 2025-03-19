#ifndef GAME_SETTINGS_UI_H
#define GAME_SETTINGS_UI_H

#include <SFML/Graphics.hpp>
#include <unordered_map>
#include <string>
#include <vector>
#include <functional>

// Forward declarations
class Game;
class GameSettingsManager;
struct GameSetting;

/**
 * @brief UI component for displaying and adjusting game settings.
 * 
 * This class provides sliders, buttons, and labels for modifying game settings
 * in the lobby. Only the host can modify settings.
 */
class GameSettingsUI {
public:
    // Constructor and destructor
    GameSettingsUI(Game* game, GameSettingsManager* settingsManager);
    ~GameSettingsUI() = default;
    
    // UI components
    struct Slider {
        sf::RectangleShape track;        // Background track
        sf::RectangleShape handle;       // Draggable handle
        sf::Text label;                  // Label showing name
        sf::Text valueText;              // Text showing current value
        std::string settingName;         // Name of associated setting
        float minValue;                  // Minimum value
        float maxValue;                  // Maximum value
        float value;                     // Current value
        bool isDragging;                 // Whether user is dragging this slider
        bool isIntegerOnly;              // Whether to display as integer
        
        Slider() : isDragging(false), isIntegerOnly(false) {}
    };
    
    struct Button {
        sf::RectangleShape shape;        // Button shape
        sf::Text text;                   // Button text
        std::function<void()> onClick;   // Callback for click
        bool isHovered;                  // Whether mouse is over button
        
        Button() : isHovered(false) {}
    };
    
    // Update and render
    void Update(float dt);
    void Render(sf::RenderWindow& window);
    
    // Event handling
    bool ProcessEvent(const sf::Event& event);
    
    // UI state management
    void Show() { isVisible = true; }
    void Hide() { isVisible = false; }
    bool IsVisible() const { return isVisible; }
    void Toggle() { isVisible = !isVisible; }
    
    // Refreshes UI when settings change
    void RefreshUI();
    
    // Apply changes to game
    void ApplyChanges();
    
private:
    Game* game;
    GameSettingsManager* settingsManager;
    bool isVisible;
    bool isHostPlayer;
    
    // UI elements
    sf::RectangleShape panelBackground;
    sf::Text titleText;
    std::vector<Slider> sliders;
    std::unordered_map<std::string, Button> buttons;
    
    // Helper functions
    void InitializeUI();
    void CreateSlider(const std::string& settingName, float y);
    float MapValueToPosition(const Slider& slider, float value);
    float MapPositionToValue(const Slider& slider, float position);
    void UpdateSliderAppearance(Slider& slider, bool isActive);
    bool IsSliderClicked(const Slider& slider, sf::Vector2f mousePos);
    void UpdateSliderValues();
    void UpdateButtonAppearance(Button& button, const std::string& name);
    
    // Check if local player is host
    bool IsLocalPlayerHost() const;
};

#endif // GAME_SETTINGS_UI_H