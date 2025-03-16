#include "MessageHandler.h"
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
    
    // Register chunking handlers
    messageParsers["CHUNK_START"] = [](const std::vector<std::string>& parts) {
        if (parts.size() >= 4) {
            std::string messageType = parts[1];
            int totalChunks = std::stoi(parts[2]);
            std::string chunkId = parts[3];
            chunkTypes[chunkId] = messageType;
            chunkCounts[chunkId] = totalChunks;
            chunkStorage[chunkId].resize(totalChunks);
        }
        return ParsedMessage{MessageType::Unknown};
    };
    
    messageParsers["CHUNK_PART"] = [](const std::vector<std::string>& parts) {
        if (parts.size() >= 4) {
            std::string chunkId = parts[1];
            int chunkNum = std::stoi(parts[2]);
            std::string chunkData = parts[3];
            AddChunk(chunkId, chunkNum, chunkData);
        }
        return ParsedMessage{MessageType::Unknown};
    };
    
    messageParsers["CHUNK_END"] = [](const std::vector<std::string>& parts) {
        if (parts.size() >= 2) {
            std::string chunkId = parts[1];
            if (chunkStorage.find(chunkId) != chunkStorage.end() && 
                chunkCounts.find(chunkId) != chunkCounts.end()) {
                int expectedChunks = chunkCounts[chunkId];
                if (IsChunkComplete(chunkId, expectedChunks)) {
                    std::string fullMessage = GetReconstructedMessage(chunkId);
                    std::string messageType = chunkTypes[chunkId];
                    ClearChunks(chunkId);
                    return ParseMessage(fullMessage);
                }
            }
        }
        return ParsedMessage{MessageType::Unknown};
    };
    
    // Register message types with their parsers and handlers
    RegisterMessageType("C", 
                        ParseConnectionMessage,
                        [](Game& game, ClientNetwork& client, const ParsedMessage& parsed) {
                            client.ProcessConnectionMessage(game, client, parsed);
                        },
                        [](Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender) {
                            host.ProcessConnectionMessage(game, host, parsed, sender);
                        });
    
    RegisterMessageType("M", 
                        ParseMovementMessage,
                        [](Game& game, ClientNetwork& client, const ParsedMessage& parsed) {
                            client.ProcessMovementMessage(game, client, parsed);
                        },
                        [](Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender) {
                            host.ProcessMovementMessage(game, host, parsed, sender);
                        });
    
    RegisterMessageType("T", 
                        ParseChatMessage,
                        [](Game& game, ClientNetwork& client, const ParsedMessage& parsed) {
                            client.ProcessChatMessage(game, client, parsed);
                        },
                        [](Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender) {
                            host.ProcessChatMessageParsed(game, host, parsed, sender);
                        });

    RegisterMessageType("R", 
                        ParseReadyStatusMessage,
                        [](Game& game, ClientNetwork& client, const ParsedMessage& parsed) {
                            client.ProcessReadyStatusMessage(game, client, parsed);
                        },
                        [](Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender) {
                            host.ProcessReadyStatusMessage(game, host, parsed, sender);
                        });

    RegisterMessageType("B", 
                        ParseBulletMessage,
                        [](Game& game, ClientNetwork& client, const ParsedMessage& parsed) {
                            client.ProcessBulletMessage(game, client, parsed);
                        },
                        [](Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender) {
                            host.ProcessBulletMessage(game, host, parsed, sender);
                        });

    RegisterMessageType("D", 
                        ParsePlayerDeathMessage,
                        [](Game& game, ClientNetwork& client, const ParsedMessage& parsed) {
                            client.ProcessPlayerDeathMessage(game, client, parsed);
                        },
                        [](Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender) {
                            host.ProcessPlayerDeathMessage(game, host, parsed, sender);
                        });

    RegisterMessageType("RS", 
                        ParsePlayerRespawnMessage,
                        [](Game& game, ClientNetwork& client, const ParsedMessage& parsed) {
                            client.ProcessPlayerRespawnMessage(game, client, parsed);
                        },
                        [](Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender) {
                            host.ProcessPlayerRespawnMessage(game, host, parsed, sender);
                        });

    RegisterMessageType("SG", 
                        ParseStartGameMessage,
                        [](Game& game, ClientNetwork& client, const ParsedMessage& parsed) {
                            client.ProcessStartGameMessage(game, client, parsed);
                        },
                        [](Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender) {
                            host.ProcessStartGameMessage(game, host, parsed, sender);
                        });

    RegisterMessageType("PD", 
                        ParsePlayerDamageMessage,
                        [](Game& game, ClientNetwork& client, const ParsedMessage& parsed) {
                            client.ProcessPlayerDamageMessage(game, client, parsed);
                        },
                        [](Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender) {
                            host.ProcessPlayerDamageMessage(game, host, parsed, sender);
                        });
                        RegisterMessageType("EA", 
                            ParseEnemyAddMessage,
                            [](Game& game, ClientNetwork& client, const ParsedMessage& parsed) {
                                PlayingState* state = GetPlayingState(&game);
                                if (state && state->GetEnemyManager()) {
                                    state->GetEnemyManager()->RemoteAddEnemy(parsed.enemyId, parsed.enemyType, parsed.position, parsed.health);
                                }
                            },
                            [](Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender) {
                                // Host should be the one creating enemies, not receiving
                                std::cout << "[HOST] Received enemy add message from client, ignoring\n";
                            });
        
        RegisterMessageType("ER", 
                            ParseEnemyRemoveMessage,
                            [](Game& game, ClientNetwork& client, const ParsedMessage& parsed) {
                                PlayingState* state = GetPlayingState(&game);
                                if (state && state->GetEnemyManager()) {
                                    state->GetEnemyManager()->RemoteRemoveEnemy(parsed.enemyId);
                                }
                            },
                            [](Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender) {
                                // Client notifying host of enemy removal
                                PlayingState* state = GetPlayingState(&game);
                                if (state && state->GetEnemyManager()) {
                                    state->GetEnemyManager()->RemoveEnemy(parsed.enemyId);
                                }
                            });
        
                            RegisterMessageType("ED", 
                                ParseEnemyDamageMessage,
                                [](Game& game, ClientNetwork& client, const ParsedMessage& parsed) {
                                    PlayingState* state = GetPlayingState(&game);
                                    if (state && state->GetEnemyManager()) {
                                        EnemyManager* enemyManager = state->GetEnemyManager();
                                        // Access enemies using the proper interface methods, not directly
                                        // Use SetHealth method which is part of public API instead of direct access
                                        if (parsed.enemyId >= 0) {
                                            // Assuming there's a method to find an enemy by ID and update it
                                            // If not, you would need to add one to EnemyManager
                                            auto enemy = enemyManager->FindEnemy(parsed.enemyId);
                                            if (enemy) {
                                                enemy->SetHealth(parsed.health);
                                                if (parsed.health <= 0) {
                                                    enemyManager->RemoteRemoveEnemy(parsed.enemyId);
                                                }
                                            }
                                        }
                                    }
                                },
                                [](Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender) {
                                    // Client informing host of damage to enemy
                                    PlayingState* state = GetPlayingState(&game);
                                    if (state && state->GetEnemyManager()) {
                                        state->GetEnemyManager()->InflictDamage(parsed.enemyId, parsed.damage);
                                    }
                                });
                            
                            // For EP (EnemyPositionUpdate) message
                            RegisterMessageType("EP", 
                                ParseEnemyPositionUpdateMessage,
                                [](Game& game, ClientNetwork& client, const ParsedMessage& parsed) {
                                    PlayingState* state = GetPlayingState(&game);
                                    if (state && state->GetEnemyManager()) {
                                        EnemyManager* enemyManager = state->GetEnemyManager();
                                        
                                        // Update each enemy position
                                        for (size_t i = 0; i < parsed.enemyIds.size(); ++i) {
                                            int id = parsed.enemyIds[i];
                                            // Use a method to find and update an enemy, rather than direct access
                                            auto enemy = enemyManager->FindEnemy(id);
                                            if (enemy) {
                                                enemy->SetPosition(parsed.enemyPositions[i]);
                                            }
                                        }
                                    }
                                },
                                [](Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender) {
                                    // Host should be the one updating positions, not receiving
                                    std::cout << "[HOST] Received position update from client, ignoring\n";
                                });
                            
                            // For ES (EnemyState) message
        RegisterMessageType("ES", 
                                ParseEnemyStateMessage,
                                [](Game& game, ClientNetwork& client, const ParsedMessage& parsed) {
                                    PlayingState* state = GetPlayingState(&game);
                                    if (state && state->GetEnemyManager()) {
                                        EnemyManager* enemyManager = state->GetEnemyManager();
                                        
                                        // Process all enemy states
                                        for (size_t i = 0; i < parsed.enemyIds.size(); ++i) {
                                            int id = parsed.enemyIds[i];
                                            auto enemy = enemyManager->FindEnemy(id);
                                            
                                            if (enemy) {
                                                // Update existing enemy
                                                enemy->SetPosition(parsed.enemyPositions[i]);
                                                enemy->SetHealth(parsed.enemyHealths[i]);
                                            } else {
                                                // Create new enemy
                                                EnemyType type = static_cast<EnemyType>(parsed.enemyTypes[i]);
                                                enemyManager->RemoteAddEnemy(id, type, parsed.enemyPositions[i], parsed.enemyHealths[i]);
                                            }
                                        }
                                        
                                        // Mark enemies not in the update for removal
                                        enemyManager->RemoveEnemiesNotInList(parsed.enemyIds);
                                    }
                                },
                                [](Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender) {
                                    // Host sends full state, not receives it
                                    std::cout << "[HOST] Received state update from client, ignoring\n";
                                });
        
        RegisterMessageType("WS", 
                            ParseWaveStartMessage,
                            [](Game& game, ClientNetwork& client, const ParsedMessage& parsed) {
                                PlayingState* state = GetPlayingState(&game);
                                if (state && state->GetEnemyManager()) {
                                    state->GetEnemyManager()->SetCurrentWave(parsed.waveNumber);
                                    // Note: clients don't spawn enemies; they wait for EA messages from host
                                }
                            },
                            [](Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender) {
                                // Host initiates waves, not receives them
                                std::cout << "[HOST] Received wave start from client, ignoring\n";
                            });
        
        RegisterMessageType("EC", 
                            ParseEnemyClearMessage,
                            [](Game& game, ClientNetwork& client, const ParsedMessage& parsed) {
                                PlayingState* state = GetPlayingState(&game);
                                if (state && state->GetEnemyManager()) {
                                    state->GetEnemyManager()->ClearEnemies();
                                }
                            },
                            [](Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender) {
                                // Host initiates clear, not receives it
                                std::cout << "[HOST] Received enemy clear from client, ignoring\n";
                            });
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
        case MessageType::Connection: return "C";
        case MessageType::Movement: return "M";
        case MessageType::Chat: return "T";
        case MessageType::ReadyStatus: return "R";
        case MessageType::Bullet: return "B";
        case MessageType::PlayerDeath: return "D";
        case MessageType::PlayerRespawn: return "RS";
        case MessageType::StartGame: return "SG";
        case MessageType::PlayerDamage: return "PD";
        case MessageType::EnemyAdd: return "EA";
        case MessageType::EnemyRemove: return "ER";
        case MessageType::EnemyDamage: return "ED";
        case MessageType::EnemyPositionUpdate: return "EP";
        case MessageType::EnemyState: return "ES";
        case MessageType::WaveStart: return "WS";
        case MessageType::EnemyClear: return "EC";
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

