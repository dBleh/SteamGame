#ifndef MESSAGEHANDLER_H
#define MESSAGEHANDLER_H

#include <string>
#include <SFML/Graphics.hpp>

enum class MessageType {
    Connection,
    Movement,
    Chat,
    ReadyStatus, // New type
    Unknown
};

struct ParsedMessage {
    MessageType type;
    std::string steamID;
    std::string steamName;
    sf::Color color;
    sf::Vector2f position;
    std::string chatMessage;
    bool isReady; // New field for ReadyStatus
};

class MessageHandler {
public:
    static std::string FormatConnectionMessage(const std::string& steamID, const std::string& steamName, const sf::Color& color);
    static std::string FormatMovementMessage(const std::string& steamID, const sf::Vector2f& position);
    static std::string FormatChatMessage(const std::string& steamID, const std::string& message);
    static std::string FormatReadyStatusMessage(const std::string& steamID, bool isReady); // New method
    static ParsedMessage ParseMessage(const std::string& msg);
};

#endif // MESSAGE_HANDLER_H