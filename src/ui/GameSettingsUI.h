#pragma once

#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <map>
#include <functional>

class Game;
class GameSettingsManager;
class GameSetting;



class GameSettingsUI {
public:
    GameSettingsUI(Game* game, GameSettingsManager* settingsManager);
    
    void Show() { isVisible = true; RefreshUI(); }
    void Hide() { isVisible = false; }
    bool IsVisible() const { return isVisible; }

    void Update(float dt);
    void Render(sf::RenderWindow& window);
    bool ProcessEvent(const sf::Event& event);
    void RefreshUI();
    
private:
    struct Button {
        sf::RectangleShape shape;
        sf::Text text;
        std::function<void()> onClick;
        bool isHovered = false;
    };
    
    struct Slider {
        std::string settingName;
        sf::RectangleShape track;
        sf::RectangleShape handle;
        sf::Text label;
        sf::Text valueText;
        sf::RectangleShape valueTextBackground;
        float minValue;
        float maxValue;
        float value;
        bool isIntegerOnly;
        bool isDragging;
        bool isEditing;
        bool valueTextHovered;
        sf::String editingText;
    };
    
    enum class DialogMode {
        None,
        Save,
        Load
    };
    
    void InitializeUI();
    void CreateSlider(const std::string& settingName, float y);
    void UpdateButtonAppearance(Button& button, const std::string& name);
    void UpdateSliderAppearance(Slider& slider, bool isActive);
    void ApplyChanges();
    bool IsLocalPlayerHost() const;
    bool IsSliderClicked(const Slider& slider, sf::Vector2f mousePos);
    
    float MapPositionToValue(const Slider& slider, float position);
    float MapValueToPosition(const Slider& slider, float value);
    
   
    void UpdateSliderValues();
    void ApplyTextInput(Slider& slider);
    
    void CalculateTotalPages();
    void UpdatePageIndicator();
    void NavigateToPage(int page);
    
    // File dialog related methods
    void ShowSaveDialog();
    void ShowLoadDialog();
    void CloseDialog();
    void RenderDialog(sf::RenderWindow& window);
    bool ProcessDialogEvent(const sf::Event& event);
    void UpdatePresetList();
    
    Game* game;
    GameSettingsManager* settingsManager;
    bool isVisible;
    bool isHostPlayer;
    
    sf::RectangleShape panelBackground;
    sf::Text titleText;
    
    std::vector<Slider> sliders;
    std::map<std::string, Button> buttons;
    
    Button prevPageButton;
    Button nextPageButton;
    sf::Text pageIndicatorText;
    
    int currentPage = 0;
    int totalPages = 1;
    int settingsPerPage = 8;
    
    // File dialog related members
    DialogMode dialogMode = DialogMode::None;
    sf::RectangleShape dialogBackground;
    sf::Text dialogTitleText;
    sf::RectangleShape fileNameBox;
    sf::Text fileNameText;
    sf::String fileNameInput;
    Button dialogSaveButton;
    Button dialogCancelButton;
    Button dialogLoadButton;
    std::vector<Button> presetButtons;
    int selectedPresetIndex = -1;
};