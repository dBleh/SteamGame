#ifndef GAMEPLAY_CONFIG_H
#define GAMEPLAY_CONFIG_H

// Wave configuration
#define FIRST_WAVE_ENEMY_COUNT 5           // Number of enemies in first wave
#define BASE_ENEMIES_PER_WAVE 50           // Base number of enemies per wave (after first)
#define ENEMIES_SCALE_PER_WAVE 100         // Additional enemies per wave number
#define WAVE_COOLDOWN_TIME 2.0f            // Seconds between waves

// Camera/zoom configuration
#define MIN_ZOOM 0.5f     // Maximum zoom out (shows more of the map)
#define MAX_ZOOM 2.0f     // Maximum zoom in (shows less of the map)
#define ZOOM_SPEED 0.1f   // How quickly the zoom changes per scroll
#define DEFAULT_ZOOM 1.0f // Default zoom level

// Shop configuration
#define SHOP_DEFAULT_MAX_LEVEL 10
#define SHOP_COST_INCREMENT 1

#endif // GAMEPLAY_CONFIG_H