// Utility function to split strings
std::vector<std::string> MessageHandler::SplitString(const std::string& str, char delimiter) {
    std::vector<std::string> parts;
    std::istringstream iss(str);
    std::string part;
    while (std::getline(iss, part, delimiter)) {
        parts.push_back(part);
    }
    return parts;
}

// Message parsing
ParsedMessage MessageHandler::ParseMessage(const std::string& msg) {
    std::vector<std::string> parts = SplitString(msg, '|');
    if (parts.empty()) return ParsedMessage{MessageType::Unknown};
    std::string prefix = parts[0];
    auto parserIt = messageParsers.find(prefix);
    if (parserIt != messageParsers.end()) {
        return parserIt->second(parts);
    }
    std::cout << "[MessageHandler] Unknown message type: " << prefix << "\n";
    return ParsedMessage{MessageType::Unknown};
}

// Chunking functions
std::vector<std::string> MessageHandler::ChunkMessage(const std::string& message, const std::string& messageType) {
    std::vector<std::string> chunks;
    if (message.size() <= MAX_PACKET_SIZE) {
        chunks.push_back(message);
        return chunks;
    }
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    std::string chunkId = std::to_string(timestamp) + "_" + std::to_string(rand() % 10000);
    size_t chunkSize = MAX_PACKET_SIZE - 50;
    size_t numChunks = (message.size() + chunkSize - 1) / chunkSize;
    chunks.push_back(FormatChunkStartMessage(messageType, static_cast<int>(numChunks), chunkId));
    for (size_t i = 0; i < numChunks; i++) {
        size_t start = i * chunkSize;
        size_t end = std::min(start + chunkSize, message.size());
        std::string chunkData = message.substr(start, end - start);
        chunks.push_back(FormatChunkPartMessage(chunkId, static_cast<int>(i), chunkData));
    }
    chunks.push_back(FormatChunkEndMessage(chunkId));
    return chunks;
}

