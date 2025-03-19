#include "SystemMessageHandler.h"
#include "MessageHandler.h"
#include "../Client.h"
#include "../Host.h"
#include "../../core/Game.h"
#include <sstream>
#include <iostream>
#include <chrono>
#include <cstdlib>

// Constants

void SystemMessageHandler::Initialize() {
    // Register chunking handlers
    MessageHandler::RegisterMessageType("CHUNK_START", ParseChunkStartMessage,
        [](Game& game, ClientNetwork& client, const ParsedMessage& parsed) {
            // Client just stores the start info
        },
        [](Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender) {
            // Host just stores the start info
        });
  
    MessageHandler::RegisterMessageType("CHUNK_PART", ParseChunkPartMessage,
        [](Game& game, ClientNetwork& client, const ParsedMessage& parsed) {
            // Client just processes the chunk part
        },
        [](Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender) {
            // Host just processes the chunk part
        });
  
    MessageHandler::RegisterMessageType("CHUNK_END", ParseChunkEndMessage,
        [](Game& game, ClientNetwork& client, const ParsedMessage& parsed) {
            // Client should process the reconstructed message
        },
        [](Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender) {
            // Host should process the reconstructed message
        });
        
    MessageHandler::RegisterMessageType("T", 
        ParseChatMessage,
        [](Game& game, ClientNetwork& client, const ParsedMessage& parsed) {
            client.ProcessChatMessage(game, client, parsed);
        },
        [](Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender) {
            host.ProcessChatMessageParsed(game, host, parsed, sender);
        });
    
    // Setup chunking handlers
    MessageHandler::messageParsers["CHUNK_START"] = [](const std::vector<std::string>& parts) {
        if (parts.size() >= 4) {
            std::string messageType = parts[1];
            int totalChunks = std::stoi(parts[2]);
            std::string chunkId = parts[3];
            
            std::cout << "[MessageHandler] Starting new chunked message " << chunkId 
                      << " of type " << messageType << " with " << totalChunks << " chunks\n";
            
            MessageHandler::chunkTypes[chunkId] = messageType;
            MessageHandler::chunkCounts[chunkId] = totalChunks;
            MessageHandler::chunkStorage[chunkId].resize(totalChunks);
        }
        return ParsedMessage{MessageType::ChunkStart}; // Create a specific type for chunks
    };
    
    MessageHandler::messageParsers["CHUNK_PART"] = [](const std::vector<std::string>& parts) {
        ParsedMessage result{MessageType::ChunkPart};
        if (parts.size() >= 4) {
            std::string chunkId = parts[1];
            int chunkNum = std::stoi(parts[2]);
            std::string chunkData = parts[3];
            
            std::cout << "[MessageHandler] Processing chunk part " << chunkNum 
                      << " for message " << chunkId << "\n";
            
            AddChunk(chunkId, chunkNum, chunkData);
        }
        return result;
    };
    
    MessageHandler::messageParsers["CHUNK_END"] = [](const std::vector<std::string>& parts) {
        if (parts.size() >= 2) {
            std::string chunkId = parts[1];
            std::cout << "[MessageHandler] Processing CHUNK_END for " << chunkId << "\n";
            
            if (MessageHandler::chunkStorage.find(chunkId) != MessageHandler::chunkStorage.end() && 
                MessageHandler::chunkCounts.find(chunkId) != MessageHandler::chunkCounts.end()) {
                
                int expectedChunks = MessageHandler::chunkCounts[chunkId];
                if (IsChunkComplete(chunkId, expectedChunks)) {
                    // Get message type
                    std::string messageType = MessageHandler::chunkTypes[chunkId];
                    
                    // Reconstruct the full message
                    std::string fullMessage = GetReconstructedMessage(chunkId);
                    
                    // Parse the reconstructed message
                    std::vector<std::string> messageParts = MessageHandler::SplitString(fullMessage, '|');
                    auto parserIt = MessageHandler::messageParsers.find(messageType);
                    if (parserIt != MessageHandler::messageParsers.end()) {
                        // Clear chunks to free memory
                        ClearChunks(chunkId);
                        
                        // Parse the reconstructed message and return the result
                        return parserIt->second(messageParts);
                    }
                    else {
                        std::cout << "[MessageHandler] No parser found for message type: " << messageType << "\n";
                        ClearChunks(chunkId);
                    }
                }
                else {
                    std::cout << "[MessageHandler] Chunks incomplete for " << chunkId << ". Expected: " 
                              << expectedChunks << ", Have: " << MessageHandler::chunkStorage[chunkId].size() << "\n";
                }
            }
            else {
                std::cout << "[MessageHandler] Chunk storage or counts not found for " << chunkId << "\n";
            }
        }
        
        return ParsedMessage{MessageType::ChunkEnd};
    };
}

