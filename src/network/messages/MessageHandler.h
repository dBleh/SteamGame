#ifndef MESSAGE_HANDLER_H
#define MESSAGE_HANDLER_H

#include <string>
#include <unordered_map>
#include <SFML/Graphics.hpp>
#include <vector>
#include <functional>
#include <steam/steam_api.h>
#include "../utils/SteamHelpers.h"
#include "../../entities/enemies/EnemyTypes.h"



class Game;
class ClientNetwork;
class HostNetwork;

enum class MessageType {
    Connection,
    Movement,
    Chat,
    ReadyStatus,
    Bullet,
    PlayerDeath,
    PlayerRespawn,
    StartGame,
    PlayerDamage,
    Unknown,
    EnemyAdd,
    EnemyRemove,
    EnemyUpdate,
    EnemyDamage,
    EnemyPositionUpdate,
    EnemyState,
    EnemyStateRequest, 
    WaveStart,
    EnemyClear,
    ChunkStart, 
    ChunkPart,   
    ChunkEnd,
    Kill,
    ForceFieldZap,
    ForceFieldUpdate  // New message type for force field updates
};

struct ParsedMessage {
    MessageType type = MessageType::Unknown;
    std::string steamID;
    std::string steamName;
    std::string killerID;
    sf::Vector2f position;
    sf::Color color;
    std::string chatMessage;
    bool isReady;
    bool isHost;
    sf::Vector2f direction;
    float velocity;
    int damage;
    int enemyId;
    EnemyType enemyType;
    int waveNumber;
    int enemyCount;
    std::vector<int> enemyIds;
    std::vector<sf::Vector2f> enemyPositions;
    std::vector<sf::Vector2f> enemyVelocities;
    std::vector<float> enemyHealths;
    int health;
    std::vector<int> enemyTypes;
    
    // Force field update parameters
    float ffRadius;
    float ffDamage;
    float ffCooldown;
    int ffChainTargets;
    int ffType;
    int ffPowerLevel;
    bool ffChainEnabled;
};
class MessageHandler {
public:
    // Message parser function type
    using MessageParserFunc = ParsedMessage (*)(const std::vector<std::string>&);
    
    // Message descriptor structure (consolidated from MessageRegistry)
    struct MessageDescriptor {
        std::string prefix;
        void (*clientHandler)(Game&, ClientNetwork&, const ParsedMessage&);
        void (*hostHandler)(Game&, HostNetwork&, const ParsedMessage&, CSteamID);
    };
    
    // Static maps for parsers and descriptors
    static std::unordered_map<std::string, MessageParserFunc> messageParsers;
    static std::unordered_map<std::string, MessageDescriptor> messageDescriptors;
    
    // Initialize and register all message handlers
    static void Initialize();
    
    // Message parsing and handling
    static ParsedMessage ParseMessage(const std::string& msg);
    static const MessageDescriptor* GetDescriptorByType(MessageType type);
    
    // Message registration
    static void RegisterMessageType(
        const std::string& prefix,
        MessageParserFunc parser,
        void (*clientHandler)(Game&, ClientNetwork&, const ParsedMessage&),
        void (*hostHandler)(Game&, HostNetwork&, const ParsedMessage&, CSteamID)
    );
    
    // Get prefix for a message type
    static std::string GetPrefixForType(MessageType type);
    
