#include "GameSettingsManager.h"

#include "../core/Game.h"
#include "PlayingState.h"
#include "../entities/enemies/EnemyManager.h"
#include <sstream>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <ctime>

namespace fs = std::filesystem;

GameSettingsManager::GameSettingsManager(Game* game)
    : game(game) {
    InitializeDefaultSettings();
    LoadPresets();
    LoadMostRecentSettingsFile();
}

void GameSettingsManager::InitializeDefaultSettings() {
    // ===== PLAYER SETTINGS =====
    // Basic player attributes
    settings.emplace("player_health", 
                     GameSetting("Player Health", 
                                PLAYER_HEALTH, 
                                100, 2000, PLAYER_HEALTH));
    
    settings.emplace("player_speed", 
                     GameSetting("Player Speed", 
                                PLAYER_SPEED, 
                                50.0f, 300.0f, PLAYER_SPEED, 10.0f));
    
    // Weapon/combat settings
    settings.emplace("bullet_damage", 
                     GameSetting("Bullet Damage", 
                                BULLET_DAMAGE, 
                                5.0f, 100.0f, BULLET_DAMAGE, 5.0f));
    
    settings.emplace("bullet_speed", 
                     GameSetting("Bullet Speed", 
                                BULLET_SPEED, 
                                200.0f, 800.0f, BULLET_SPEED, 50.0f));
    
    settings.emplace("bullet_radius", 
                     GameSetting("Bullet Radius", 
                                BULLET_RADIUS, 
                                2.0f, 20.0f, BULLET_RADIUS, 1.0f));
                                
    // ===== ENEMY SETTINGS =====
    // Base enemy attributes
    settings.emplace("enemy_health", 
                     GameSetting("Enemy Health", 
                                ENEMY_HEALTH, 
                                10.0f, 200.0f, ENEMY_HEALTH, 10.0f));
    
    settings.emplace("enemy_speed", 
                     GameSetting("Enemy Speed", 
                                ENEMY_SPEED, 
                                10.0f, 100.0f, ENEMY_SPEED, 5.0f));
    
    settings.emplace("enemy_size", 
                     GameSetting("Enemy Size", 
                                ENEMY_SIZE, 
                                1.0f, 20.0f, ENEMY_SIZE, 1.0f));
    
    // Triangle enemy specific
    settings.emplace("triangle_size", 
                     GameSetting("Triangle Size", 
                                TRIANGLE_SIZE, 
                                3.0f, 30.0f, TRIANGLE_SIZE, 1.0f));
    
    settings.emplace("triangle_health", 
                     GameSetting("Triangle Health", 
                                TRIANGLE_HEALTH, 
                                10, 200, TRIANGLE_HEALTH));
    
    settings.emplace("triangle_damage", 
                     GameSetting("Triangle Enemy Damage", 
                                TRIANGLE_DAMAGE, 
                                5, 50, TRIANGLE_DAMAGE));
    
    settings.emplace("triangle_kill_reward", 
                     GameSetting("Triangle Kill Reward", 
                                TRIANGLE_KILL_REWARD, 
                                5, 100, TRIANGLE_KILL_REWARD));
    
    // ===== WAVE SETTINGS =====
    settings.emplace("first_wave_enemy_count", 
                     GameSetting("First Wave Enemy Count", 
                                FIRST_WAVE_ENEMY_COUNT, 
                                1, 50, FIRST_WAVE_ENEMY_COUNT));
    
    settings.emplace("base_enemies_per_wave", 
                     GameSetting("Base Enemies Per Wave", 
                                BASE_ENEMIES_PER_WAVE, 
                                10, 200, BASE_ENEMIES_PER_WAVE));
    
    settings.emplace("enemies_scale_per_wave", 
                     GameSetting("Enemies Scale Per Wave", 
                                ENEMIES_SCALE_PER_WAVE, 
                                10, 300, ENEMIES_SCALE_PER_WAVE));
    
    settings.emplace("wave_cooldown_time", 
                     GameSetting("Wave Cooldown (sec)", 
                                WAVE_COOLDOWN_TIME, 
                                1.0f, 10.0f, WAVE_COOLDOWN_TIME, 0.5f));
    
    // ===== SPAWN SETTINGS =====
    settings.emplace("spawn_radius", 
                     GameSetting("Spawn Radius", 
                                SPAWN_RADIUS, 
                                100.0f, 1000.0f, SPAWN_RADIUS, 50.0f));
    
    settings.emplace("triangle_min_spawn_distance", 
                     GameSetting("Triangle Min Spawn Distance", 
                                TRIANGLE_MIN_SPAWN_DISTANCE, 
                                100.0f, 500.0f, TRIANGLE_MIN_SPAWN_DISTANCE, 50.0f));
    
    settings.emplace("triangle_max_spawn_distance", 
                     GameSetting("Triangle Max Spawn Distance", 
                                TRIANGLE_MAX_SPAWN_DISTANCE, 
                                300.0f, 1000.0f, TRIANGLE_MAX_SPAWN_DISTANCE, 50.0f));
    
    settings.emplace("enemy_spawn_batch_interval", 
                     GameSetting("Enemy Spawn Batch Interval", 
                                ENEMY_SPAWN_BATCH_INTERVAL, 
                                0.1f, 2.0f, ENEMY_SPAWN_BATCH_INTERVAL, 0.1f));
    
    settings.emplace("enemy_spawn_batch_size", 
                     GameSetting("Enemy Spawn Batch Size", 
                                ENEMY_SPAWN_BATCH_SIZE, 
                                5, 50, ENEMY_SPAWN_BATCH_SIZE));
    
    settings.emplace("max_enemies_spawnable", 
                     GameSetting("Max Spawnable Enemies", 
                                MAX_ENEMIES_SPAWNABLE, 
                                100, 2000, MAX_ENEMIES_SPAWNABLE));
    
    // ===== COLLISION SETTINGS =====
    settings.emplace("collision_radius", 
                     GameSetting("Collision Radius", 
                                COLLISION_RADIUS, 
                                10.0f, 50.0f, COLLISION_RADIUS, 5.0f));
    
    // ===== SHOP SETTINGS =====
    settings.emplace("shop_default_max_level", 
                     GameSetting("Shop Default Max Level", 
                                SHOP_DEFAULT_MAX_LEVEL, 
                                5, 20, SHOP_DEFAULT_MAX_LEVEL));
    
    settings.emplace("shop_bullet_speed_multiplier", 
                     GameSetting("Shop Bullet Speed Multiplier", 
                                SHOP_BULLET_SPEED_MULTIPLIER, 
                                1, 10, SHOP_BULLET_SPEED_MULTIPLIER));
    
    settings.emplace("shop_move_speed_multiplier", 
                     GameSetting("Shop Move Speed Multiplier", 
                                SHOP_MOVE_SPEED_MULTIPLIER, 
                                1, 10, SHOP_MOVE_SPEED_MULTIPLIER));
    
    settings.emplace("shop_health_increase", 
                     GameSetting("Shop Health Increase", 
                                SHOP_HEALTH_INCREASE, 
                                1, 10, SHOP_HEALTH_INCREASE));
    
    settings.emplace("shop_cost_increment", 
                     GameSetting("Shop Cost Increment", 
                                SHOP_COST_INCREMENT, 
                                1, 10, SHOP_COST_INCREMENT));
    
    settings.emplace("shop_bullet_speed_base_cost", 
                     GameSetting("Shop Bullet Speed Base Cost", 
                                SHOP_BULLET_SPEED_BASE_COST, 
                                1, 100, SHOP_BULLET_SPEED_BASE_COST));
    
    settings.emplace("shop_move_speed_base_cost", 
                     GameSetting("Shop Move Speed Base Cost", 
                                SHOP_MOVE_SPEED_BASE_COST, 
                                1, 100, SHOP_MOVE_SPEED_BASE_COST));
    
    settings.emplace("shop_health_base_cost", 
                     GameSetting("Shop Health Base Cost", 
                                SHOP_HEALTH_BASE_COST, 
                                1, 100, SHOP_HEALTH_BASE_COST));
    
    // ===== NETWORK SETTINGS =====
    settings.emplace("enemy_sync_interval", 
                     GameSetting("Enemy Sync Interval", 
                                ENEMY_SYNC_INTERVAL, 
                                0.01f, 0.2f, ENEMY_SYNC_INTERVAL, 0.01f));
    
    settings.emplace("full_sync_interval", 
                     GameSetting("Full Sync Interval", 
                                FULL_SYNC_INTERVAL, 
                                0.5f, 5.0f, FULL_SYNC_INTERVAL, 0.5f));
    
    settings.emplace("max_enemies_per_update", 
                     GameSetting("Max Enemies Per Update", 
                                MAX_ENEMIES_PER_UPDATE, 
                                5, 30, MAX_ENEMIES_PER_UPDATE));
    
    settings.emplace("enemy_culling_distance", 
                     GameSetting("Enemy Culling Distance", 
                                ENEMY_CULLING_DISTANCE, 
                                1000.0f, 5000.0f, ENEMY_CULLING_DISTANCE, 500.0f));
    
    // ===== CAMERA SETTINGS =====
    settings.emplace("min_zoom", 
                     GameSetting("Minimum Zoom", 
                                MIN_ZOOM, 
                                0.1f, 1.0f, MIN_ZOOM, 0.1f));
    
    settings.emplace("max_zoom", 
                     GameSetting("Maximum Zoom", 
                                MAX_ZOOM, 
                                1.0f, 5.0f, MAX_ZOOM, 0.5f));
    
    settings.emplace("zoom_speed", 
                     GameSetting("Zoom Speed", 
                                ZOOM_SPEED, 
                                0.05f, 0.5f, ZOOM_SPEED, 0.05f));
    
    settings.emplace("default_zoom", 
                     GameSetting("Default Zoom", 
                                DEFAULT_ZOOM, 
                                0.5f, 2.0f, DEFAULT_ZOOM, 0.1f));
}

