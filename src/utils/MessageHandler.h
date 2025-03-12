#ifndef MESSAGEHANDLER_H
#define MESSAGEHANDLER_H

#include <string>
#include <SFML/Graphics.hpp>

enum class MessageType {
    Connection,
    Movement,
    Chat,    // New type for chat messages
    Unknown
};

struct ParsedMessage {
    MessageType type;
    std::string steamID;      // For Connection, Movement, and Chat
    std::string steamName;    // Only for Connection
    sf::Color color;          // Only for Connection
    sf::Vector2f position;    // Only for Movement
    std::string chatMessage;  // Only for Chat
};

class MessageHandler {
public:
    static std::string FormatConnectionMessage(const std::string& steamID, const std::string& steamName, const sf::Color& color);
    static std::string FormatMovementMessage(const std::string& steamID, const sf::Vector2f& position);
    static std::string FormatChatMessage(const std::string& steamID, const std::string& message); // New method
    static ParsedMessage ParseMessage(const std::string& msg);
};

#endif // MESSAGE_HANDLER_H