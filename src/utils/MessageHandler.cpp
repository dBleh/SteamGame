#include "MessageHandler.h"
#include <sstream>
#include <vector>
#include "../Game.h"
#include "../states/PlayingState.h"
#include "../entities/EnemyManager.h"


std::unordered_map<std::string, std::vector<std::string>> MessageHandler::chunkStorage;
std::unordered_map<std::string, std::string> MessageHandler::chunkTypes;
std::unordered_map<std::string, int> MessageHandler::chunkCounts;

std::string MessageHandler::FormatConnectionMessage(const std::string& steamID, const std::string& steamName, const sf::Color& color, bool isReady, bool isHost) {
    std::ostringstream oss;
    oss << "C|" << steamID << "|" << steamName << "|" << static_cast<int>(color.r) << "," 
        << static_cast<int>(color.g) << "," << static_cast<int>(color.b) << "|" 
        << (isReady ? "1" : "0") << "|" << (isHost ? "1" : "0");
    return oss.str();
}

std::string MessageHandler::FormatMovementMessage(const std::string& steamID, const sf::Vector2f& position) {
    std::ostringstream oss;
    oss << "M|" << steamID << "|" << position.x << "," << position.y;
    return oss.str();
}

std::vector<std::string> MessageHandler::ChunkMessage(const std::string& message, const std::string& messageType) {
    std::vector<std::string> chunks;
    
    // If the message is small enough to send directly, don't chunk it
    if (message.size() <= MAX_PACKET_SIZE) {
        chunks.push_back(message);
        return chunks;
    }
    
    // Generate a unique chunk ID (timestamp + random number)
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    std::string chunkId = std::to_string(timestamp) + "_" + std::to_string(rand() % 10000);
    
    // Split the message into chunks
    size_t chunkSize = MAX_PACKET_SIZE - 50; // Leave room for chunk headers
    size_t numChunks = (message.size() + chunkSize - 1) / chunkSize; // Ceiling division
    
    // Add the start message
    chunks.push_back(FormatChunkStartMessage(messageType, static_cast<int>(numChunks), chunkId));
    
    // Split and add each chunk
    for (size_t i = 0; i < numChunks; i++) {
        size_t start = i * chunkSize;
        size_t end = std::min(start + chunkSize, message.size());
        std::string chunkData = message.substr(start, end - start);
        
        chunks.push_back(FormatChunkPartMessage(chunkId, static_cast<int>(i), chunkData));
    }
    
    // Add the end message
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
    // Ensure the vector is large enough
    if (chunkStorage[chunkId].size() <= static_cast<size_t>(chunkNum)) {
        chunkStorage[chunkId].resize(chunkNum + 1);
    }
    
    // Store the chunk
    chunkStorage[chunkId][chunkNum] = chunkData;
}

bool MessageHandler::IsChunkComplete(const std::string& chunkId, int expectedChunks) {
    // Check if we have the right number of chunks
    if (chunkStorage.find(chunkId) == chunkStorage.end() ||
        chunkStorage[chunkId].size() != static_cast<size_t>(expectedChunks)) {
        return false;
    }
    
    // Check that no chunks are empty
    for (const auto& chunk : chunkStorage[chunkId]) {
        if (chunk.empty()) {
            return false;
        }
    }
    
    return true;
}