GameSetting* GameSettingsManager::GetSetting(const std::string& name) {
    auto it = settings.find(name);
    if (it != settings.end()) {
        return &it->second;
    }
    return nullptr;
}

bool GameSettingsManager::UpdateSetting(const std::string& name, float value) {
    auto setting = GetSetting(name);
    if (setting) {
        setting->SetValue(value);
        
        if (game && game->GetCurrentState() == GameState::Playing) {
            PlayingState* playingState = GetPlayingState(game);
            if (playingState) {
                std::cout << "[GameSettingsManager] Notifying PlayingState of settings change" << std::endl;
                playingState->OnSettingsChanged();
            }
        }
        
        return true;
    }
    return false;
}

void GameSettingsManager::ResetToDefaults() {
    for (auto& [name, setting] : settings) {
        setting.value = setting.defaultValue;
    }
}

std::string GameSettingsManager::SerializeSettings() const {
    std::ostringstream oss;
    bool first = true;
    
    for (const auto& [name, setting] : settings) {
        if (!first) {
            oss << ";";
        }
        first = false;
        
        oss << name << ":" << setting.value;
    }
    
    return oss.str();
}

// In GameSettingsManager.cpp, modify the DeserializeSettings method
bool GameSettingsManager::DeserializeSettings(const std::string& data) {
    std::istringstream iss(data);
    std::string pair;
    
    // Set a flag to prevent recursive notifications during deserialization
    static bool isDeserializing = false;
    bool wasDeserializing = isDeserializing;
    isDeserializing = true;
    
    while (std::getline(iss, pair, ';')) {
        size_t colonPos = pair.find(':');
        if (colonPos == std::string::npos) {
            std::cerr << "[GameSettingsManager] Invalid setting pair: " << pair << std::endl;
            continue;
        }
        
        std::string name = pair.substr(0, colonPos);
        float value = std::stof(pair.substr(colonPos + 1));
        
        // Update setting without triggering notifications during deserialization
        auto setting = GetSetting(name);
        if (setting) {
            setting->SetValue(value);
        } else {
            std::cerr << "[GameSettingsManager] Unknown setting: " << name << std::endl;
        }
    }
    
    // Only notify of settings changes once after all settings are deserialized
    if (!wasDeserializing && game && game->GetCurrentState() == GameState::Playing) {
        PlayingState* playingState = GetPlayingState(game);
        if (playingState) {
            std::cout << "[GameSettingsManager] Notifying PlayingState of settings change" << std::endl;
            playingState->OnSettingsChanged();
        }
    }
    
    isDeserializing = wasDeserializing;
    return true;
}

