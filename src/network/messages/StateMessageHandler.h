#ifndef STATE_MESSAGE_HANDLER_H
#define STATE_MESSAGE_HANDLER_H

#include <string>
#include <vector>

// Forward declarations
struct ParsedMessage;


class StateMessageHandler {
public:
    // Initialize and register state message handlers
    static void Initialize();
    
    // Message parsing functions
    static ParsedMessage ParseReadyStatusMessage(const std::vector<std::string>& parts);
    static ParsedMessage ParseStartGameMessage(const std::vector<std::string>& parts);
    static ParsedMessage ParseWaveStartMessage(const std::vector<std::string>& parts);
    static ParsedMessage ParseReturnToLobbyMessage(const std::vector<std::string>& parts);
static std::string FormatReturnToLobbyMessage(const std::string& hostID);
    // Message formatting functions
    static std::string FormatReadyStatusMessage(const std::string& steamID, bool isReady);
    static std::string FormatStartGameMessage(const std::string& hostID);
    static std::string FormatWaveStartMessage(int waveNumber, int enemyCount);
};

#endif // STATE_MESSAGE_HANDLER_H