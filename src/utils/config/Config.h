#ifndef CONFIG_H
#define CONFIG_H

#define GAME_ID "SteamGame_v1"
// Screen dimensions
#define SCREEN_WIDTH 1000
#define SCREEN_HEIGHT 800

// Player configuration
#define PLAYER_SPEED 100.0f
#define PLAYER_HEALTH 1000000

// Enemy configuration
#define ENEMY_SIZE 20.0f
#define ENEMY_ORIGIN 10.0f // Half of ENEMY_SIZE
#define ENEMY_SPEED 30.0f
#define ENEMY_HEALTH 40.0f

// Triangle configuration
#define TRIANGLE_SIZE 10.0f

// Bullet configuration
#define BULLET_SPEED 400.0f
#define BULLET_RADIUS 8.0f
#define BULLET_DAMAGE 20.0f

// Spawning configuration
#define SPAWN_RADIUS 300.0f

#define MAX_PACKET_SIZE 800

// Triangle Enemy configuration
#define TRIANGLE_MIN_SPAWN_DISTANCE 200.0f  // Minimum spawn distance from players
#define TRIANGLE_MAX_SPAWN_DISTANCE 500.0f  // Maximum spawn distance from players
#define TRIANGLE_DAMAGE 10                  // Damage dealt to players on collision
#define TRIANGLE_HEALTH 40                  // Default triangle enemy health
#define TRIANGLE_KILL_REWARD 10             // Money rewarded for killing a triangle

// Wave configuration
#define FIRST_WAVE_ENEMY_COUNT 5           // Number of enemies in first wave
#define BASE_ENEMIES_PER_WAVE 50          // Base number of enemies per wave (after first)
#define ENEMIES_SCALE_PER_WAVE 100           // Additional enemies per wave number
#define WAVE_COOLDOWN_TIME 2.0f            // Seconds between waves

// UI Colors
// Light gray similar to Claude's background - RGB(249, 249, 249)
#define MAIN_BACKGROUND_COLOR sf::Color(70, 70, 70)
constexpr int BASE_WIDTH = 1280;
constexpr int BASE_HEIGHT = 720;

// Network sync constants
#define ENEMY_SYNC_INTERVAL 0.05f
#define FULL_SYNC_INTERVAL 2.0f

// Collision constants
#define COLLISION_RADIUS 25.0f

// Network sync constants
#define ENEMY_SYNC_INTERVAL 0.05f          // Interval for position updates
#define FULL_SYNC_INTERVAL 2.0f            // Interval for full state sync

// Enemy system performance optimization constants
#define MAX_ENEMIES_PER_UPDATE 10          // Max enemies to update in a single sync message
#define MAX_ENEMIES_SPAWNABLE 100000         // Maximum enemies allowed in the game
#define ENEMY_CULLING_DISTANCE 2000.0f     // Distance from player at which enemies are culled

// In Config.h - add these constants
#define MIN_ZOOM 0.5f     // Maximum zoom out (shows more of the map)
#define MAX_ZOOM 2.0f     // Maximum zoom in (shows less of the map)
#define ZOOM_SPEED 0.1f   // How quickly the zoom changes per scroll
#define DEFAULT_ZOOM 1.0f // Default zoom level

#define ENEMY_SPAWN_BATCH_INTERVAL 0.5f
#define ENEMY_SPAWN_BATCH_SIZE 20

#define SHOP_DEFAULT_MAX_LEVEL 10

#define SHOP_BULLET_SPEED_MULTIPLIER 1
#define SHOP_MOVE_SPEED_MULTIPLIER 1 
#define SHOP_HEALTH_INCREASE 1

#define SHOP_COST_INCREMENT 1
#define SHOP_BULLET_SPEED_BASE_COST 1
#define SHOP_MOVE_SPEED_BASE_COST 1
#define SHOP_HEALTH_BASE_COST 1
#endif // CONFIG_H