std::string MessageHandler::FormatChunkStartMessage(const std::string& messageType, int totalChunks, const std::string& chunkId) {
    std::ostringstream oss;
    oss << "CHUNK_START|" << messageType << "|" << totalChunks << "|" << chunkId;
    return oss.str();
}

std::string MessageHandler::FormatChunkPartMessage(const std::string& chunkId, int chunkNum, const std::string& chunkData) {
    std::ostringstream oss;
    oss << "CHUNK_PART|" << chunkId << "|" << chunkNum << "|" << chunkData;
    return oss.str();
}

std::string MessageHandler::FormatChunkEndMessage(const std::string& chunkId) {
    std::ostringstream oss;
    oss << "CHUNK_END|" << chunkId;
    return oss.str();
}

void MessageHandler::AddChunk(const std::string& chunkId, int chunkNum, const std::string& chunkData) {
    if (chunkStorage[chunkId].size() <= static_cast<size_t>(chunkNum)) {
        chunkStorage[chunkId].resize(chunkNum + 1);
    }
    chunkStorage[chunkId][chunkNum] = chunkData;
}

bool MessageHandler::IsChunkComplete(const std::string& chunkId, int expectedChunks) {
    if (chunkStorage.find(chunkId) == chunkStorage.end() ||
        chunkStorage[chunkId].size() != static_cast<size_t>(expectedChunks)) {
        return false;
    }
    for (const auto& chunk : chunkStorage[chunkId]) {
        if (chunk.empty()) return false;
    }
    return true;
}