    // Message parsing functions (one for each type)
    static ParsedMessage ParseConnectionMessage(const std::vector<std::string>& parts);
    static ParsedMessage ParseMovementMessage(const std::vector<std::string>& parts);
    static ParsedMessage ParseChatMessage(const std::vector<std::string>& parts);
    static ParsedMessage ParseReadyStatusMessage(const std::vector<std::string>& parts);
    static ParsedMessage ParseBulletMessage(const std::vector<std::string>& parts);
    static ParsedMessage ParsePlayerDeathMessage(const std::vector<std::string>& parts);
    static ParsedMessage ParsePlayerRespawnMessage(const std::vector<std::string>& parts);
    static ParsedMessage ParseStartGameMessage(const std::vector<std::string>& parts);
    static ParsedMessage ParsePlayerDamageMessage(const std::vector<std::string>& parts);
    static ParsedMessage ParseKillMessage(const std::vector<std::string>& parts);
    static std::string FormatKillMessage(const std::string& killerID, int enemyId);
    // Enemy message parsing functions
    static ParsedMessage ParseEnemyAddMessage(const std::vector<std::string>& parts);
    static ParsedMessage ParseEnemyRemoveMessage(const std::vector<std::string>& parts);
    static ParsedMessage ParseEnemyDamageMessage(const std::vector<std::string>& parts);
    static ParsedMessage ParseEnemyPositionUpdateMessage(const std::vector<std::string>& parts);
    static ParsedMessage ParseEnemyStateMessage(const std::vector<std::string>& parts);
    static ParsedMessage ParseWaveStartMessage(const std::vector<std::string>& parts);
    static ParsedMessage ParseEnemyClearMessage(const std::vector<std::string>& parts);
    static ParsedMessage ParseEnemyStateRequestMessage(const std::vector<std::string>& parts);
    static ParsedMessage ParseForceFieldZapMessage(const std::vector<std::string>& parts);
    void ProcessUnknownMessage(Game& game, ClientNetwork& client, const ParsedMessage& parsed);
    static std::string FormatForceFieldZapMessage(const std::string& playerID, int enemyId, float damage);
    static ParsedMessage ParseChunkStartMessage(const std::vector<std::string>& parts);
    static ParsedMessage ParseChunkPartMessage(const std::vector<std::string>& parts);
    static ParsedMessage ParseChunkEndMessage(const std::vector<std::string>& parts);
    // Message formatting functions (one for each type)
    static std::string FormatConnectionMessage(const std::string& steamID, 
                                             const std::string& steamName, 
                                             const sf::Color& color, 
                                             bool isReady, 
                                             bool isHost);
    static std::string FormatMovementMessage(const std::string& steamID, 
                                           const sf::Vector2f& position);
    static std::string FormatChatMessage(const std::string& steamID, 
                                       const std::string& message);
    static std::string FormatReadyStatusMessage(const std::string& steamID, 
                                              bool isReady);
    static std::string FormatBulletMessage(const std::string& shooterID, 
                                         const sf::Vector2f& position, 
                                         const sf::Vector2f& direction, 
                                         float velocity);
    static std::string FormatPlayerDeathMessage(const std::string& playerID, 
                                              const std::string& killerID);
    static std::string FormatPlayerRespawnMessage(const std::string& playerID, 
                                                const sf::Vector2f& position);
    static std::string FormatStartGameMessage(const std::string& hostID);
    static std::string FormatPlayerDamageMessage(const std::string& playerID, 
                                               int damage, 
                                               int enemyId);
    static ParsedMessage ParseForceFieldUpdateMessage(const std::vector<std::string>& parts);
    static std::string FormatForceFieldUpdateMessage(
        const std::string& playerID,
        float radius,
        float damage,
        float cooldown,
        int chainTargets,
        int fieldType,
        int powerLevel,
        bool chainEnabled);
    // Enemy message formatting functions
    static std::string FormatEnemyAddMessage(int enemyId, EnemyType type, const sf::Vector2f& position, float health);
    static std::string FormatEnemyRemoveMessage(int enemyId);
    static std::string FormatEnemyDamageMessage(int enemyId, float damage, float remainingHealth);
    static std::string FormatEnemyPositionUpdateMessage(const std::vector<int>& enemyIds, const std::vector<sf::Vector2f>& positions);
    static std::string FormatEnemyStateMessage(const std::vector<int>& enemyIds, const std::vector<EnemyType>& types, 
                                            const std::vector<sf::Vector2f>& positions, const std::vector<float>& healths);
    static std::string FormatWaveStartMessage(int waveNumber, int enemyCount);
    static std::string FormatEnemyClearMessage();

    // Chunking methods
    static std::vector<std::string> ChunkMessage(const std::string& message, const std::string& messageType);
    static std::string FormatChunkStartMessage(const std::string& messageType, int totalChunks, const std::string& chunkId);
    static std::string FormatChunkPartMessage(const std::string& chunkId, int chunkNum, const std::string& chunkData);
    static std::string FormatChunkEndMessage(const std::string& chunkId);
    
    static void AddChunk(const std::string& chunkId, int chunkNum, const std::string& chunkData);
    static bool IsChunkComplete(const std::string& chunkId, int expectedChunks);
    static std::string GetReconstructedMessage(const std::string& chunkId);
    static void ClearChunks(const std::string& chunkId);

    // Chunk storage
    static std::unordered_map<std::string, std::vector<std::string>> chunkStorage;
    static std::unordered_map<std::string, std::string> chunkTypes;
    static std::unordered_map<std::string, int> chunkCounts;

private:
    static std::vector<std::string> SplitString(const std::string& str, char delimiter);
};

#endif // MESSAGE_HANDLER_H