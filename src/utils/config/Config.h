#ifndef CONFIG_H
#define CONFIG_H

// Core game identification and window settings
#define GAME_ID "SteamGame_v1"
#define SCREEN_WIDTH 1000
#define SCREEN_HEIGHT 800

// Base dimensions
constexpr int BASE_WIDTH = 1280;
constexpr int BASE_HEIGHT = 720;

// UI Colors
#define MAIN_BACKGROUND_COLOR sf::Color(70, 70, 70)

// Network constants
#define MAX_PACKET_SIZE 800
#define ENEMY_SYNC_INTERVAL 0.05f          // Interval for position updates
#define FULL_SYNC_INTERVAL .5f            // Interval for full state sync

// Include specific configurations
#include "PlayerConfig.h"
#include "EnemyConfig.h"
#include "BulletConfig.h"
#include "GameplayConfig.h"
#include "ForceFieldConfig.h"

#endif // CONFIG_H