std::string MessageHandler::GetReconstructedMessage(const std::string& chunkId) {
    if (chunkStorage.find(chunkId) == chunkStorage.end()) return "";
    std::string result;
    for (const auto& chunk : chunkStorage[chunkId]) {
        result += chunk;
    }
    return result;
}

void MessageHandler::ClearChunks(const std::string& chunkId) {
    chunkStorage.erase(chunkId);
    chunkTypes.erase(chunkId);
    chunkCounts.erase(chunkId);
}

// Message-specific parsing functions
ParsedMessage MessageHandler::ParseConnectionMessage(const std::vector<std::string>& parts) {
    ParsedMessage parsed;
    parsed.type = MessageType::Connection;
    if (parts.size() >= 6) {
        parsed.steamID = parts[1];
        parsed.steamName = parts[2];
        std::istringstream colorStream(parts[3]);
        int r, g, b;
        char comma;
        colorStream >> r >> comma >> g >> comma >> b;
        parsed.color = sf::Color(r, g, b);
        parsed.isReady = (parts[4] == "1");
        parsed.isHost = (parts[5] == "1");
    }
    return parsed;
}

ParsedMessage MessageHandler::ParseMovementMessage(const std::vector<std::string>& parts) {
    ParsedMessage parsed;
    parsed.type = MessageType::Movement;
    if (parts.size() >= 3) {
        parsed.steamID = parts[1];
        std::istringstream posStream(parts[2]);
        float x, y;
        char comma;
        posStream >> x >> comma >> y;
        parsed.position = sf::Vector2f(x, y);
    }
    return parsed;
}

