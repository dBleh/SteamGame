#include "MessageHandler.h"
#include "PlayerMessageHandler.h"
#include "EnemyMessageHandler.h"
#include "StateMessageHandler.h"
#include "SystemMessageHandler.h"
#include <iostream>
#include <sstream>
#include <chrono>
#include <cstdlib>
#include "../Client.h"
#include "../Host.h"
#include "../../core/Game.h"
#include "../../states/PlayingState.h"
#include <sstream>
#include <iostream>
#include <chrono>
#include <cstdlib>

// Initialize static members
std::unordered_map<std::string, MessageHandler::MessageParserFunc> MessageHandler::messageParsers;
std::unordered_map<std::string, MessageHandler::MessageDescriptor> MessageHandler::messageDescriptors;
std::unordered_map<std::string, std::vector<std::string>> MessageHandler::chunkStorage;
std::unordered_map<std::string, std::string> MessageHandler::chunkTypes;
std::unordered_map<std::string, int> MessageHandler::chunkCounts;

void MessageHandler::Initialize() {
    // Clear existing handlers
    messageParsers.clear();
    messageDescriptors.clear();
    
    // Initialize all message handlers
    SystemMessageHandler::Initialize();
    PlayerMessageHandler::Initialize();
    EnemyMessageHandler::Initialize();
    StateMessageHandler::Initialize();
}

void MessageHandler::RegisterMessageType(
    const std::string& prefix,
    MessageParserFunc parser,
    void (*clientHandler)(Game&, ClientNetwork&, const ParsedMessage&),
    void (*hostHandler)(Game&, HostNetwork&, const ParsedMessage&, CSteamID)
) {
    messageParsers[prefix] = parser;
    messageDescriptors[prefix] = {prefix, clientHandler, hostHandler};
}

std::string MessageHandler::GetPrefixForType(MessageType type) {
    switch (type) {
        // Player-related messages
        case MessageType::Connection: return "C";
        case MessageType::Movement: return "M";
        case MessageType::Bullet: return "B";
        case MessageType::PlayerDeath: return "D";
        case MessageType::PlayerRespawn: return "RS";
        case MessageType::PlayerDamage: return "PD";
        case MessageType::Kill: return "KL";
        case MessageType::ForceFieldZap: return "FZ";
        case MessageType::ForceFieldUpdate: return "FFU";
        
        // Enemy-related messages
        case MessageType::EnemyAdd: return "EA";
        case MessageType::EnemyRemove: return "ER";
        case MessageType::EnemyDamage: return "ED";
        case MessageType::EnemyPositionUpdate: return "EP";
        case MessageType::EnemyState: return "ES";
        case MessageType::EnemyStateRequest: return "ESR";
        case MessageType::EnemyClear: return "EC";
        
        // State-related messages
        case MessageType::ReadyStatus: return "R";
        case MessageType::StartGame: return "SG";
        case MessageType::WaveStart: return "WS";
        
        // System messages
        case MessageType::Chat: return "T";
        case MessageType::ChunkStart: return "CHUNK_START";
        case MessageType::ChunkPart: return "CHUNK_PART";
        case MessageType::ChunkEnd: return "CHUNK_END";
        
        case MessageType::Unknown: 
        default: return "";
    }
}

const MessageHandler::MessageDescriptor* MessageHandler::GetDescriptorByType(MessageType type) {
    std::string prefix = GetPrefixForType(type);
    if (prefix.empty()) return nullptr;
    
    auto it = messageDescriptors.find(prefix);
    if (it != messageDescriptors.end()) {
        return &(it->second);
    }
    return nullptr;
}

// Message parsing
ParsedMessage MessageHandler::ParseMessage(const std::string& msg) {
    std::vector<std::string> parts = SplitString(msg, '|');
    if (parts.empty()) return ParsedMessage{MessageType::Unknown};
    
    std::string prefix = parts[0];
    
    // Special handling for chunk messages
    if (prefix == "CHUNK_START" || prefix == "CHUNK_PART" || prefix == "CHUNK_END") {
        auto parserIt = messageParsers.find(prefix);
        if (parserIt != messageParsers.end()) {
            return parserIt->second(parts);
        }
    }
    
    // Regular message handling
    auto parserIt = messageParsers.find(prefix);
    if (parserIt != messageParsers.end()) {
        return parserIt->second(parts);
    }
    
    std::cout << "[MessageHandler] Unknown message type: " << prefix << "\n";
    return ParsedMessage{MessageType::Unknown};
}

void MessageHandler::ProcessUnknownMessage(Game& game, ClientNetwork& client, const ParsedMessage& parsed) {
    // Check if it might be a chunked message that was incorrectly parsed
    if (!parsed.steamID.empty()) {
        std::cout << "[MessageHandler] Attempting to recover from unknown message, ID: " << parsed.steamID << "\n";
        
        // Try to interpret it as an enemy state update if it contains numbers and commas
        if (parsed.steamID.find_first_of("0123456789,.") != std::string::npos) {
            std::vector<std::string> parts;
            parts.push_back("ES"); // Enemy State prefix
            parts.push_back(parsed.steamID);
            
            // Try to parse as enemy state
            ParsedMessage recoveredMessage = EnemyMessageHandler::ParseEnemyStateMessage(parts);
            
            // If we recovered at least one enemy, process it
            if (!recoveredMessage.enemyIds.empty()) {
                std::cout << "[MessageHandler] Recovered " << recoveredMessage.enemyIds.size() << " enemy positions\n";
                PlayingState* state = GetPlayingState(&game);
                if (state && state->GetEnemyManager()) {
                    for (size_t i = 0; i < recoveredMessage.enemyIds.size(); ++i) {
                        int id = recoveredMessage.enemyIds[i];
                        auto enemyManager = state->GetEnemyManager();
                        auto enemy = enemyManager->FindEnemy(id);
                        
                        if (enemy) {
                            // Update existing enemy
                            enemy->SetPosition(recoveredMessage.enemyPositions[i]);
                        } else {
                            // Create new enemy
                            EnemyType type = static_cast<EnemyType>(recoveredMessage.enemyTypes[i]);
                            enemyManager->RemoteAddEnemy(id, type, recoveredMessage.enemyPositions[i], 
                                                       recoveredMessage.enemyHealths[i]);
                        }
                    }
                }
            }
        }
    }
}

// Utility function to split strings - needed by all message handlers
std::vector<std::string> MessageHandler::SplitString(const std::string& str, char delimiter) {
    std::vector<std::string> parts;
    std::istringstream iss(str);
    std::string part;
    while (std::getline(iss, part, delimiter)) {
        parts.push_back(part);
    }
    return parts;
}