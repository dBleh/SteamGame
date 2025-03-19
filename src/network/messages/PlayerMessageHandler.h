#ifndef PLAYER_MESSAGE_HANDLER_H
#define PLAYER_MESSAGE_HANDLER_H

#include <string>
#include <vector>
#include <SFML/Graphics.hpp>
#include <steam/steam_api.h>

// Forward declarations
struct ParsedMessage;

class PlayerMessageHandler {
public:
    // Initialize and register player message handlers
    static void Initialize();
    
    // Message parsing functions
    static ParsedMessage ParseConnectionMessage(const std::vector<std::string>& parts);
    static ParsedMessage ParseMovementMessage(const std::vector<std::string>& parts);
    static ParsedMessage ParseBulletMessage(const std::vector<std::string>& parts);
    static ParsedMessage ParsePlayerDeathMessage(const std::vector<std::string>& parts);
    static ParsedMessage ParsePlayerRespawnMessage(const std::vector<std::string>& parts);
    static ParsedMessage ParsePlayerDamageMessage(const std::vector<std::string>& parts);
    static ParsedMessage ParseKillMessage(const std::vector<std::string>& parts);
    static ParsedMessage ParseForceFieldZapMessage(const std::vector<std::string>& parts);
    static ParsedMessage ParseForceFieldUpdateMessage(const std::vector<std::string>& parts);
    
    // Message formatting functions
    static std::string FormatConnectionMessage(const std::string& steamID, 
                                             const std::string& steamName, 
                                             const sf::Color& color, 
                                             bool isReady, 
                                             bool isHost);
    static std::string FormatMovementMessage(const std::string& steamID, 
                                           const sf::Vector2f& position);
    static std::string FormatBulletMessage(const std::string& shooterID, 
                                         const sf::Vector2f& position, 
                                         const sf::Vector2f& direction, 
                                         float velocity);
    static std::string FormatPlayerDeathMessage(const std::string& playerID, 
                                              const std::string& killerID);
    static std::string FormatPlayerRespawnMessage(const std::string& playerID, 
                                                const sf::Vector2f& position);
    static std::string FormatPlayerDamageMessage(const std::string& playerID, 
                                               int damage, 
                                               int enemyId);
    static std::string FormatKillMessage(const std::string& killerID, int enemyId);
    static std::string FormatForceFieldZapMessage(const std::string& playerID, int enemyId, float damage);
    static std::string FormatForceFieldUpdateMessage(
        const std::string& playerID,
        float radius,
        float damage,
        float cooldown,
        int chainTargets,
        int fieldType,
        int powerLevel,
        bool chainEnabled);
};

#endif // PLAYER_MESSAGE_HANDLER_H