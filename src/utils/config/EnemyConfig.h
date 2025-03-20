#ifndef ENEMY_CONFIG_H
#define ENEMY_CONFIG_H

// Basic enemy configuration
#define ENEMY_SIZE 20.0f
#define ENEMY_ORIGIN 10.0f // Half of ENEMY_SIZE
#define ENEMY_SPEED 30.0f
#define ENEMY_HEALTH 40.0f

// Spawning configuration
#define SPAWN_RADIUS 300.0f
#define ENEMY_SPAWN_BATCH_INTERVAL 0.1f
#define ENEMY_SPAWN_BATCH_SIZE 10

// Performance optimization constants
#define MAX_ENEMIES_PER_UPDATE 10          // Max enemies to update in a single sync message
#define MAX_ENEMIES_SPAWNABLE 100000       // Maximum enemies allowed in the game
#define ENEMY_CULLING_DISTANCE 2000.0f     // Distance from player at which enemies are culled
#define ENEMY_OPTIMIZATION_THRESHOLD 200

// Triangle Enemy configuration
#define TRIANGLE_SIZE 10.0f
#define TRIANGLE_MIN_SPAWN_DISTANCE 200.0f  // Minimum spawn distance from players
#define TRIANGLE_MAX_SPAWN_DISTANCE 500.0f  // Maximum spawn distance from players
#define TRIANGLE_DAMAGE 10                  // Damage dealt to players on collision
#define TRIANGLE_HEALTH 40                  // Default triangle enemy health
#define TRIANGLE_KILL_REWARD 10             // Money rewarded for killing a triangle

// Visual settings
#define ENEMY_ROTATION_SPEED 90.0f
#define ENEMY_OUTLINE_THICKNESS 1.0f
#define TRIANGLE_ROTATION_OFFSET 90.0f
#define TRIANGLE_FILL_COLOR sf::Color(255, 140, 0)
#define TRIANGLE_OUTLINE_COLOR sf::Color(200, 80, 0)

#endif // ENEMY_CONFIG_H 