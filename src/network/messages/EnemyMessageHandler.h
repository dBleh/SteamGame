#ifndef ENEMY_MESSAGE_HANDLER_H
#define ENEMY_MESSAGE_HANDLER_H

#include <string>
#include <vector>
#include <unordered_set>
#include <SFML/Graphics.hpp>
#include "../../entities/enemies/EnemyTypes.h"

// Forward declarations
struct ParsedMessage;

class EnemyMessageHandler {
public:
    // Initialize and register enemy message handlers
    static void Initialize();
    
    // Message parsing functions
    static ParsedMessage ParseEnemyAddMessage(const std::vector<std::string>& parts);
    static ParsedMessage ParseEnemyRemoveMessage(const std::vector<std::string>& parts);
    static ParsedMessage ParseEnemyDamageMessage(const std::vector<std::string>& parts);
    static ParsedMessage ParseEnemyPositionUpdateMessage(const std::vector<std::string>& parts);
    static ParsedMessage ParseEnemyStateMessage(const std::vector<std::string>& parts);
    static ParsedMessage ParseEnemyStateRequestMessage(const std::vector<std::string>& parts);
    static ParsedMessage ParseEnemyClearMessage(const std::vector<std::string>& parts);
    
    // Message formatting functions
    static std::string FormatEnemyAddMessage(int enemyId, EnemyType type, const sf::Vector2f& position, float health);
    static std::string FormatEnemyRemoveMessage(int enemyId);
    static std::string FormatEnemyDamageMessage(int enemyId, float damage, float remainingHealth);
    static std::string FormatEnemyPositionUpdateMessage(const std::vector<int>& enemyIds, const std::vector<sf::Vector2f>& positions, const std::vector<sf::Vector2f>& velocities = {});
    static std::string FormatEnemyStateMessage(const std::vector<int>& enemyIds, const std::vector<EnemyType>& types, 
                                            const std::vector<sf::Vector2f>& positions, const std::vector<float>& healths);
    static std::string FormatEnemyClearMessage();
};

#endif // ENEMY_MESSAGE_HANDLER_H