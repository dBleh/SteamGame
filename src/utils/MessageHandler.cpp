#include "MessageHandler.h"
#include <sstream>
#include <vector>
#include <iostream>
#include <cstdlib>

static std::vector<std::string> splitString(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(s);
    std::string token;
    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

std::string MessageHandler::FormatConnectionMessage(const std::string& steamID, const std::string& steamName, 
    const sf::Color& color, bool isReady, bool isHost) {
    std::ostringstream oss;
    oss << "C|" << steamID << "|" << steamName << "|" 
        << static_cast<int>(color.r) << "," << static_cast<int>(color.g) << "," << static_cast<int>(color.b)
        << "|" << (isReady ? "1" : "0") << "|" << (isHost ? "1" : "0");
    return oss.str();
}

std::string MessageHandler::FormatMovementMessage(const std::string& steamID, const sf::Vector2f& position) {
    std::stringstream ss;
    ss << "P|" << steamID << "|" << position.x << "|" << position.y;
    return ss.str();
}

std::string MessageHandler::FormatChatMessage(const std::string& steamID, const std::string& message) {
    std::stringstream ss;
    ss << "T|" << steamID << "|" << message;
    return ss.str();
}

std::string MessageHandler::FormatReadyStatusMessage(const std::string& steamID, bool isReady) {
    std::stringstream ss;
    ss << "R|" << steamID << "|" << (isReady ? "1" : "0");
    return ss.str();
}

ParsedMessage MessageHandler::ParseMessage(const std::string& msg) {
    ParsedMessage parsed;
    auto parts = splitString(msg, '|');
    if (parts.empty()) {
        parsed.type = MessageType::Unknown;
        return parsed;
    }
    
    char msgType = parts[0][0];
    if (msgType == 'C') {
        parsed.type = MessageType::Connection;
        if (parts.size() >= 6) {  // Need 6 parts: C|steamID|steamName|color|isReady|isHost
            parsed.steamID = parts[1];
            parsed.steamName = parts[2];
            
            auto colorParts = splitString(parts[3], ',');
            if (colorParts.size() >= 3) {
                parsed.color = sf::Color(
                    std::stoi(colorParts[0]), 
                    std::stoi(colorParts[1]), 
                    std::stoi(colorParts[2])
                );
            }
            parsed.isReady = (parts[4] == "1");  // Convert string "1" or "0" to bool
            parsed.isHost = (parts[5] == "1");   // Convert string "1" or "0" to bool
        }
    } else if (msgType == 'P') {
        parsed.type = MessageType::Movement;
        if (parts.size() >= 4) {
            parsed.steamID = parts[1];
            parsed.position = sf::Vector2f(std::stof(parts[2]), std::stof(parts[3]));
        }
    } else if (msgType == 'T') {
        parsed.type = MessageType::Chat;
        if (parts.size() >= 3) {
            parsed.steamID = parts[1];
            parsed.chatMessage = parts[2];
        }
    } else if (msgType == 'R') {
        parsed.type = MessageType::ReadyStatus;
        std::cout << "User wants to start game" << std::endl;
        if (parts.size() >= 3) {
            parsed.steamID = parts[1];
            parsed.isReady = (parts[2] == "1");  // Convert string "1" or "0" to bool
        }
    } else {
        parsed.type = MessageType::Unknown;
    }
    
    return parsed;
}