ParsedMessage MessageHandler::ParseChatMessage(const std::vector<std::string>& parts) {
    ParsedMessage parsed;
    parsed.type = MessageType::Chat;
    if (parts.size() >= 3) {
        parsed.steamID = parts[1];
        parsed.chatMessage = parts[2];
    }
    return parsed;
}

ParsedMessage MessageHandler::ParseReadyStatusMessage(const std::vector<std::string>& parts) {
    ParsedMessage parsed;
    parsed.type = MessageType::ReadyStatus;
    if (parts.size() >= 3) {
        parsed.steamID = parts[1];
        parsed.isReady = (parts[2] == "1");
    }
    return parsed;
}

ParsedMessage MessageHandler::ParseBulletMessage(const std::vector<std::string>& parts) {
    ParsedMessage parsed;
    parsed.type = MessageType::Bullet;
    if (parts.size() >= 5) {
        parsed.steamID = parts[1];
        std::istringstream posStream(parts[2]);
        float px, py;
        char comma;
        posStream >> px >> comma >> py;
        parsed.position = sf::Vector2f(px, py);
        std::istringstream dirStream(parts[3]);
        float dx, dy;
        dirStream >> dx >> comma >> dy;
        parsed.direction = sf::Vector2f(dx, dy);
        parsed.velocity = std::stof(parts[4]);
    }
    return parsed;
}

ParsedMessage MessageHandler::ParsePlayerDeathMessage(const std::vector<std::string>& parts) {
    ParsedMessage parsed;
    parsed.type = MessageType::PlayerDeath;
    if (parts.size() >= 3) {
        parsed.steamID = parts[1];
        parsed.killerID = parts[2];
    }
    return parsed;
}

ParsedMessage MessageHandler::ParsePlayerRespawnMessage(const std::vector<std::string>& parts) {
    ParsedMessage parsed;
    parsed.type = MessageType::PlayerRespawn;
    if (parts.size() >= 3) {
        parsed.steamID = parts[1];
        std::istringstream posStream(parts[2]);
        float x, y;
        char comma;
        posStream >> x >> comma >> y;
        parsed.position = sf::Vector2f(x, y);
    }
    return parsed;
}

ParsedMessage MessageHandler::ParseStartGameMessage(const std::vector<std::string>& parts) {
    ParsedMessage parsed;
    parsed.type = MessageType::StartGame;
    if (parts.size() >= 2) {
        parsed.steamID = parts[1];
    }
    return parsed;
}

ParsedMessage MessageHandler::ParsePlayerDamageMessage(const std::vector<std::string>& parts) {
    ParsedMessage parsed;
    parsed.type = MessageType::PlayerDamage;
    if (parts.size() >= 4) {
        parsed.steamID = parts[1];
        parsed.damage = std::stoi(parts[2]);
        parsed.enemyId = std::stoi(parts[3]);
    }
    return parsed;
}

// Message-specific formatting functions
std::string MessageHandler::FormatConnectionMessage(const std::string& steamID, 
                                                  const std::string& steamName, 
                                                  const sf::Color& color, 
                                                  bool isReady, 
                                                  bool isHost) {
    std::ostringstream oss;
    oss << "C|" << steamID << "|" << steamName << "|" 
        << static_cast<int>(color.r) << "," 
        << static_cast<int>(color.g) << "," 
        << static_cast<int>(color.b) << "|" 
        << (isReady ? "1" : "0") << "|" 
        << (isHost ? "1" : "0");
    return oss.str();
}

