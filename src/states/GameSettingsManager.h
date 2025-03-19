#ifndef GAME_SETTINGS_MANAGER_H
#define GAME_SETTINGS_MANAGER_H

#include <unordered_map>
#include <string>
#include <functional>
#include "../utils/config/Config.h"

// Forward declarations
class Game;
class EnemyManager;  // Add this forward declaration
class PlayingState;
// Struct to hold a setting with min/max/default values
struct GameSetting {
    std::string name;           // Name of the setting
    float value;                // Current value
    float defaultValue;         // Default value
    float minValue;             // Minimum value
    float maxValue;             // Maximum value
    float step;                 // Step amount for slider
    bool isIntegerOnly;         // Whether the setting should be integer only

    // Constructor for integer settings
    GameSetting(const std::string& _name, int _value, int _minValue, int _maxValue, int _defaultValue)
        : name(_name), value(static_cast<float>(_value)), 
          defaultValue(static_cast<float>(_defaultValue)),
          minValue(static_cast<float>(_minValue)), 
          maxValue(static_cast<float>(_maxValue)),
          step(1.0f), isIntegerOnly(true) {}

    // Constructor for float settings
    GameSetting(const std::string& _name, float _value, float _minValue, float _maxValue, 
                float _defaultValue, float _step = 0.1f)
        : name(_name), value(_value), defaultValue(_defaultValue),
          minValue(_minValue), maxValue(_maxValue),
          step(_step), isIntegerOnly(false) {}

    // Get value as integer or float
    int GetIntValue() const { return static_cast<int>(value); }
    float GetFloatValue() const { return value; }

    // Set value with boundary checking
    void SetValue(float newValue) {
        value = std::max(minValue, std::min(maxValue, newValue));
        if (isIntegerOnly) {
            value = std::round(value);
        }
    }
};

/**
 * @brief Manages game settings that can be adjusted by the host.
 * 
 * This class stores and validates game settings like enemy count, wave timing,
 * player health, etc. It also handles serialization/deserialization of settings
 * for network transmission.
 */
class GameSettingsManager {
public:
    // Constructor
    GameSettingsManager(Game* game);
    ~GameSettingsManager() = default;

    // Initialize default settings
    void InitializeDefaultSettings();

    // Get a specific setting
    GameSetting* GetSetting(const std::string& name);

    // Access all settings
    const std::unordered_map<std::string, GameSetting>& GetAllSettings() const { return settings; }

    // Update a specific setting
    bool UpdateSetting(const std::string& name, float value);

    // Reset all settings to defaults
    void ResetToDefaults();

    // Serialize all settings to a string for network transmission
    std::string SerializeSettings() const;

    // Deserialize settings from a network message
    bool DeserializeSettings(const std::string& data);

    // Apply settings to the game
    void ApplySettings();
    void ApplyEnemySettings(EnemyManager* enemyManager);

private:
    Game* game;
    std::unordered_map<std::string, GameSetting> settings;
};

#endif // GAME_SETTINGS_MANAGER_H