#ifndef MESSAGE_HANDLER_H
#define MESSAGE_HANDLER_H

#include <string>
#include <SFML/Graphics.hpp>

enum class MessageType {
    Connection,
    Movement,
    Chat,
    ReadyStatus,
    Bullet,
    PlayerDeath,
    PlayerRespawn,
    StartGame,
    EnemySpawn,
    EnemyHit,
    EnemyDeath,
    PlayerDamage,
    WaveStart,
    WaveComplete,
    EnemyPositions,
    EnemyValidation,
    EnemyValidationRequest 
};

struct ParsedMessage {
    MessageType type;
    std::string steamID;
    std::string steamName;
    std::string killerID;     // For PlayerDeath
    sf::Vector2f position;
    sf::Color color;
    std::string chatMessage;
    bool isReady;
    bool isHost;
    sf::Vector2f direction;  // For Bullet
    float velocity;          // For Bullet
    std::vector<int> validEnemyIds;
    // Enemy-related fields
    int enemyId;             // ID of the enemy
    int damage;              // Damage amount
    bool killed;             // Was enemy killed
    int waveNumber;          // Wave number
    bool rewardKill;         // Should killer be rewarded
    std::vector<std::pair<int, sf::Vector2f>> enemyPositions; // For enemy position updates (id, position)
    
};

class MessageHandler {
public:
    static std::string FormatConnectionMessage(const std::string& steamID, const std::string& steamName, const sf::Color& color, bool isReady, bool isHost);
    static std::string FormatMovementMessage(const std::string& steamID, const sf::Vector2f& position);
    static std::string FormatChatMessage(const std::string& steamID, const std::string& message);
    static std::string FormatReadyStatusMessage(const std::string& steamID, bool isReady);
    static std::string FormatBulletMessage(const std::string& shooterID, const sf::Vector2f& position, const sf::Vector2f& direction, float velocity);
    static std::string FormatPlayerDeathMessage(const std::string& playerID, const std::string& killerID);
    static std::string FormatPlayerRespawnMessage(const std::string& playerID, const sf::Vector2f& position);
    static std::string FormatStartGameMessage(const std::string& hostID);
    static std::string FormatEnemyPositionsMessage(const std::vector<std::pair<int, sf::Vector2f>>& enemyPositions);
    // Enemy-related messages
    static std::string FormatEnemySpawnMessage(int enemyId, const sf::Vector2f& position);
    static std::string FormatEnemyHitMessage(int enemyId, int damage, bool killed, const std::string& shooterID);
    static std::string FormatEnemyDeathMessage(int enemyId, const std::string& killerID, bool rewardKill);
    static std::string FormatPlayerDamageMessage(const std::string& playerID, int damage, int enemyId);
    static std::string FormatWaveStartMessage(int waveNumber);
    static std::string FormatWaveCompleteMessage(int waveNumber);
    static std::string FormatEnemyValidationMessage(const std::vector<int>& enemyIds);
    static std::string FormatEnemyValidationRequestMessage();

    static ParsedMessage ParseMessage(const std::string& msg);
};

#endif // MESSAGE_HANDLER_H