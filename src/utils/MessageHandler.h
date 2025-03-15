#ifndef MESSAGE_HANDLER_H
#define MESSAGE_HANDLER_H

#include <string>
#include <SFML/Graphics.hpp>
#include "../utils/SteamHelpers.h"
class Game;
class EnemyManager;
class PlayingState;
 
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
    EnemyValidationRequest,
    EnemyBatchSpawn,
    TriangleWaveStart,
    ChunkStart,
    ChunkPart,
    ChunkEnd
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
    
    // Enemy position data
    std::vector<std::pair<int, sf::Vector2f>> enemyPositions; // For enemy position updates (id, position)
    std::vector<std::pair<int, int>> enemyHealths;            // For enemy health updates (id, health)
    
    // Enemy types enum
    enum class EnemyType {
        Regular = 0,
        Triangle = 1
    };
    
    EnemyType enemyType = EnemyType::Regular;  // Default to regular enemy
    
    // Wave data
    uint32_t seed;
    int enemyCount;
    
    // Chunking data
    std::string chunkId;
    int chunkNum;
    int totalChunks;
    std::string chunkType;
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
    
    // Enemy-related message formatting
    static std::string FormatEnemySpawnMessage(int enemyId, const sf::Vector2f& position, ParsedMessage::EnemyType enemyType = ParsedMessage::EnemyType::Regular);
    static std::string FormatEnemyBatchSpawnMessage(const std::vector<std::tuple<int, sf::Vector2f, int>>& batchData, ParsedMessage::EnemyType enemyType = ParsedMessage::EnemyType::Regular);
    static std::string FormatEnemyHitMessage(int enemyId, int damage, bool killed, const std::string& shooterID, ParsedMessage::EnemyType enemyType = ParsedMessage::EnemyType::Regular);
    static std::string FormatEnemyDeathMessage(int enemyId, const std::string& killerID, bool rewardKill, ParsedMessage::EnemyType enemyType = ParsedMessage::EnemyType::Regular);
    static std::string FormatEnemyFullListMessage(const std::vector<int>& enemyIds, ParsedMessage::EnemyType enemyType = ParsedMessage::EnemyType::Regular);

    
    // Unified enemy positions message for all enemy types
    static std::string FormatEnemyPositionsMessage(const std::vector<std::tuple<int, sf::Vector2f, int>>& enemyData);
    
    // Wave management messages
    static std::string FormatTriangleWaveStartMessage(uint32_t seed, int enemyCount);
    static std::string FormatWaveStartMessage(int waveNumber);
    static std::string FormatWaveCompleteMessage(int waveNumber);
    static std::string FormatWaveStartWithTypesMessage(int waveNumber, uint32_t seed, const std::vector<int>& typeInts);
    
    // Other enemy-related messages
    static std::string FormatPlayerDamageMessage(const std::string& playerID, int damage, int enemyId);
    static std::string FormatEnemyValidationMessage(const std::vector<int>& enemyIds);
    static std::string FormatEnemyFullListMessage(const std::vector<int>& enemyIds, int enemyType = 0);
    static std::string FormatEnemyValidationRequestMessage();
    
    // Message parsing methods
    static ParsedMessage ParseTriangleWaveStartMessage(const std::string& message);

    // Chunking methods for large messages
    static std::vector<std::string> ChunkMessage(const std::string& message, const std::string& messageType);
    static std::string FormatChunkStartMessage(const std::string& messageType, int totalChunks, const std::string& chunkId);
    static std::string FormatChunkPartMessage(const std::string& chunkId, int chunkNum, const std::string& chunkData);
    static std::string FormatChunkEndMessage(const std::string& chunkId);
    
    // Message reconstruction
    static void AddChunk(const std::string& chunkId, int chunkNum, const std::string& chunkData);
    static bool IsChunkComplete(const std::string& chunkId, int expectedChunks);
    static std::string GetReconstructedMessage(const std::string& chunkId);
    static void ClearChunks(const std::string& chunkId);

    // Chunk storage
    static std::unordered_map<std::string, std::vector<std::string>> chunkStorage;
    static std::unordered_map<std::string, std::string> chunkTypes;
    static std::unordered_map<std::string, int> chunkCounts;

    // Main message parser
    static ParsedMessage ParseMessage(const std::string& msg);
    
    // Maximum packet size constant
};

#endif // MESSAGE_HANDLER_H