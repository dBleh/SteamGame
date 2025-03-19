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

// Forward declarations
struct ParsedMessage;
enum class MessageType;
class PlayingState;
class GameSettingsManager;
class Game;
class ClientNetwork;
class HostNetwork;
class CSteamID;

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
    ForceFieldUpdate,
    SettingsUpdate,
    SettingsRequest,
    
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
    uint32_t killSequence;
    
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
    
    // Message descriptor structure
    struct MessageDescriptor {
        std::string prefix;
        void (*clientHandler)(Game&, ClientNetwork&, const ParsedMessage&);
        void (*hostHandler)(Game&, HostNetwork&, const ParsedMessage&, CSteamID);
    };
    
    // Static maps for parsers and descriptors
    static std::unordered_map<std::string, MessageParserFunc> messageParsers;
    static std::unordered_map<std::string, MessageDescriptor> messageDescriptors;
    
    // Chunk storage
    static std::unordered_map<std::string, std::vector<std::string>> chunkStorage;
    static std::unordered_map<std::string, std::string> chunkTypes;
    static std::unordered_map<std::string, int> chunkCounts;
    
    // Initialize and register all message handlers
    static void Initialize();
    
    // Message parsing and handling
    static ParsedMessage ParseMessage(const std::string& msg);
    static const MessageDescriptor* GetDescriptorByType(MessageType type);
    static std::string GetPrefixForType(MessageType type);
    static void ProcessUnknownMessage(Game& game, ClientNetwork& client, const ParsedMessage& parsed);
    
    // Message registration
    static void RegisterMessageType(
        const std::string& prefix,
        MessageParserFunc parser,
        void (*clientHandler)(Game&, ClientNetwork&, const ParsedMessage&),
        void (*hostHandler)(Game&, HostNetwork&, const ParsedMessage&, CSteamID)
    );
    
    // Utility functions
    static std::vector<std::string> SplitString(const std::string& str, char delimiter);
};



#endif // MESSAGE_HANDLER_H