std::string MessageHandler::GetReconstructedMessage(const std::string& chunkId) {
    if (chunkStorage.find(chunkId) == chunkStorage.end()) {
        return "";
    }
    
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

std::string MessageHandler::FormatChatMessage(const std::string& steamID, const std::string& message) {
    std::ostringstream oss;
    oss << "T|" << steamID << "|" << message;
    return oss.str();
}

std::string MessageHandler::FormatReadyStatusMessage(const std::string& steamID, bool isReady) {
    std::ostringstream oss;
    oss << "R|" << steamID << "|" << (isReady ? "1" : "0");
    return oss.str();
}

std::string MessageHandler::FormatBulletMessage(const std::string& shooterID, const sf::Vector2f& position, const sf::Vector2f& direction, float velocity) {
    // Normalize the shooter ID to ensure consistent format
    std::string normalizedID = shooterID;
    try {
        uint64_t idNum = std::stoull(shooterID);
        normalizedID = std::to_string(idNum);
    } catch (const std::exception&) {
        // Keep original if conversion fails
    }

    std::ostringstream oss;
    oss << "B|" << normalizedID << "|" << position.x << "," << position.y << "|" 
        << direction.x << "," << direction.y << "|" << velocity;
    return oss.str();
}

std::string MessageHandler::FormatPlayerDeathMessage(const std::string& playerID, const std::string& killerID) {
    std::ostringstream oss;
    oss << "D|" << playerID << "|" << killerID;
    return oss.str();
}

std::string MessageHandler::FormatEnemyValidationMessage(const std::vector<int>& enemyIds) {
    std::ostringstream oss;
    oss << "EV|" << enemyIds.size();
    
    for (int id : enemyIds) {
        oss << "|" << id;
    }
    
    return oss.str();
}
std::string MessageHandler::FormatEnemyClearMessage() {
    std::stringstream ss;
    ss << "EC|"; // EnemyClear message type
    return ss.str();
}
std::string MessageHandler::FormatPlayerRespawnMessage(const std::string& playerID, const sf::Vector2f& position) {
    std::ostringstream oss;
    oss << "RS|" << playerID << "|" << position.x << "," << position.y;
    return oss.str();
}

std::string MessageHandler::FormatStartGameMessage(const std::string& hostID) {
    std::ostringstream oss;
    oss << "SG|" << hostID;
    return oss.str();
}

std::string MessageHandler::FormatEnemySpawnMessage(int enemyId, const sf::Vector2f& position, ParsedMessage::EnemyType enemyType) {
    std::ostringstream oss;
    oss << "ES|" << enemyId << "|" << position.x << "," << position.y << "|" << static_cast<int>(enemyType);
    return oss.str();
}

std::string MessageHandler::FormatEnemyBatchSpawnMessage(
    const std::vector<std::tuple<int, sf::Vector2f, int>>& batchData, 
    ParsedMessage::EnemyType enemyType) {
    
    std::stringstream ss;
    ss << "EBATCH|" << static_cast<int>(enemyType) << "|";
    
    // Format: enemyId,posX,posY,health;enemyId,posX,posY,health;...
    for (const auto& data : batchData) {
        int id = std::get<0>(data);
        sf::Vector2f pos = std::get<1>(data);
        int health = std::get<2>(data);
        
        // Always include health in the message
        ss << id << "," << pos.x << "," << pos.y << "," << health << ";";
    }
    
    return ss.str();
}

std::string MessageHandler::FormatEnemyPositionsMessage(const std::vector<std::tuple<int, sf::Vector2f, int>>& enemyData) {
    std::ostringstream oss;
    oss << "EP|" << enemyData.size();
    
    for (const auto& data : enemyData) {
        int id = std::get<0>(data);
        sf::Vector2f pos = std::get<1>(data);
        int health = std::get<2>(data);
        
        oss << "|" << id << "," << pos.x << "," << pos.y << "," << health;
    }
    
    return oss.str();
}

std::string MessageHandler::FormatEnemyHitMessage(int enemyId, int damage, bool killed, const std::string& shooterID, ParsedMessage::EnemyType enemyType) {
    std::ostringstream oss;
    oss << "EH|" << enemyId << "|" << damage << "|" << (killed ? "1" : "0") << "|" << shooterID << "|" << static_cast<int>(enemyType);
    return oss.str();
}

std::string MessageHandler::FormatEnemyDeathMessage(int enemyId, const std::string& killerID, bool rewardKill, ParsedMessage::EnemyType enemyType) {
    std::ostringstream oss;
    oss << "ED|" << enemyId << "|" << killerID << "|" << (rewardKill ? "1" : "0") << "|" << static_cast<int>(enemyType);
    return oss.str();
}

std::string MessageHandler::FormatPlayerDamageMessage(const std::string& playerID, int damage, int enemyId) {
    std::ostringstream oss;
    oss << "PD|" << playerID << "|" << damage << "|" << enemyId;
    return oss.str();
}

std::string MessageHandler::FormatWaveStartMessage(int waveNumber) {
    std::ostringstream oss;
    oss << "WS|" << waveNumber;
    return oss.str();
}

std::string MessageHandler::FormatWaveCompleteMessage(int waveNumber) {
    std::ostringstream oss;
    oss << "WC|" << waveNumber;
    return oss.str();
}

std::string MessageHandler::FormatWaveStartWithTypesMessage(int waveNumber, uint32_t seed, const std::vector<int>& typeInts) {
    std::ostringstream oss;
    oss << "WST|" << waveNumber << "|" << seed << "|" << typeInts.size();
    
    for (int type : typeInts) {
        oss << "|" << type;
    }
    
    return oss.str();
}

std::string MessageHandler::FormatEnemyValidationRequestMessage() {
    return "EVR|";  // Simple message, no additional data needed
}

std::string MessageHandler::FormatTriangleWaveStartMessage(uint32_t seed, int enemyCount) {
    std::ostringstream oss;
    oss << "TWS|" << seed << "|" << enemyCount;
    return oss.str();
}

std::string MessageHandler::FormatEnemyFullListMessage(const std::vector<int>& enemyIds, ParsedMessage::EnemyType enemyType) {
    std::ostringstream oss;
    oss << "EFL|" << static_cast<int>(enemyType) << "|" << enemyIds.size();
    
    for (int id : enemyIds) {
        oss << "|" << id;
    }
    
    return oss.str();
}

ParsedMessage MessageHandler::ParseTriangleWaveStartMessage(const std::string& message) {
    ParsedMessage result;
    result.type = MessageType::TriangleWaveStart;
    
    std::istringstream ss(message);
    std::string token;
    
    // Skip "TWS" token
    std::getline(ss, token, '|');
    
    // Parse seed
    std::getline(ss, token, '|');
    try {
        result.seed = std::stoul(token);
    } catch (...) {
        result.seed = 0;
    }
    
    // Parse count
    std::getline(ss, token, '|');
    try {
        result.enemyCount = std::stoi(token);
    } catch (...) {
        result.enemyCount = 0;
    }
    
    return result;
}

ParsedMessage MessageHandler::ParseMessage(const std::string& msg) {
    ParsedMessage parsed{};
    std::vector<std::string> parts;
    std::istringstream iss(msg);
    std::string part;
    while (std::getline(iss, part, '|')) {
        parts.push_back(part);
    }

    if (parts.empty()) return parsed;

    if (parts[0] == "C" && parts.size() >= 6) {
        parsed.type = MessageType::Connection;
        parsed.steamID = parts[1];
        parsed.steamName = parts[2];
        std::istringstream colorStream(parts[3]);
        int r, g, b;
        char comma;
        colorStream >> r >> comma >> g >> comma >> b;
        parsed.color = sf::Color(r, g, b);
        parsed.isReady = (parts[4] == "1");
        parsed.isHost = (parts[5] == "1");
    } else if (parts[0] == "M" && parts.size() >= 3) {
        parsed.type = MessageType::Movement;
        parsed.steamID = parts[1];
        std::istringstream posStream(parts[2]);
        float x, y;
        char comma;
        posStream >> x >> comma >> y;
        parsed.position = sf::Vector2f(x, y);
    } else if (parts[0] == "T" && parts.size() >= 3) {
        parsed.type = MessageType::Chat;
        parsed.steamID = parts[1];
        parsed.chatMessage = parts[2];
    } else if (parts[0] == "R" && parts.size() >= 3) {
        parsed.type = MessageType::ReadyStatus;
        parsed.steamID = parts[1];
        parsed.isReady = (parts[2] == "1");
    } else if (parts[0] == "B" && parts.size() >= 5) {
        parsed.type = MessageType::Bullet;
        parsed.steamID = parts[1];  // shooterID
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
    } else if (parts[0] == "D" && parts.size() >= 3) {
        parsed.type = MessageType::PlayerDeath;
        parsed.steamID = parts[1];    // Player who died
        parsed.killerID = parts[2];   // Player who caused the death
    } else if (parts[0] == "RS" && parts.size() >= 3) {
        parsed.type = MessageType::PlayerRespawn;
        parsed.steamID = parts[1];
        
        std::istringstream posStream(parts[2]);
        float x, y;
        char comma;
        posStream >> x >> comma >> y;
        parsed.position = sf::Vector2f(x, y);
    } else if (parts[0] == "SG" && parts.size() >= 2) {
        parsed.type = MessageType::StartGame;
        parsed.steamID = parts[1];    // Host ID who started the game
    } 
    // Enemy-related message parsing
    else if (parts[0] == "ES" && parts.size() >= 3) {
        parsed.type = MessageType::EnemySpawn;
        parsed.enemyId = std::stoi(parts[1]);
        
        std::istringstream posStream(parts[2]);
        float x, y;
        char comma;
        posStream >> x >> comma >> y;
        parsed.position = sf::Vector2f(x, y);
        
        // Parse enemy type if provided
        if (parts.size() >= 4) {
            parsed.enemyType = static_cast<ParsedMessage::EnemyType>(std::stoi(parts[3]));
        } else {
            parsed.enemyType = ParsedMessage::EnemyType::Regular; // Default
        }
    } else if (parts[0] == "EH" && parts.size() >= 5) {
        parsed.type = MessageType::EnemyHit;
        parsed.enemyId = std::stoi(parts[1]);
        parsed.damage = std::stoi(parts[2]);
        parsed.killed = (parts[3] == "1");
        parsed.steamID = parts[4];  // Shooter ID
        
        // Parse enemy type if available
        if (parts.size() >= 6) {
            parsed.enemyType = static_cast<ParsedMessage::EnemyType>(std::stoi(parts[5]));
        } else {
            // Default to Regular if not specified
            parsed.enemyType = ParsedMessage::EnemyType::Regular;
        }
    } else if (parts[0] == "ED" && parts.size() >= 4) {
        parsed.type = MessageType::EnemyDeath;
        parsed.enemyId = std::stoi(parts[1]);
        parsed.killerID = parts[2];
        parsed.rewardKill = (parts[3] == "1");
        
        // Parse enemy type if available
        if (parts.size() >= 5) {
            parsed.enemyType = static_cast<ParsedMessage::EnemyType>(std::stoi(parts[4]));
        } else {
            // Default to Regular if not specified
            parsed.enemyType = ParsedMessage::EnemyType::Regular;
        }
    } else if (parts[0] == "PD" && parts.size() >= 4) {
        parsed.type = MessageType::PlayerDamage;
        parsed.steamID = parts[1];   // Player who was damaged
        parsed.damage = std::stoi(parts[2]);
        parsed.enemyId = std::stoi(parts[3]);
    } else if (parts[0] == "WS" && parts.size() >= 2) {
        parsed.type = MessageType::WaveStart;
        parsed.waveNumber = std::stoi(parts[1]);
    } else if (parts[0] == "WC" && parts.size() >= 2) {
        parsed.type = MessageType::WaveComplete;
        parsed.waveNumber = std::stoi(parts[1]);
    } else if (parts[0] == "EP" && parts.size() >= 2) {
        parsed.type = MessageType::EnemyPositions;
        int numEnemies = std::stoi(parts[1]);
        
        parsed.enemyPositions.clear();
        parsed.enemyHealths.clear();
        
        for (int i = 0; i < numEnemies && i + 2 < parts.size(); ++i) {
            // Get the raw data for this enemy
            std::string enemyData = parts[i + 2];
            
            // Split the data by commas
            std::vector<std::string> values;
            std::stringstream ss(enemyData);
            std::string value;
            
            while (std::getline(ss, value, ',')) {
                values.push_back(value);
            }
            
            // Check if we have enough values
            if (values.size() >= 4) {
                try {
                    int id = std::stoi(values[0]);
                    float x = std::stof(values[1]);
                    float y = std::stof(values[2]);
                    int health = std::stoi(values[3]);
                    
                    parsed.enemyPositions.emplace_back(id, sf::Vector2f(x, y));
                    parsed.enemyHealths.emplace_back(id, health);
                } catch (const std::exception& e) {
                    std::cout << "[ERROR] Failed to parse enemy position data: " << e.what() << std::endl;
                }
            }
        }
    } else if (parts[0] == "EV" && parts.size() >= 2) {
        parsed.type = MessageType::EnemyValidation;
        int numEnemies = std::stoi(parts[1]);
        
        for (int i = 0; i < numEnemies && i + 2 < parts.size(); ++i) {
            parsed.validEnemyIds.push_back(std::stoi(parts[i + 2]));
        }
    } else if (parts[0] == "EFL" && parts.size() >= 3) {
        parsed.type = MessageType::EnemyValidation;
        parsed.enemyType = static_cast<ParsedMessage::EnemyType>(std::stoi(parts[1]));
        int numEnemies = std::stoi(parts[2]);
        
        parsed.validEnemyIds.clear();
        
        for (int i = 0; i < numEnemies && i + 3 < parts.size(); ++i) {
            parsed.validEnemyIds.push_back(std::stoi(parts[i + 3]));
        }
    } else if (parts[0] == "EBATCH" && parts.size() >= 3) {
        // Update to match FormatEnemyBatchSpawnMessage format
        parsed.type = MessageType::EnemyBatchSpawn;
        parsed.enemyType = static_cast<ParsedMessage::EnemyType>(std::stoi(parts[1]));
        
        parsed.enemyPositions.clear();
        parsed.enemyHealths.clear();
        
        // Format from FormatEnemyBatchSpawnMessage: enemyId,posX,posY,health;enemyId,posX,posY,health;...
        // Split by semicolon to get each enemy data
        std::vector<std::string> enemyDataList;
        std::stringstream ss(parts[2]);
        std::string enemyData;
        
        while (std::getline(ss, enemyData, ';')) {
            if (!enemyData.empty()) {
                enemyDataList.push_back(enemyData);
            }
        }
        
        // Process each enemy entry
        for (const auto& data : enemyDataList) {
            std::vector<std::string> values;
            std::stringstream valueStream(data);
            std::string value;
            
            while (std::getline(valueStream, value, ',')) {
                values.push_back(value);
            }
            
            if (values.size() >= 4) {
                try {
                    int id = std::stoi(values[0]);
                    float x = std::stof(values[1]);
                    float y = std::stof(values[2]);
                    int health = std::stoi(values[3]);
                    
                    parsed.enemyPositions.emplace_back(id, sf::Vector2f(x, y));
                    parsed.enemyHealths.emplace_back(id, health);
                } catch (const std::exception& e) {
                    std::cout << "[ERROR] Failed to parse enemy batch spawn data: " << e.what() << std::endl;
                }
            }
        }
    } else if (parts[0] == "EBS" && parts.size() >= 3) {
        // Keep the old format for backward compatibility
        parsed.type = MessageType::EnemyBatchSpawn;
        parsed.enemyType = static_cast<ParsedMessage::EnemyType>(std::stoi(parts[1]));
        int count = std::stoi(parts[2]);
        
        parsed.enemyPositions.clear();
        parsed.enemyHealths.clear();
        
        for (int i = 0; i < count && i + 3 < parts.size(); ++i) {
            std::string enemyData = parts[i + 3];
            std::vector<std::string> values;
            std::stringstream ss(enemyData);
            std::string value;
            
            while (std::getline(ss, value, ',')) {
                values.push_back(value);
            }
            
            if (values.size() >= 4) {
                try {
                    int id = std::stoi(values[0]);
                    float x = std::stof(values[1]);
                    float y = std::stof(values[2]);
                    int health = std::stoi(values[3]);
                    
                    parsed.enemyPositions.emplace_back(id, sf::Vector2f(x, y));
                    parsed.enemyHealths.emplace_back(id, health);
                } catch (const std::exception& e) {
                    std::cout << "[ERROR] Failed to parse enemy batch spawn data: " << e.what() << std::endl;
                }
            }
        }
    } else if (parts[0] == "EVR") {
        parsed.type = MessageType::EnemyValidationRequest;
    } else if (parts[0] == "WST" && parts.size() >= 4) {
        parsed.type = MessageType::WaveStart;
        parsed.waveNumber = std::stoi(parts[1]);
        parsed.seed = std::stoul(parts[2]);
        int typeCount = std::stoi(parts[3]);
        
        // Parse enemy types if available
        for (int i = 0; i < typeCount && i + 4 < parts.size(); ++i) {
            int typeValue = std::stoi(parts[i + 4]);
            if (typeValue == static_cast<int>(ParsedMessage::EnemyType::Triangle)) {
                parsed.enemyType = ParsedMessage::EnemyType::Triangle;
            }
            // Could store multiple types in the future if needed
        }
    } else if (parts[0] == "TWS") {
        return ParseTriangleWaveStartMessage(msg);
    } else if (parts[0] == "CHUNK_START" && parts.size() >= 4) {
        parsed.type = MessageType::ChunkStart;
        parsed.chunkType = parts[1];
        parsed.totalChunks = std::stoi(parts[2]);
        parsed.chunkId = parts[3];
        
        // Initialize storage for this chunk group
        chunkStorage[parsed.chunkId].clear();
        chunkStorage[parsed.chunkId].resize(parsed.totalChunks);
        chunkTypes[parsed.chunkId] = parsed.chunkType;
        chunkCounts[parsed.chunkId] = parsed.totalChunks;
    } else if (parts[0] == "CHUNK_PART" && parts.size() >= 4) {
        parsed.type = MessageType::ChunkPart;
        parsed.chunkId = parts[1];
        parsed.chunkNum = std::stoi(parts[2]);
        
        // Reconstruct the chunk data (which may contain pipe characters)
        std::string chunkData;
        for (size_t i = 3; i < parts.size(); i++) {
            if (i > 3) chunkData += "|";
            chunkData += parts[i];
        }
        
        // Store the chunk
        AddChunk(parsed.chunkId, parsed.chunkNum, chunkData);
    } else if (parts[0] == "CHUNK_END" && parts.size() >= 2) {
        parsed.type = MessageType::ChunkEnd;
        parsed.chunkId = parts[1];
        
        // Check if all chunks have been received
        if (chunkCounts.find(parsed.chunkId) != chunkCounts.end() &&
            IsChunkComplete(parsed.chunkId, chunkCounts[parsed.chunkId])) {
            
            // Reconstruct and re-parse the complete message
            std::string completeMessage = GetReconstructedMessage(parsed.chunkId);
            std::string messageType = chunkTypes[parsed.chunkId];
            
            // Clear chunk storage
            ClearChunks(parsed.chunkId);
            
            // Prepend the original message type
            completeMessage = messageType + "|" + completeMessage;
            
            // Re-parse the complete message
            return ParseMessage(completeMessage);
        }
    }else if (parts[0] == "EC") {
        parsed.type = MessageType::EnemyClear;
        // This message doesn't have any additional data
    }

    return parsed;
}