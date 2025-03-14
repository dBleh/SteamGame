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
#define ENEMY_SPEED 70.0f

// Bullet configuration
#define BULLET_SPEED 400.0f

// Spawning configuration
#define SPAWN_RADIUS 300.0f

// UI Colors
// Light gray similar to Claude's background - RGB(249, 249, 249)
#define MAIN_BACKGROUND_COLOR sf::Color(70, 70, 70)
constexpr int BASE_WIDTH = 1280;
constexpr int BASE_HEIGHT = 720;
#endif // CONFIG_H