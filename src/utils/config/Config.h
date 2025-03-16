#ifndef CONFIG_H
#define CONFIG_H

#define GAME_ID "SteamGame_v1"
// Screen dimensions
#define SCREEN_WIDTH 1000
#define SCREEN_HEIGHT 800

// Player configuration
#define PLAYER_SPEED 100.0f
#define PLAYER_HEALTH 10000

// Enemy configuration
#define ENEMY_SIZE 20.0f  // Removed erroneous '='
#define ENEMY_ORIGIN 10.0f // Half of ENEMY_SIZE
#define ENEMY_SPEED 70.0f
#define ENEMY_HEALTH 40.0f

// Triangle configuration
#define TRIANGLE_SIZE 15.0f

// Bullet configuration
#define BULLET_SPEED 400.0f

// Spawning configuration
#define SPAWN_RADIUS 300.0f

#define MAX_PACKET_SIZE 1024

// Triangle Enemy configuration
#define TRIANGLE_MIN_SPAWN_DISTANCE 200.0f  // Minimum spawn distance from players
#define TRIANGLE_MAX_SPAWN_DISTANCE 500.0f  // Maximum spawn distance from players
#define TRIANGLE_DAMAGE 10                  // Damage dealt to players on collision
#define TRIANGLE_HEALTH 40                  // Default triangle enemy health
#define TRIANGLE_KILL_REWARD 10             // Money rewarded for killing a triangle

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

// Triangle Enemy configuration
#define TRIANGLE_MIN_SPAWN_DISTANCE 200.0f  // Minimum spawn distance from players
#define TRIANGLE_MAX_SPAWN_DISTANCE 500.0f  // Maximum spawn distance from players
#define TRIANGLE_DAMAGE 10                  // Damage dealt to players on collision
#define TRIANGLE_HEALTH 40                  // Default triangle enemy health
#define TRIANGLE_KILL_REWARD 10             // Money rewarded for killing a triangle

// Network sync constants
#define ENEMY_SYNC_INTERVAL 0.05f          // Interval for position updates
#define FULL_SYNC_INTERVAL 2.0f            // Interval for full state sync

// Enemy system performance optimization constants
#define MAX_ENEMIES_PER_UPDATE 10          // Max enemies to update in a single sync message
#define MAX_ENEMIES_SPAWNABLE 1000         // Maximum enemies allowed in the game
#define ENEMY_CULLING_DISTANCE 2000.0f     // Distance from player at which enemies are culled

#endif // CONFIG_H