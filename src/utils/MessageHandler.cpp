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
    std::ostringstream oss;
    oss << "B|" << shooterID << "|" << position.x << "," << position.y << "|" 
        << direction.x << "," << direction.y << "|" << velocity;
    return oss.str();
}

std::string MessageHandler::FormatPlayerDeathMessage(const std::string& playerID, const std::string& killerID) {
    std::ostringstream oss;
    oss << "D|" << playerID << "|" << killerID;
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

    return parsed;
}