// Chat message parsing
ParsedMessage SystemMessageHandler::ParseChatMessage(const std::vector<std::string>& parts) {
    ParsedMessage parsed;
    parsed.type = MessageType::Chat;
    if (parts.size() >= 3) {
        parsed.steamID = parts[1];
        parsed.chatMessage = parts[2];
    }
    return parsed;
}

// Chunk message parsing
ParsedMessage SystemMessageHandler::ParseChunkStartMessage(const std::vector<std::string>& parts) {
    ParsedMessage parsed;
    parsed.type = MessageType::ChunkStart;
    return parsed;
}

ParsedMessage SystemMessageHandler::ParseChunkPartMessage(const std::vector<std::string>& parts) {
    ParsedMessage parsed;
    parsed.type = MessageType::ChunkPart;
    return parsed;
}

ParsedMessage SystemMessageHandler::ParseChunkEndMessage(const std::vector<std::string>& parts) {
    ParsedMessage parsed;
    parsed.type = MessageType::ChunkEnd;
    return parsed;
}

// Message formatting functions
std::string SystemMessageHandler::FormatChatMessage(const std::string& steamID, 
                                               const std::string& message) {
    std::ostringstream oss;
    oss << "T|" << steamID << "|" << message;
    return oss.str();
}

// Chunking functions
std::vector<std::string> SystemMessageHandler::ChunkMessage(const std::string& message, const std::string& messageType) {
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

std::string SystemMessageHandler::FormatChunkStartMessage(const std::string& messageType, int totalChunks, const std::string& chunkId) {
    std::ostringstream oss;
    oss << "CHUNK_START|" << messageType << "|" << totalChunks << "|" << chunkId;
    return oss.str();
}

std::string SystemMessageHandler::FormatChunkPartMessage(const std::string& chunkId, int chunkNum, const std::string& chunkData) {
    std::ostringstream oss;
    oss << "CHUNK_PART|" << chunkId << "|" << chunkNum << "|" << chunkData;
    return oss.str();
}

std::string SystemMessageHandler::FormatChunkEndMessage(const std::string& chunkId) {
    std::ostringstream oss;
    oss << "CHUNK_END|" << chunkId;
    return oss.str();
}

void SystemMessageHandler::AddChunk(const std::string& chunkId, int chunkNum, const std::string& chunkData) {
    // Make sure the storage exists for this chunk ID
    if (MessageHandler::chunkStorage.find(chunkId) == MessageHandler::chunkStorage.end()) {
        int expectedCount = 0;
        auto countIt = MessageHandler::chunkCounts.find(chunkId);
        if (countIt != MessageHandler::chunkCounts.end()) {
            expectedCount = countIt->second;
        } else {
            // If we don't know how many chunks to expect, use a reasonable default
            expectedCount = std::max(10, chunkNum + 1);
            MessageHandler::chunkCounts[chunkId] = expectedCount;
        }
        
        MessageHandler::chunkStorage[chunkId].resize(expectedCount);
        std::cout << "[MessageHandler] Created storage for chunk ID " << chunkId 
                  << " with " << expectedCount << " slots\n";
    }
    
    // Ensure the vector is large enough
    if (static_cast<size_t>(chunkNum) >= MessageHandler::chunkStorage[chunkId].size()) {
        size_t newSize = chunkNum + 1;
        std::cout << "[MessageHandler] Resizing chunk storage for " << chunkId 
                  << " from " << MessageHandler::chunkStorage[chunkId].size() << " to " << newSize << "\n";
        MessageHandler::chunkStorage[chunkId].resize(newSize);
        
        // Update expected count if necessary
        if (MessageHandler::chunkCounts[chunkId] < static_cast<int>(newSize)) {
            MessageHandler::chunkCounts[chunkId] = static_cast<int>(newSize);
        }
    }
    
    // Store the chunk
    MessageHandler::chunkStorage[chunkId][chunkNum] = chunkData;
    
    // Debug output
    std::cout << "[MessageHandler] Added chunk " << chunkNum << " of " << MessageHandler::chunkCounts[chunkId] 
              << " for ID " << chunkId << "\n";
}

bool SystemMessageHandler::IsChunkComplete(const std::string& chunkId, int expectedChunks) {
    if (MessageHandler::chunkStorage.find(chunkId) == MessageHandler::chunkStorage.end()) {
        std::cout << "[MessageHandler] Chunk ID " << chunkId << " not found in storage\n";
        return false;
    }
    
    // Verify all chunks are present
    if (MessageHandler::chunkStorage[chunkId].size() != static_cast<size_t>(expectedChunks)) {
        std::cout << "[MessageHandler] Chunk count mismatch for " << chunkId 
                  << ": have " << MessageHandler::chunkStorage[chunkId].size() 
                  << ", need " << expectedChunks << "\n";
                  
        // If we have more chunks than expected, update the expected count
        if (MessageHandler::chunkStorage[chunkId].size() > static_cast<size_t>(expectedChunks)) {
            MessageHandler::chunkCounts[chunkId] = static_cast<int>(MessageHandler::chunkStorage[chunkId].size());
            expectedChunks = MessageHandler::chunkCounts[chunkId];
            std::cout << "[MessageHandler] Updated expected chunk count to " << expectedChunks << "\n";
        } else {
            return false;
        }
    }
    
    // Verify no chunks are empty
    bool complete = true;
    for (size_t i = 0; i < MessageHandler::chunkStorage[chunkId].size(); i++) {
        if (MessageHandler::chunkStorage[chunkId][i].empty()) {
            std::cout << "[MessageHandler] Missing chunk " << i << " for " << chunkId << "\n";
            complete = false;
            break;
        }
    }
    
    if (complete) {
        std::cout << "[MessageHandler] All " << expectedChunks << " chunks received for " << chunkId << "\n";
    }
    
    return complete;
}

std::string SystemMessageHandler::GetReconstructedMessage(const std::string& chunkId) {
    if (MessageHandler::chunkStorage.find(chunkId) == MessageHandler::chunkStorage.end() || 
        MessageHandler::chunkTypes.find(chunkId) == MessageHandler::chunkTypes.end()) {
        return "";
    }
    
    // Get the message type for this chunked message
    std::string messageType = MessageHandler::chunkTypes[chunkId];
    
    // Start with the message type as prefix
    std::ostringstream result;
    result << messageType;
    
    // Join all chunks without additional separators - the content already has them
    for (size_t i = 0; i < MessageHandler::chunkStorage[chunkId].size(); ++i) {
        result << MessageHandler::chunkStorage[chunkId][i];
    }
    
    std::string finalMessage = result.str();
    std::cout << "[MessageHandler] Reconstructed message with type " << messageType 
              << ", length: " << finalMessage.length() << "\n";
    
    return finalMessage;
}

void SystemMessageHandler::ClearChunks(const std::string& chunkId) {
    MessageHandler::chunkStorage.erase(chunkId);
    MessageHandler::chunkTypes.erase(chunkId);
    MessageHandler::chunkCounts.erase(chunkId);
}