std::string MessageHandler::FormatMovementMessage(const std::string& steamID, 
                                                const sf::Vector2f& position) {
    std::ostringstream oss;
    oss << "M|" << steamID << "|" << position.x << "," << position.y;
    return oss.str();
}

std::string MessageHandler::FormatChatMessage(const std::string& steamID, 
                                            const std::string& message) {
    std::ostringstream oss;
    oss << "T|" << steamID << "|" << message;
    return oss.str();
}

std::string MessageHandler::FormatReadyStatusMessage(const std::string& steamID, 
                                                   bool isReady) {
    std::ostringstream oss;
    oss << "R|" << steamID << "|" << (isReady ? "1" : "0");
    return oss.str();
}

std::string MessageHandler::FormatBulletMessage(const std::string& shooterID, 
                                              const sf::Vector2f& position, 
                                              const sf::Vector2f& direction, 
                                              float velocity) {
    std::ostringstream oss;
    oss << "B|" << shooterID << "|" << position.x << "," << position.y << "|" 
        << direction.x << "," << direction.y << "|" << velocity;
    return oss.str();
}

std::string MessageHandler::FormatPlayerDeathMessage(const std::string& playerID, 
                                                   const std::string& killerID) {
    std::ostringstream oss;
    oss << "D|" << playerID << "|" << killerID;
    return oss.str();
}

std::string MessageHandler::FormatPlayerRespawnMessage(const std::string& playerID, 
                                                     const sf::Vector2f& position) {
    std::ostringstream oss;
    oss << "RS|" << playerID << "|" << position.x << "," << position.y;
    return oss.str();
}

std::string MessageHandler::FormatStartGameMessage(const std::string& hostID) {
    std::ostringstream oss;
    oss << "SG|" << hostID;
    return oss.str();
}

std::string MessageHandler::FormatPlayerDamageMessage(const std::string& playerID, 
                                                    int damage, 
                                                    int enemyId) {
    std::ostringstream oss;
    oss << "PD|" << playerID << "|" << damage << "|" << enemyId;
    return oss.str();
}

// Enemy Format and Parse Messages
ParsedMessage MessageHandler::ParseEnemyAddMessage(const std::vector<std::string>& parts) {
    ParsedMessage parsed;
    parsed.type = MessageType::EnemyAdd;
    
    if (parts.size() >= 5) {
        parsed.enemyId = std::stoi(parts[1]);
        parsed.enemyType = static_cast<EnemyType>(std::stoi(parts[2]));
        
        std::string posStr = parts[3];
        size_t commaPos = posStr.find(',');
        if (commaPos != std::string::npos) {
            parsed.position.x = std::stof(posStr.substr(0, commaPos));
            parsed.position.y = std::stof(posStr.substr(commaPos + 1));
        }
        
        parsed.health = std::stof(parts[4]);
    }
    
    return parsed;
}

ParsedMessage MessageHandler::ParseEnemyRemoveMessage(const std::vector<std::string>& parts) {
    ParsedMessage parsed;
    parsed.type = MessageType::EnemyRemove;
    
    if (parts.size() >= 2) {
        parsed.enemyId = std::stoi(parts[1]);
    }
    
    return parsed;
}

ParsedMessage MessageHandler::ParseEnemyDamageMessage(const std::vector<std::string>& parts) {
    ParsedMessage parsed;
    parsed.type = MessageType::EnemyDamage;
    
    if (parts.size() >= 4) {
        parsed.enemyId = std::stoi(parts[1]);
        parsed.damage = std::stof(parts[2]);
        parsed.health = std::stof(parts[3]);
    }
    
    return parsed;
}

ParsedMessage MessageHandler::ParseEnemyPositionUpdateMessage(const std::vector<std::string>& parts) {
    ParsedMessage parsed;
    parsed.type = MessageType::EnemyPositionUpdate;
    
    // Format: EP|id,x,y|id,x,y|...
    for (size_t i = 1; i < parts.size(); ++i) {
        std::string chunk = parts[i];
        std::vector<std::string> subParts = SplitString(chunk, ',');
        
        if (subParts.size() >= 3) {
            parsed.enemyIds.push_back(std::stoi(subParts[0]));
            parsed.enemyPositions.push_back(sf::Vector2f(
                std::stof(subParts[1]),
                std::stof(subParts[2])
            ));
        }
    }
    
    return parsed;
}

