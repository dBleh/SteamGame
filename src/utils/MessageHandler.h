#ifndef MESSAGE_HANDLER_H
#define MESSAGE_HANDLER_H

#include <string>
#include <SFML/Graphics.hpp>

enum class MessageType {
    Connection,
    Movement,
    Chat,
    ReadyStatus,
    Bullet  // New type
};

struct ParsedMessage {
    MessageType type;
    std::string steamID;
    std::string steamName;
    sf::Vector2f position;
    sf::Color color;
    std::string chatMessage;
    bool isReady;
    bool isHost;
    sf::Vector2f direction;  // For Bullet
    float velocity;          // For Bullet
};

class MessageHandler {
public:
    static std::string FormatConnectionMessage(const std::string& steamID, const std::string& steamName, const sf::Color& color, bool isReady, bool isHost);
    static std::string FormatMovementMessage(const std::string& steamID, const sf::Vector2f& position);
    static std::string FormatChatMessage(const std::string& steamID, const std::string& message);
    static std::string FormatReadyStatusMessage(const std::string& steamID, bool isReady);
    static std::string FormatBulletMessage(const std::string& shooterID, const sf::Vector2f& position, const sf::Vector2f& direction, float velocity);
    static ParsedMessage ParseMessage(const std::string& msg);
};

#endif // MESSAGE_HANDLER_H