void GameSettingsManager::ApplySettings() {
    // Note: Since we can't directly modify the #define constants at runtime,
    // other parts of the code will need to reference these values instead
    
    // For now, we can just print the values as a demonstration
    std::cout << "[GameSettingsManager] Applying settings:" << std::endl;
    for (const auto& [name, setting] : settings) {
        std::cout << "  " << name << ": " << setting.value;
        if (setting.isIntegerOnly) {
            std::cout << " (int: " << setting.GetIntValue() << ")";
        }
        std::cout << std::endl;
    }
    
    // In a real implementation, you would store these values somewhere 
    // accessible to the game systems
}

void GameSettingsManager::ApplyEnemySettings(EnemyManager* enemyManager) {
    if (!enemyManager) return;
    
    // Apply settings to the enemy manager
    enemyManager->ApplySettings();
}

std::string GameSettingsManager::GetSettingsDirectory() const {
    // Create settings directory if it doesn't exist
    std::string settingsDir = "settings";
    
    if (!fs::exists(settingsDir)) {
        fs::create_directory(settingsDir);
    }
    
    return settingsDir;
}

bool GameSettingsManager::SaveSettings(const std::string& fileName) {
    std::string settingsDir = GetSettingsDirectory();
    std::string filePath = settingsDir + "/" + fileName;
    
    // Ensure file has .cfg extension
    if (filePath.find(".cfg") == std::string::npos) {
        filePath += ".cfg";
    }
    
    std::ofstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "[GameSettingsManager] Failed to open file for saving: " << filePath << std::endl;
        return false;
    }
    
    // Serialize settings and write to file
    file << SerializeSettings();
    
    // Update most recent settings file
    SetMostRecentSettingsFile(filePath);
    
    std::cout << "[GameSettingsManager] Settings saved to: " << filePath << std::endl;
    return true;
}

