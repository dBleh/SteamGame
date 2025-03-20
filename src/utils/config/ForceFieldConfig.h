#ifndef FORCEFIELD_CONFIG_H
#define FORCEFIELD_CONFIG_H

// Force field constants
#define DEFAULT_RADIUS 150.0f        // Default force field radius
#define DEFAULT_COOLDOWN 0.3f        // Default seconds between zaps
#define DEFAULT_DAMAGE 25.0f         // Default damage per zap
#define INITIAL_ZAP_TIMER 0.5f       // Initial value for zapTimer
#define DEFAULT_POWER_LEVEL 1

// Field type probabilities
#define FIELD_TYPE_SHOCK_PROB 25     // Probability for SHOCK type (0-25)
#define FIELD_TYPE_PLASMA_PROB 50    // Probability for PLASMA type (25-50)
#define FIELD_TYPE_VORTEX_PROB 75    // Probability for VORTEX type (50-75)
                                     // STANDARD type (75-100)

// Visual settings
#define FIELD_ZAP_EFFECT_DURATION 0.3f
#define FIELD_INTENSITY_DEFAULT 1.0f
#define FIELD_ROTATION_SPEED 15.0f
#define FIELD_PULSE_SPEED 3.0f
#define FIELD_OUTLINE_THICKNESS 4.0f
#define FIELD_COMBO_DURATION 3.0f
#define FIELD_CHARGE_DECAY_RATE 0.2f
#define FIELD_CHARGE_GAIN_RATE 0.1f
#define FIELD_PLAYER_CENTER_OFFSET 25.0f
#define FIELD_PULSE_FACTOR 1.2f           // Visual pulse size factor
#define FIELD_PULSE_DURATION 0.2f         // Duration of visual pulse effects in seconds

// Math constants
#define PI 3.14159f

// Power level modifiers
#define MAX_POWER_LEVEL 6
#define POWER_LEVEL_RADIUS_FACTOR 0.1f      // Radius increase per power level
#define POWER_LEVEL_COOLDOWN_FACTOR 0.1f    // Cooldown reduction per power level
#define POWER_LEVEL_DAMAGE_FACTOR 0.2f      // Damage increase per power level
#define POWER_LEVEL_COMBO_FACTOR 0.1f       // Additional damage per combo hit
#define CHARGE_COOLDOWN_REDUCTION 0.3f      // Cooldown reduction at max charge
#define POWER_LEVEL_ORB_SIZE_FACTOR 0.1f    // Orb size increase per power level

// Particle system settings
#define MAX_PARTICLES 1000                  // Maximum number of particles
#define PARTICLE_AMBIENT_CHANCE 70          // Percent chance for ambient vs orbiting particles
#define PARTICLE_ORBIT_SPEED_MIN 60.0f      // Minimum orbit speed
#define PARTICLE_ORBIT_SPEED_MAX 120.0f     // Maximum orbit speed
#define AMBIENT_PARTICLE_CREATION_CHANCE 30 // Base chance to create ambient particles per frame
#define PARTICLE_VELOCITY_RANGE 10          // Random velocity range (-range to +range)
#define PARTICLE_VELOCITY_MULTIPLIER 5.0f   // Velocity multiplier
#define PARTICLE_AMBIENT_SIZE_MIN 1.0f      // Minimum ambient particle size
#define PARTICLE_AMBIENT_SIZE_MAX 4.0f      // Maximum ambient particle size
#define PARTICLE_ORBIT_SIZE_MIN 2.0f        // Minimum orbit particle size
#define PARTICLE_ORBIT_SIZE_MAX 6.0f        // Maximum orbit particle size
#define PARTICLE_AMBIENT_LIFETIME_MIN 0.5f  // Minimum ambient particle lifetime
#define PARTICLE_AMBIENT_LIFETIME_MAX 1.5f  // Maximum ambient particle lifetime
#define PARTICLE_ORBIT_LIFETIME_MIN 1.0f    // Minimum orbit particle lifetime
#define PARTICLE_ORBIT_LIFETIME_MAX 3.0f    // Maximum orbit particle lifetime
#define PARTICLE_COLOR_VARIATION 100        // Color variation range
#define PARTICLE_DEFAULT_ALPHA 200          // Default particle alpha

