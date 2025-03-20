#ifndef PLAYER_CONFIG_H
#define PLAYER_CONFIG_H

// Player movement and stats
#define PLAYER_SPEED 100.0f
#define PLAYER_DEFAULT_MOVE_SPEED 200.0f    // Initial movement speed (before upgrades)
#define PLAYER_HEALTH 1000000

// Player shop upgrade multipliers
#define SHOP_MOVE_SPEED_MULTIPLIER 1 
#define SHOP_HEALTH_INCREASE 1
#define SHOP_MOVE_SPEED_BASE_COST 1
#define SHOP_HEALTH_BASE_COST 1

// Shooting configuration
#define SHOOT_COOLDOWN_DURATION 0.1f  // seconds between shots
#define RESPAWN_TIME 3.0f

// Collision constants
#define COLLISION_RADIUS 25.0f

// Player appearance
#define PLAYER_WIDTH 50.0f
#define PLAYER_HEIGHT 50.0f
#define PLAYER_DEFAULT_COLOR sf::Color::Blue
#define PLAYER_DEFAULT_START_X 100.0f
#define PLAYER_DEFAULT_START_Y 100.0f

// UI settings
#define PLAYER_NAME_FONT_SIZE 16
#define PLAYER_NAME_COLOR sf::Color::Black
#define PLAYER_INTERP_DURATION 0.1f

// Gameplay configuration
#define BULLET_CLEANUP_DISTANCE 1000.0f

#endif // PLAYER_CONFIG_H