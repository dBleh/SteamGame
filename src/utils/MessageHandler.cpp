#include "MessageHandler.h"
#include <sstream>
#include <vector>

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
    
    for (const auto& enemy : enemyPositions) {
        oss << "|" << enemy.first << "," << enemy.second.x << "," << enemy.second.y;
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
        
        for (int i = 0; i < numEnemies && i + 2 < parts.size(); ++i) {
            std::istringstream enemyStream(parts[i + 2]);
            int id;
            float x, y;
            char comma;
            enemyStream >> id >> comma >> x >> comma >> y;
            parsed.enemyPositions.emplace_back(id, sf::Vector2f(x, y));
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
    }

    return parsed;
}