// Impact particle settings
#define IMPACT_PARTICLES_BASE 15            // Base number of impact particles
#define IMPACT_PARTICLES_PER_POWER 5        // Additional particles per power level
#define IMPACT_PARTICLE_SPEED_MIN 50.0f     // Minimum impact particle speed
#define IMPACT_PARTICLE_SPEED_MAX 200.0f    // Maximum impact particle speed
#define IMPACT_PARTICLE_SIZE_MIN 1.0f       // Minimum impact particle size
#define IMPACT_PARTICLE_SIZE_MAX 5.0f       // Maximum impact particle size
#define IMPACT_PARTICLE_LIFETIME_MIN 0.3f   // Minimum impact particle lifetime
#define IMPACT_PARTICLE_LIFETIME_MAX 0.8f   // Maximum impact particle lifetime

// Outline colors for each field type
#define FIELD_SHOCK_OUTLINE_COLOR sf::Color(100, 210, 255, 180)
#define FIELD_PLASMA_OUTLINE_COLOR sf::Color(255, 170, 90, 180)
#define FIELD_VORTEX_OUTLINE_COLOR sf::Color(220, 130, 255, 180)
#define FIELD_STANDARD_OUTLINE_COLOR sf::Color(160, 200, 255, 180)

// Visual effect settings
#define NUM_FIELD_RINGS 3            // Number of decorative rings
#define NUM_ENERGY_ORBS 12           // Number of orbiting energy orbs

// Ring configuration
#define FIELD_RING_INNER_RADIUS_FACTOR 0.4f  // Base factor for innermost ring
#define FIELD_RING_RADIUS_INCREMENT 0.2f     // Increment factor between ring sizes
#define FIELD_RING_MIN_THICKNESS 2.0f        // Minimum ring thickness
#define FIELD_RING_THICKNESS_DECREMENT 0.5f  // Thickness decrease per ring
#define RING_ALPHA_BASE 70.0f                // Base alpha for ring pulsing
#define RING_ALPHA_VARIATION 30.0f           // Alpha variation during pulsing
#define RING_PHASE_OFFSET 0.5f               // Phase offset between rings
#define RING_SCALE_BASE 1.0f                 // Base scale factor for rings
#define RING_SCALE_VARIATION 0.05f           // Scale variation during pulsing
#define RING_PULSE_FREQUENCY 1.5f            // Frequency multiplier for ring pulsing

// Force field color definitions
#define FIELD_STANDARD_COLOR sf::Color(100, 100, 255, 50)
#define FIELD_SHOCK_COLOR sf::Color(70, 130, 255, 50) 
#define FIELD_PLASMA_COLOR sf::Color(255, 90, 40, 50)
#define FIELD_VORTEX_COLOR sf::Color(180, 70, 255, 50)

// Update field color settings
#define FIELD_COLOR_INTENSITY_MIN 0.7f       // Minimum intensity multiplier for color
#define FIELD_COLOR_INTENSITY_MAX 0.3f       // Maximum additional intensity for color
#define FIELD_ZAP_INTENSITY_BONUS 0.5f       // Additional intensity when zapping
#define FIELD_CHARGE_INTENSITY_FACTOR 0.5f   // Intensity increase with charge

// Energy orb settings
#define ENERGY_ORB_MIN_SIZE 5.0f              // Minimum orb size
#define ENERGY_ORB_SIZE_VARIATION 10.0f       // Random size variation
#define ENERGY_ORB_COLOR_GROUPS 3             // Number of color groups
#define ENERGY_ORB_ALPHA 180                  // Base alpha value for orbs
#define ENERGY_ORB_MIN_SPEED 1.0f             // Minimum orbit speed
#define ENERGY_ORB_SPEED_VARIATION 2.0f       // Orbit speed variation factor
#define ENERGY_ORB_INNER_ORBIT_FACTOR 0.7f    // Inner orbit distance factor
#define ENERGY_ORB_MIDDLE_ORBIT_FACTOR 0.85f  // Middle orbit distance factor
#define ENERGY_ORB_OUTER_ORBIT_FACTOR 1.0f    // Outer orbit distance factor
#define ENERGY_ORB_DISTANCE_VARIATION 20.0f   // Random variation in orbit distance
#define ORB_PULSE_SIZE_BASE 1.0f              // Base size factor for orb pulsing
#define ORB_PULSE_SIZE_VARIATION 0.3f         // Size variation during pulsing
#define ORB_PULSE_FREQUENCY 2.0f              // Frequency multiplier for orb pulsing
#define ORB_PHASE_OFFSET 0.9f                 // Phase offset between orbs
#define ORB_ALPHA_VARIATION 40                // Alpha variation during pulsing
#define ORB_SPEED_MULTIPLIER 60.0f            // Base speed multiplier for orbs
#define ORB_INTENSITY_FACTOR 0.5f             // Intensity factor for orb speed