bool GameSettingsManager::LoadSettings(const std::string& fileName) {
    std::string settingsDir = GetSettingsDirectory();
    std::string filePath = fileName;
    
    // If fileName doesn't include directory path, add it
    if (fileName.find("/") == std::string::npos && fileName.find("\\") == std::string::npos) {
        filePath = settingsDir + "/" + fileName;
    }
    
    // Ensure file has .cfg extension
    if (filePath.find(".cfg") == std::string::npos) {
        filePath += ".cfg";
    }
    
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "[GameSettingsManager] Failed to open file for loading: " << filePath << std::endl;
        return false;
    }
    
    // Read settings from file
    std::string settingsData;
    std::getline(file, settingsData);
    
    // Deserialize and apply settings
    bool result = DeserializeSettings(settingsData);
    
    if (result) {
        // Update most recent settings file
        SetMostRecentSettingsFile(filePath);
        std::cout << "[GameSettingsManager] Settings loaded from: " << filePath << std::endl;
    }
    
    return result;
}

void GameSettingsManager::LoadPresets() {
    std::string settingsDir = GetSettingsDirectory();
    
    // Add default preset
    SettingsPreset defaultPreset;
    defaultPreset.name = "Default";
    defaultPreset.filePath = "";
    defaultPreset.isDefault = true;
    presets.push_back(defaultPreset);
    
    // Check if directory exists
    if (!fs::exists(settingsDir)) {
        return;
    }
    
    // Load presets from settings directory
    for (const auto& entry : fs::directory_iterator(settingsDir)) {
        if (entry.path().extension() == ".cfg") {
            SettingsPreset preset;
            preset.name = entry.path().stem().string();
            preset.filePath = entry.path().string();
            preset.isDefault = false;
            presets.push_back(preset);
        }
    }
}

void GameSettingsManager::SetMostRecentSettingsFile(const std::string& filePath) {
    mostRecentSettingsFile = filePath;
    SaveMostRecentSettingsFile();
}

std::string GameSettingsManager::GetMostRecentSettingsFile() const {
    return mostRecentSettingsFile;
}

void GameSettingsManager::SaveMostRecentSettingsFile() {
    std::string settingsDir = GetSettingsDirectory();
    std::string filePath = settingsDir + "/recent.txt";
    
    std::ofstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "[GameSettingsManager] Failed to save most recent settings file" << std::endl;
        return;
    }
    
    file << mostRecentSettingsFile;
}

bool GameSettingsManager::LoadMostRecentSettingsFile() {
    std::string settingsDir = GetSettingsDirectory();
    std::string filePath = settingsDir + "/recent.txt";
    
    // If the file doesn't exist, there's no recent settings file
    if (!fs::exists(filePath)) {
        return false;
    }
    
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "[GameSettingsManager] Failed to load most recent settings file" << std::endl;
        return false;
    }
    
    std::getline(file, mostRecentSettingsFile);
    
    // If the file exists but is empty, there's no recent settings file
    if (mostRecentSettingsFile.empty()) {
        return false;
    }
    
    return true;
}

bool GameSettingsManager::LoadMostRecentSettings() {
    // If no recent settings file, or it doesn't exist, return false
    if (mostRecentSettingsFile.empty() || !fs::exists(mostRecentSettingsFile)) {
        return false;
    }
    
    return LoadSettings(mostRecentSettingsFile);
}

void GameSettingsManager::RefreshPresets() {
    presets.clear();
    LoadPresets();
}
