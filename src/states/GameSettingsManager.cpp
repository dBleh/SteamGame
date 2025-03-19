#include "GameSettingsManager.h"
#include "../core/Game.h"
#include "PlayingState.h"
#include "../entities/enemies/EnemyManager.h"
#include <sstream>
#include <iostream>

GameSettingsManager::GameSettingsManager(Game* game)
    : game(game) {
    InitializeDefaultSettings();
}

void GameSettingsManager::InitializeDefaultSettings() {
    // Wave settings
    settings.emplace("base_enemies_per_wave", 
                     GameSetting("Base Enemies Per Wave", 
                                BASE_ENEMIES_PER_WAVE, 
                                10, 200, BASE_ENEMIES_PER_WAVE));
    
    settings.emplace("enemies_scale_per_wave", 
                     GameSetting("Enemies Scale Per Wave", 
                                ENEMIES_SCALE_PER_WAVE, 
                                10, 300, ENEMIES_SCALE_PER_WAVE));
    
    settings.emplace("first_wave_enemy_count", 
                     GameSetting("First Wave Enemy Count", 
                                FIRST_WAVE_ENEMY_COUNT, 
                                1, 50, FIRST_WAVE_ENEMY_COUNT));
    
    settings.emplace("wave_cooldown_time", 
                     GameSetting("Wave Cooldown (sec)", 
                                WAVE_COOLDOWN_TIME, 
                                1.0f, 10.0f, WAVE_COOLDOWN_TIME, 0.5f));
    
    // Enemy settings
    settings.emplace("enemy_health", 
                     GameSetting("Enemy Health", 
                                ENEMY_HEALTH, 
                                10.0f, 200.0f, ENEMY_HEALTH, 10.0f));
    
    settings.emplace("enemy_speed", 
                     GameSetting("Enemy Speed", 
                                ENEMY_SPEED, 
                                10.0f, 100.0f, ENEMY_SPEED, 5.0f));
    
    settings.emplace("triangle_damage", 
                     GameSetting("Triangle Enemy Damage", 
                                TRIANGLE_DAMAGE, 
                                5, 50, TRIANGLE_DAMAGE));
    
    // Player settings
    settings.emplace("player_speed", 
                     GameSetting("Player Speed", 
                                PLAYER_SPEED, 
                                50.0f, 300.0f, PLAYER_SPEED, 10.0f));
    
    settings.emplace("bullet_damage", 
                     GameSetting("Bullet Damage", 
                                BULLET_DAMAGE, 
                                5.0f, 100.0f, BULLET_DAMAGE, 5.0f));
    
    settings.emplace("bullet_speed", 
                     GameSetting("Bullet Speed", 
                                BULLET_SPEED, 
                                200.0f, 800.0f, BULLET_SPEED, 50.0f));
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

bool GameSettingsManager::DeserializeSettings(const std::string& data) {
    std::istringstream iss(data);
    std::string pair;
    
    while (std::getline(iss, pair, ';')) {
        size_t colonPos = pair.find(':');
        if (colonPos == std::string::npos) {
            std::cerr << "[GameSettingsManager] Invalid setting pair: " << pair << std::endl;
            continue;
        }
        
        std::string name = pair.substr(0, colonPos);
        float value = std::stof(pair.substr(colonPos + 1));
        
        if (!UpdateSetting(name, value)) {
            std::cerr << "[GameSettingsManager] Unknown setting: " << name << std::endl;
        }
    }
    
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