// Shock type orb colors
#define SHOCK_ORB_COLOR_1 sf::Color(220, 240, 255, ENERGY_ORB_ALPHA) // Near white
#define SHOCK_ORB_COLOR_2 sf::Color(80, 180, 255, ENERGY_ORB_ALPHA)  // Electric blue
#define SHOCK_ORB_COLOR_3 sf::Color(0, 220, 230, ENERGY_ORB_ALPHA)   // Cyan
#define UPDATE_SHOCK_ORB_COLOR sf::Color(100, 200, 255, 180 + ORB_ALPHA_VARIATION)

// Plasma type orb colors
#define PLASMA_ORB_COLOR_1 sf::Color(255, 230, 120, ENERGY_ORB_ALPHA) // Yellow
#define PLASMA_ORB_COLOR_2 sf::Color(255, 140, 50, ENERGY_ORB_ALPHA)  // Orange
#define PLASMA_ORB_COLOR_3 sf::Color(255, 60, 30, ENERGY_ORB_ALPHA)   // Red
#define UPDATE_PLASMA_ORB_COLOR sf::Color(255, 150, 100, 180 + ORB_ALPHA_VARIATION)

// Vortex type orb colors
#define VORTEX_ORB_COLOR_1 sf::Color(230, 140, 255, ENERGY_ORB_ALPHA) // Light purple
#define VORTEX_ORB_COLOR_2 sf::Color(180, 70, 255, ENERGY_ORB_ALPHA)  // Medium purple
#define VORTEX_ORB_COLOR_3 sf::Color(140, 0, 230, ENERGY_ORB_ALPHA)   // Deep purple
#define UPDATE_VORTEX_ORB_COLOR sf::Color(180, 100, 255, 180 + ORB_ALPHA_VARIATION)

// Standard type orb colors
#define STANDARD_ORB_COLOR_1 sf::Color(180, 220, 255, ENERGY_ORB_ALPHA) // Light blue
#define STANDARD_ORB_COLOR_2 sf::Color(130, 200, 220, ENERGY_ORB_ALPHA) // Teal
#define STANDARD_ORB_COLOR_3 sf::Color(180, 255, 220, ENERGY_ORB_ALPHA) // Aqua
#define UPDATE_STANDARD_ORB_COLOR sf::Color(200, 200, 255, 180 + ORB_ALPHA_VARIATION)

// Ring colors for each field type (based on index i)
// These are template macros for color calculation
#define SHOCK_RING_COLOR(i) sf::Color(70 + (i) * 30, 170 + (i) * 20, 255, 120 - (i) * 20)
#define PLASMA_RING_COLOR(i) sf::Color(255, 100 + (i) * 50, 50 + (i) * 40, 120 - (i) * 20)
#define VORTEX_RING_COLOR(i) sf::Color(180 + (i) * 20, 70 + (i) * 10, 255 - (i) * 30, 120 - (i) * 20)
#define STANDARD_RING_COLOR(i) sf::Color(120 + (i) * 10, 170 + (i) * 20, 255 - (i) * 10, 120 - (i) * 20)