ParsedMessage MessageHandler::ParseEnemyStateMessage(const std::vector<std::string>& parts) {
    ParsedMessage parsed;
    parsed.type = MessageType::EnemyState;
    
    // Format: ES|id,type,x,y,health|id,type,x,y,health|...
    for (size_t i = 1; i < parts.size(); ++i) {
        std::string chunk = parts[i];
        std::vector<std::string> subParts = SplitString(chunk, ',');
        
        if (subParts.size() >= 5) {
            parsed.enemyIds.push_back(std::stoi(subParts[0]));
            parsed.enemyTypes.push_back(std::stoi(subParts[1]));
            parsed.enemyPositions.push_back(sf::Vector2f(
                std::stof(subParts[2]),
                std::stof(subParts[3])
            ));
            parsed.enemyHealths.push_back(std::stof(subParts[4]));
        }
    }
    
    return parsed;
}

ParsedMessage MessageHandler::ParseWaveStartMessage(const std::vector<std::string>& parts) {
    ParsedMessage parsed;
    parsed.type = MessageType::WaveStart;
    
    if (parts.size() >= 3) {
        parsed.waveNumber = std::stoi(parts[1]);
        parsed.enemyCount = std::stoi(parts[2]);
    }
    
    return parsed;
}

ParsedMessage MessageHandler::ParseEnemyClearMessage(const std::vector<std::string>& parts) {
    ParsedMessage parsed;
    parsed.type = MessageType::EnemyClear;
    return parsed;
}

// Add to MessageHandler.cpp implementation of the formatting functions
std::string MessageHandler::FormatEnemyAddMessage(int enemyId, EnemyType type, const sf::Vector2f& position, float health) {
    std::ostringstream oss;
    oss << "EA|" << enemyId << "|" << static_cast<int>(type) << "|" 
        << position.x << "," << position.y << "|" << health;
    return oss.str();
}

std::string MessageHandler::FormatEnemyRemoveMessage(int enemyId) {
    std::ostringstream oss;
    oss << "ER|" << enemyId;
    return oss.str();
}

std::string MessageHandler::FormatEnemyDamageMessage(int enemyId, float damage, float remainingHealth) {
    std::ostringstream oss;
    oss << "ED|" << enemyId << "|" << damage << "|" << remainingHealth;
    return oss.str();
}

std::string MessageHandler::FormatEnemyPositionUpdateMessage(const std::vector<int>& enemyIds, const std::vector<sf::Vector2f>& positions) {
    std::ostringstream oss;
    oss << "EP";
    
    size_t count = std::min(enemyIds.size(), positions.size());
    for (size_t i = 0; i < count; ++i) {
        oss << "|" << enemyIds[i] << "," << positions[i].x << "," << positions[i].y;
    }
    
    return oss.str();
}

std::string MessageHandler::FormatEnemyStateMessage(const std::vector<int>& enemyIds, const std::vector<EnemyType>& types, 
                                                  const std::vector<sf::Vector2f>& positions, const std::vector<float>& healths) {
    std::ostringstream oss;
    oss << "ES";
    
    size_t count = std::min(enemyIds.size(), std::min(positions.size(), healths.size()));
    for (size_t i = 0; i < count; ++i) {
        oss << "|" << enemyIds[i] << "," << static_cast<int>(types[i]) << "," 
            << positions[i].x << "," << positions[i].y << "," << healths[i];
    }
    
    return oss.str();
}

std::string MessageHandler::FormatWaveStartMessage(int waveNumber, int enemyCount) {
    std::ostringstream oss;
    oss << "WS|" << waveNumber << "|" << enemyCount;
    return oss.str();
}

std::string MessageHandler::FormatEnemyClearMessage() {
    return "EC";
}