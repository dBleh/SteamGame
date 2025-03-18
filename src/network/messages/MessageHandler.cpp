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
            std::cout << "[MessageHandler] Processing CHUNK_END for " << chunkId << "\n";
            
            if (chunkStorage.find(chunkId) != chunkStorage.end() && 
                chunkCounts.find(chunkId) != chunkCounts.end()) {
                
                int expectedChunks = chunkCounts[chunkId];
                if (IsChunkComplete(chunkId, expectedChunks)) {
                    // Get message type
                    std::string messageType = chunkTypes[chunkId];
                    
                    // Reconstruct the full message
                    std::string fullMessage = GetReconstructedMessage(chunkId);
                    
                    // Clear chunks to free memory
                    ClearChunks(chunkId);
                    
                    // Debug output
                    std::cout << "[MessageHandler] Reconstructed message: " << messageType 
                              << " (length: " << fullMessage.length() << ")\n";
                    
                    // Find the appropriate parser for this message type
                    auto parserIt = messageParsers.find(messageType);
                    if (parserIt != messageParsers.end()) {
                        // Re-parse the reconstructed message
                        std::vector<std::string> messageParts = SplitString(fullMessage, '|');
                        return parserIt->second(messageParts);
                    }
                    else {
                        std::cout << "[MessageHandler] No parser found for message type: " << messageType << "\n";
                    }
                }
                else {
                    std::cout << "[MessageHandler] Chunks incomplete for " << chunkId << "\n";
                }
            }
            else {
                std::cout << "[MessageHandler] Chunk storage or counts not found for " << chunkId << "\n";
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
    // Make sure the storage exists for this chunk ID
    if (chunkStorage.find(chunkId) == chunkStorage.end()) {
        chunkStorage[chunkId] = std::vector<std::string>();
        // Try to get expected count from map
        int expectedCount = chunkCounts[chunkId];
        if (expectedCount > 0) {
            chunkStorage[chunkId].resize(expectedCount);
        } else {
            // If we don't know how many chunks to expect, allocate enough for this one
            chunkStorage[chunkId].resize(chunkNum + 1);
        }
    }
    
    // Ensure the vector is large enough
    if (chunkStorage[chunkId].size() <= static_cast<size_t>(chunkNum)) {
        chunkStorage[chunkId].resize(chunkNum + 1);
    }
    
    // Store the chunk
    chunkStorage[chunkId][chunkNum] = chunkData;
    
    // Debug output
    std::cout << "[MessageHandler] Added chunk " << chunkNum << " of " << chunkCounts[chunkId] 
              << " for ID " << chunkId << " (" << chunkData.substr(0, 20) << "...)\n";
}

bool MessageHandler::IsChunkComplete(const std::string& chunkId, int expectedChunks) {
    if (chunkStorage.find(chunkId) == chunkStorage.end()) {
        std::cout << "[MessageHandler] Chunk ID " << chunkId << " not found in storage\n";
        return false;
    }
    
    if (chunkStorage[chunkId].size() != static_cast<size_t>(expectedChunks)) {
        std::cout << "[MessageHandler] Chunk count mismatch for " << chunkId 
                  << ": have " << chunkStorage[chunkId].size() 
                  << ", need " << expectedChunks << "\n";
        return false;
    }
    
    // Verify all chunks are present
    bool complete = true;
    for (size_t i = 0; i < chunkStorage[chunkId].size(); i++) {
        if (chunkStorage[chunkId][i].empty()) {
            std::cout << "[MessageHandler] Missing chunk " << i << " for " << chunkId << "\n";
            complete = false;
        }
    }
    
    if (complete) {
        std::cout << "[MessageHandler] All " << expectedChunks << " chunks received for " << chunkId << "\n";
    }
    
    return complete;
}

std::string MessageHandler::GetReconstructedMessage(const std::string& chunkId) {
    if (chunkStorage.find(chunkId) == chunkStorage.end() || 
        chunkTypes.find(chunkId) == chunkTypes.end()) {
        return "";
    }
    
    // Get the message type for this chunked message
    std::string messageType = chunkTypes[chunkId];
    
    // Start with the message type as prefix
    std::ostringstream result;
    result << messageType;
    
    // Join all chunks with the | delimiter
    for (size_t i = 0; i < chunkStorage[chunkId].size(); ++i) {
        result << "|" << chunkStorage[chunkId][i];
    }
    
    std::string finalMessage = result.str();
    std::cout << "[MessageHandler] Reconstructed message with type " << messageType 
              << ", length: " << finalMessage.length() << "\n";
    
    return finalMessage;
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
    
    std::cout << "[MessageHandler] Parsing ES message with " << parts.size() << " parts\n";
    
    // First, let's handle the case where we have a full message with multiple entities
    // Format might be: ES|entity1|entity2|entity3|...
    for (size_t i = 1; i < parts.size(); ++i) {
        std::string entityData = parts[i];
        
        // Skip empty parts
        if (entityData.empty()) continue;
        
        // Try to parse individual entity data
        std::vector<std::string> fields = SplitString(entityData, ',');
        
        // Expected format: id,type,x,y,health
        if (fields.size() >= 5) {
            try {
                int id = std::stoi(fields[0]);
                int type = std::stoi(fields[1]);
                float x = std::stof(fields[2]);
                float y = std::stof(fields[3]);
                float health = std::stof(fields[4]);
                
                parsed.enemyIds.push_back(id);
                parsed.enemyTypes.push_back(type);
                parsed.enemyPositions.push_back(sf::Vector2f(x, y));
                parsed.enemyHealths.push_back(health);
                
                std::cout << "[MessageHandler] Parsed enemy: id=" << id 
                          << ", pos=(" << x << "," << y << ")\n";
            }
            catch (const std::exception& e) {
                std::cout << "[MessageHandler] Error parsing enemy data: " << entityData 
                          << " - Error: " << e.what() << "\n";
            }
        }
        else {
            std::cout << "[MessageHandler] Skipping malformed enemy data: " << entityData 
                      << " (fields: " << fields.size() << ")\n";
        }
    }
    
    std::cout << "[MessageHandler] Successfully parsed " << parsed.enemyIds.size() << " enemies\n";
    return parsed;
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
            ParsedMessage recoveredMessage = ParseEnemyStateMessage(parts);
            
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