// Zap effect settings
#define ZAP_BASE_SEGMENTS 12                  // Base number of segments for zap effect
#define ZAP_SEGMENTS_PER_POWER 2              // Additional segments per power level
#define ZAP_OFFSET_MIN -60                    // Minimum zap offset
#define ZAP_OFFSET_MAX 60                     // Maximum zap offset
#define ZAP_OFFSET_POWER_FACTOR 0.2f          // Power level impact on zap offset
#define ZAP_THICKNESS_BASE 2.0f               // Base thickness of zap effect
#define ZAP_THICKNESS_POWER_FACTOR 0.2f       // Power level impact on zap thickness
#define ZAP_BRANCH_CHANCE_BASE 20             // Base chance for branches
#define ZAP_BRANCH_CHANCE_PER_POWER 10        // Additional branch chance per power level
#define ZAP_BRANCH_MIN_SEGMENTS 3             // Minimum segments in a branch
#define ZAP_BRANCH_ANGLE_MIN -30              // Minimum branch angle adjustment
#define ZAP_BRANCH_ANGLE_MAX 30               // Maximum branch angle adjustment
#define ZAP_BRANCH_LENGTH_FACTOR 0.2f         // Branch length as factor of main zap
#define ZAP_BRANCH_LENGTH_VARIATION 0.2f      // Random variation in branch length
#define ZAP_BRANCH_RANDOMNESS 40              // Random offset for branch segments
#define ZAP_SUB_BRANCH_POWER_MIN 3            // Minimum power level for sub-branches
#define ZAP_SUB_BRANCH_CHANCE 30              // Chance for sub-branches
#define ZAP_SUB_BRANCH_LENGTH 0.4f            // Sub-branch length as factor of branch

// Chain lightning settings
#define FIELD_DEFAULT_CHAIN_TARGETS 3
#define CHAIN_DAMAGE_FACTOR 0.6f              // Damage as portion of primary zap
#define CHAIN_POWER_FACTOR 0.1f               // Power level impact on chain damage
#define CHAIN_DAMAGE_DROPOFF 0.1f             // Damage reduction per jump
#define CHAIN_ZAP_BASE_SEGMENTS 8             // Base number of segments for chain effect
#define CHAIN_OFFSET_MIN -40                  // Minimum chain offset
#define CHAIN_OFFSET_MAX 40                   // Maximum chain offset
#define CHAIN_OFFSET_POWER_FACTOR 0.1f        // Power level impact on chain offset

// Zap colors
#define ZAP_SHOCK_BASE_COLOR sf::Color(80, 180, 255, 255)
#define ZAP_SHOCK_BRIGHT_COLOR sf::Color(180, 230, 255, 255)
#define ZAP_PLASMA_BASE_COLOR sf::Color(255, 150, 80, 255)
#define ZAP_PLASMA_BRIGHT_COLOR sf::Color(255, 220, 180, 255)
#define ZAP_VORTEX_BASE_COLOR sf::Color(180, 80, 255, 255)
#define ZAP_VORTEX_BRIGHT_COLOR sf::Color(220, 180, 255, 255)
#define ZAP_STANDARD_BASE_COLOR sf::Color(150, 220, 255, 255)
#define ZAP_STANDARD_BRIGHT_COLOR sf::Color(200, 240, 255, 255)

// Chain lightning colors
#define CHAIN_SHOCK_BASE_COLOR sf::Color(80, 180, 255, 200)
#define CHAIN_SHOCK_BRIGHT_COLOR sf::Color(180, 230, 255, 180)
#define CHAIN_PLASMA_BASE_COLOR sf::Color(255, 150, 80, 200)
#define CHAIN_PLASMA_BRIGHT_COLOR sf::Color(255, 220, 180, 180)
#define CHAIN_VORTEX_BASE_COLOR sf::Color(180, 80, 255, 200)
#define CHAIN_VORTEX_BRIGHT_COLOR sf::Color(220, 180, 255, 180)
#define CHAIN_STANDARD_BASE_COLOR sf::Color(150, 220, 255, 200)
#define CHAIN_STANDARD_BRIGHT_COLOR sf::Color(200, 240, 255, 180)

