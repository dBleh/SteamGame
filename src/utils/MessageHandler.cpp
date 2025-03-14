#include "MessageHandler.h"
#include <sstream>
#include <vector>
#include "../Game.h"
#include "../states/PlayingState.h"
#include "../entities/EnemyManager.h"
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
    } catch (const std::exception& e) {
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

std::string MessageHandler::FormatEnemySpawnMessage(int enemyId, const sf::Vector2f& position) {
    std::ostringstream oss;
    oss << "ES|" << enemyId << "|" << position.x << "," << position.y;
    return oss.str();
}
std::string MessageHandler::FormatEnemyPositionsMessage(const std::vector<std::pair<int, sf::Vector2f>>& enemyPositions) {
    std::ostringstream oss;
    oss << "EP|" << enemyPositions.size();
    
    // Use default health value since health info wasn't provided
    for (const auto& enemy : enemyPositions) {
        int id = enemy.first;
        sf::Vector2f pos = enemy.second;
        int defaultHealth = 40; // Default max health
        
        oss << "|" << id << "," << pos.x << "," << pos.y << "," << defaultHealth;
    }
    
    return oss.str();
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
std::string MessageHandler::FormatEnemyHitMessage(int enemyId, int damage, bool killed, const std::string& shooterID) {
    std::ostringstream oss;
    oss << "EH|" << enemyId << "|" << damage << "|" << (killed ? "1" : "0") << "|" << shooterID;
    return oss.str();
}

std::string MessageHandler::FormatEnemyDeathMessage(int enemyId, const std::string& killerID, bool rewardKill) {
    std::ostringstream oss;
    oss << "ED|" << enemyId << "|" << killerID << "|" << (rewardKill ? "1" : "0");
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

std::string MessageHandler::FormatEnemyValidationRequestMessage() {
    return "EVR|";  // Simple message, no additional data needed
}

// Triangle enemy message formatting implementations
std::string MessageHandler::FormatTriangleEnemySpawnMessage(int enemyId, const sf::Vector2f& position) {
    std::ostringstream oss;
    oss << "TRIANGLE_SPAWN|" << enemyId << "|" 
        << position.x << "|" << position.y;
    return oss.str();
}

std::string MessageHandler::FormatTriangleEnemyPositionsMessage(const std::vector<std::tuple<int, sf::Vector2f, int>>& enemyData) {
    if (enemyData.empty()) {
        return "TRIANGLE_POS|0";
    }
    
    std::ostringstream oss;
    oss << "TRIANGLE_POS|" << enemyData.size();
    
    for (const auto& data : enemyData) {
        int id = std::get<0>(data);
        sf::Vector2f pos = std::get<1>(data);
        int health = std::get<2>(data);
        
        // Quantize position values to 1 decimal place to save bandwidth
        int quantizedX = static_cast<int>(pos.x * 10);
        int quantizedY = static_cast<int>(pos.y * 10);
        
        oss << "|" << id << "," 
            << quantizedX << "," 
            << quantizedY << "," 
            << health;
    }
    
    return oss.str();
}

std::string MessageHandler::FormatTriangleEnemyHitMessage(int enemyId, int damage, bool killed, const std::string& shooterID) {
    std::ostringstream oss;
    oss << "TRIANGLE_HIT|" << enemyId << "|" 
        << damage << "|" << (killed ? "1" : "0") << "|" << shooterID;
    return oss.str();
}

std::string MessageHandler::FormatTriangleEnemyDeathMessage(int enemyId, const std::string& killerID, bool rewardKill) {
    std::ostringstream oss;
    oss << "TRIANGLE_DEATH|" << enemyId << "|" 
        << killerID << "|" << (rewardKill ? "1" : "0");
    return oss.str();
}

std::string MessageHandler::FormatTriangleEnemyBatchSpawnMessage(const std::vector<std::tuple<int, sf::Vector2f, int>>& spawnData) {
    if (spawnData.empty()) {
        return "TRIANGLE_BATCH_SPAWN|0";
    }
    
    std::ostringstream oss;
    oss << "TRIANGLE_BATCH_SPAWN|" << spawnData.size();
    
    for (const auto& data : spawnData) {
        int id = std::get<0>(data);
        sf::Vector2f pos = std::get<1>(data);
        int health = std::get<2>(data);
        
        // Quantize position values
        int quantizedX = static_cast<int>(pos.x * 10);
        int quantizedY = static_cast<int>(pos.y * 10);
        
        oss << "|" << id << "," 
            << quantizedX << "," 
            << quantizedY << "," 
            << health;
    }
    
    return oss.str();
}

std::string MessageHandler::FormatTriangleEnemyFullListMessage(const std::vector<int>& validIds) {
    if (validIds.empty()) {
        return "TRIANGLE_FULL_LIST|0";
    }
    
    std::ostringstream oss;
    oss << "TRIANGLE_FULL_LIST|" << validIds.size();
    
    for (int id : validIds) {
        oss << "|" << id;
    }
    
    return oss.str();
}

// Parse message implementations for triangle enemies
ParsedMessage MessageHandler::ParseTriangleEnemySpawnMessage(const std::string& message) {
    ParsedMessage result;
    result.type = MessageType::TriangleEnemySpawn;
    
    std::istringstream ss(message);
    std::string token;
    
    // Skip "TRIANGLE_SPAWN" token
    std::getline(ss, token, '|');
    
    // Parse enemy ID
    std::getline(ss, token, '|');
    try {
        result.enemyId = std::stoi(token);
    } catch (...) {
        result.enemyId = 0;
    }
    
    // Parse position
    std::getline(ss, token, '|');
    float x = 0.f;
    try {
        x = std::stof(token);
    } catch (...) {}
    
    std::getline(ss, token, '|');
    float y = 0.f;
    try {
        y = std::stof(token);
    } catch (...) {}
    
    result.position = sf::Vector2f(x, y);
    
    return result;
}

ParsedMessage MessageHandler::ParseTriangleEnemyHitMessage(const std::string& message) {
    ParsedMessage result;
    result.type = MessageType::TriangleEnemyHit;
    
    std::istringstream ss(message);
    std::string token;
    
    // Skip "TRIANGLE_HIT" token
    std::getline(ss, token, '|');
    
    // Parse enemy ID
    std::getline(ss, token, '|');
    try {
        result.enemyId = std::stoi(token);
    } catch (...) {
        result.enemyId = 0;
    }
    
    // Parse damage
    std::getline(ss, token, '|');
    try {
        result.damage = std::stoi(token);
    } catch (...) {
        result.damage = 0;
    }
    
    // Parse if killed
    std::getline(ss, token, '|');
    result.killed = (token == "1");
    
    // Parse shooter ID
    std::getline(ss, token, '|');
    result.steamID = token;
    
    return result;
}

ParsedMessage MessageHandler::ParseTriangleEnemyDeathMessage(const std::string& message) {
    ParsedMessage result;
    result.type = MessageType::TriangleEnemyDeath;
    
    std::istringstream ss(message);
    std::string token;
    
    // Skip "TRIANGLE_DEATH" token
    std::getline(ss, token, '|');
    
    // Parse enemy ID
    std::getline(ss, token, '|');
    try {
        result.enemyId = std::stoi(token);
    } catch (...) {
        result.enemyId = 0;
    }
    
    // Parse killer ID
    std::getline(ss, token, '|');
    result.killerID = token;
    
    // Parse reward kill
    std::getline(ss, token, '|');
    result.rewardKill = (token == "1");
    
    return result;
}

ParsedMessage MessageHandler::ParseTriangleEnemyPositionsMessage(const std::string& message) {
    ParsedMessage result;
    result.type = MessageType::TriangleEnemyPositions;
    
    std::istringstream ss(message);
    std::string token;
    
    // Skip "TRIANGLE_POS" token
    std::getline(ss, token, '|');
    
    // Parse count
    std::getline(ss, token, '|');
    int count = 0;
    try {
        count = std::stoi(token);
    } catch (...) {}
    
    // Parse positions
    for (int i = 0; i < count; i++) {
        std::getline(ss, token, '|');
        std::istringstream posStream(token);
        std::string posToken;
        
        // Parse ID
        std::getline(posStream, posToken, ',');
        int id = 0;
        try {
            id = std::stoi(posToken);
        } catch (...) {}
        
        // Parse X coordinate (dequantize)
        std::getline(posStream, posToken, ',');
        int quantizedX = 0;
        try {
            quantizedX = std::stoi(posToken);
        } catch (...) {}
        float x = quantizedX / 10.0f;
        
        // Parse Y coordinate (dequantize)
        std::getline(posStream, posToken, ',');
        int quantizedY = 0;
        try {
            quantizedY = std::stoi(posToken);
        } catch (...) {}
        float y = quantizedY / 10.0f;
        
        // Parse health
        std::getline(posStream, posToken, ',');
        int health = 40; // Default health
        try {
            health = std::stoi(posToken);
        } catch (...) {}
        
        // Add to positions and health
        result.triangleEnemyPositions.emplace_back(id, sf::Vector2f(x, y), health);
        result.triangleEnemyHealths.emplace_back(id, health);
    }
    
    return result;
}

ParsedMessage MessageHandler::ParseTriangleEnemyBatchSpawnMessage(const std::string& message) {
    ParsedMessage result;
    result.type = MessageType::TriangleEnemyBatchSpawn;
    
    std::istringstream ss(message);
    std::string token;
    
    // Skip "TRIANGLE_BATCH_SPAWN" token
    std::getline(ss, token, '|');
    
    // Parse count
    std::getline(ss, token, '|');
    int count = 0;
    try {
        count = std::stoi(token);
    } catch (...) {}
    
    // Parse enemy data
    for (int i = 0; i < count; i++) {
        std::getline(ss, token, '|');
        std::istringstream enemyStream(token);
        std::string enemyToken;
        
        // Parse ID
        std::getline(enemyStream, enemyToken, ',');
        int id = 0;
        try {
            id = std::stoi(enemyToken);
        } catch (...) {}
        
        // Parse X coordinate (dequantize)
        std::getline(enemyStream, enemyToken, ',');
        int quantizedX = 0;
        try {
            quantizedX = std::stoi(enemyToken);
        } catch (...) {}
        float x = quantizedX / 10.0f;
        
        // Parse Y coordinate (dequantize)
        std::getline(enemyStream, enemyToken, ',');
        int quantizedY = 0;
        try {
            quantizedY = std::stoi(enemyToken);
        } catch (...) {}
        float y = quantizedY / 10.0f;
        
        // Parse health
        std::getline(enemyStream, enemyToken, ',');
        int health = 40; // Default health
        try {
            health = std::stoi(enemyToken);
        } catch (...) {}
        
        // Add to spawn data
        result.triangleEnemyPositions.emplace_back(id, sf::Vector2f(x, y), health);
    }
    
    return result;
}

ParsedMessage MessageHandler::ParseTriangleEnemyFullListMessage(const std::string& message) {
    ParsedMessage result;
    result.type = MessageType::TriangleEnemyFullList;
    
    std::istringstream ss(message);
    std::string token;
    
    // Skip "TRIANGLE_FULL_LIST" token
    std::getline(ss, token, '|');
    
    // Parse count
    std::getline(ss, token, '|');
    int count = 0;
    try {
        count = std::stoi(token);
    } catch (...) {}
    
    // Parse valid IDs
    for (int i = 0; i < count; i++) {
        std::getline(ss, token, '|');
        int id = 0;
        try {
            id = std::stoi(token);
        } catch (...) {}
        
        result.triangleValidEnemyIds.push_back(id);
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
    } else if (parts[0] == "EH" && parts.size() >= 5) {
        parsed.type = MessageType::EnemyHit;
        parsed.enemyId = std::stoi(parts[1]);
        parsed.damage = std::stoi(parts[2]);
        parsed.killed = (parts[3] == "1");
        parsed.steamID = parts[4];  // Shooter ID
    } else if (parts[0] == "ED" && parts.size() >= 4) {
        parsed.type = MessageType::EnemyDeath;
        parsed.enemyId = std::stoi(parts[1]);
        parsed.killerID = parts[2];
        parsed.rewardKill = (parts[3] == "1");
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
    }else if (parts[0] == "EP" && parts.size() >= 2) {
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
    }else if (parts[0] == "EV" && parts.size() >= 2) {
        parsed.type = MessageType::EnemyValidation;
        int numEnemies = std::stoi(parts[1]);
        
        for (int i = 0; i < numEnemies && i + 2 < parts.size(); ++i) {
            parsed.validEnemyIds.push_back(std::stoi(parts[i + 2]));
        }
    }
    else if (parts[0] == "EVR") {
        parsed.type = MessageType::EnemyValidationRequest;
    }else if (parts[0] == "TRIANGLE_SPAWN") {
        return ParseTriangleEnemySpawnMessage(msg);
    }
    else if (parts[0] == "TRIANGLE_HIT") {
        return ParseTriangleEnemyHitMessage(msg);
    }
    else if (parts[0] == "TRIANGLE_DEATH") {
        return ParseTriangleEnemyDeathMessage(msg);
    }
    else if (parts[0] == "TRIANGLE_POS") {
        return ParseTriangleEnemyPositionsMessage(msg);
    }
    else if (parts[0] == "TRIANGLE_BATCH_SPAWN") {
        return ParseTriangleEnemyBatchSpawnMessage(msg);
    }
    else if (parts[0] == "TRIANGLE_FULL_LIST") {
        return ParseTriangleEnemyFullListMessage(msg);
    }

    return parsed;
}