// Zap effect rendering settings
#define ZAP_GLOW_RADIUS_BASE 20.0f          // Base radius for background glow
#define ZAP_GLOW_RADIUS_PER_POWER 5.0f      // Additional radius per power level
#define ZAP_PRIMARY_GLOW_RADIUS_BASE 8.0f   // Base radius for primary glow
#define ZAP_PRIMARY_GLOW_RADIUS_PER_POWER 2.0f // Additional primary glow radius per power level
#define ZAP_SPARKLE_MIN_POWER 2             // Minimum power level for sparkle effects
#define ZAP_SPARKLE_BASE 10                 // Base number of sparkles
#define ZAP_SPARKLE_PER_POWER 5             // Additional sparkles per power level
#define ZAP_SPARKLE_RADIUS 2.0f             // Base sparkle radius
#define ZAP_SPARKLE_OFFSET_MIN -10          // Minimum sparkle offset
#define ZAP_SPARKLE_OFFSET_MAX 10           // Maximum sparkle offset
#define ZAP_SPARKLE_OFFSET_POWER_FACTOR 0.2f // Power level impact on sparkle offset
#define ZAP_IMPACT_FLASH_RADIUS_BASE 15.0f  // Base radius for impact flash
#define ZAP_IMPACT_FLASH_RADIUS_PER_POWER 5.0f // Additional radius per power level
#define ZAP_IMPACT_PULSE_MIN 0.7f           // Minimum impact pulse scale
#define ZAP_IMPACT_PULSE_MAX 0.3f           // Maximum additional impact pulse scale
#define ZAP_IMPACT_PULSE_FREQUENCY 8.0f     // Frequency of impact pulse
#define ZAP_IMPACT_RING_DURATION_FACTOR 0.7f // Duration factor for impact rings
#define ZAP_CRITICAL_COMBO_THRESHOLD 3      // Minimum combo for critical hit effects
#define ZAP_CRITICAL_CHARGE_THRESHOLD 0.8f  // Minimum charge for critical hit effects
#define ZAP_SHOCKWAVE_SIZE_BASE 60.0f       // Base size for shockwave
#define ZAP_SHOCKWAVE_POWER_FACTOR 0.2f     // Power level impact on shockwave size

// Chain lightning rendering settings
#define CHAIN_GLOW_RADIUS_BASE 5.0f         // Base radius for chain glow
#define CHAIN_GLOW_ALPHA 60                 // Alpha value for chain glow

// Power indicator settings
#define POWER_MARKER_RADIUS 5.0f            // Radius of power level markers
#define POWER_MARKER_SIDES 4                // Number of sides (4 = diamond)
#define POWER_MARKER_DISTANCE_FACTOR 1.2f   // Distance factor for markers
#define POWER_MARKER_PULSE_MIN 1.0f         // Minimum pulse scale
#define POWER_MARKER_PULSE_MAX 0.3f         // Maximum additional pulse scale
#define POWER_MARKER_PULSE_FREQUENCY 3.0f   // Frequency of marker pulse
#define POWER_MARKER_ALPHA 200             // Alpha value for power markers
#define POWER_INDICATOR_MIN 0.1f            // Minimum charge level to show indicator
#define POWER_MIN_LEVEL 1                   // Minimum power level to show indicator
#define CHARGE_BAR_WIDTH 50.0f              // Width of full charge bar
#define CHARGE_BAR_HEIGHT 4.0f              // Height of charge bar
#define CHARGE_BAR_DISTANCE_FACTOR 1.3f     // Distance factor for charge bar
#define CHARGE_DISPLAY_THRESHOLD 0.05f      // Minimum charge to display bar
#define CHARGE_HIGH_THRESHOLD 0.8f          // Threshold for high charge pulse effect
#define CHARGE_PULSE_ALPHA_MIN 150          // Minimum alpha for charge pulse
#define CHARGE_PULSE_ALPHA_MAX 105          // Maximum additional alpha for charge pulse
#define CHARGE_PULSE_FREQUENCY 5.0f         // Frequency of charge pulse

// Enemy detection settings
#define ENEMY_SAMPLING_BASE 200             // Base number of sampling points
#define ENEMY_SAMPLING_PER_POWER 50         // Additional sampling points per power level
#define ENEMY_DETECTION_RADIUS 10.0f        // Radius for collision detection

// Field update colors
#define UPDATE_SHOCK_FIELD_COLOR sf::Color(50, 150, 255, 50)
#define UPDATE_SHOCK_OUTLINE_COLOR sf::Color(100, 200, 255, 180)
#define UPDATE_PLASMA_FIELD_COLOR sf::Color(255, 100, 50, 50)
#define UPDATE_PLASMA_OUTLINE_COLOR sf::Color(255, 150, 100, 180)
#define UPDATE_VORTEX_FIELD_COLOR sf::Color(150, 50, 255, 50)
#define UPDATE_VORTEX_OUTLINE_COLOR sf::Color(200, 100, 255, 180)
#define UPDATE_STANDARD_FIELD_COLOR sf::Color(100, 100, 255, 50)
#define UPDATE_STANDARD_OUTLINE_COLOR sf::Color(150, 150, 255, 180)

// Combat rewards
#define FIELD_ZAP_HIT_REWARD 10
#define ENEMY_KILL_REWARD 50

#endif // FORCEFIELD_CONFIG_H