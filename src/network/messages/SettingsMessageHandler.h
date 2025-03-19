#ifndef SETTINGS_MESSAGE_HANDLER_H
#define SETTINGS_MESSAGE_HANDLER_H

#include <string>
#include <vector>
#include "MessageHandler.h"

// Forward declarations
struct ParsedMessage;

// Add the new message types for settings
class Game;
class GameSettingsManager;
class HostNetwork;
class ClientNetwork;
class CSteamID;

/**
 * @brief Handles network messages related to game settings.
 * 
 * This class processes settings updates sent from host to clients
 * and ensures all players have the same settings for game start.
 */
class SettingsMessageHandler {
public:
    // Initialize and register settings message handlers
    static void Initialize();
    
    // Message parsing functions
    static ParsedMessage ParseSettingsUpdateMessage(const std::vector<std::string>& parts);
    static ParsedMessage ParseSettingsRequestMessage(const std::vector<std::string>& parts);
    
    // Message formatting functions
    static std::string FormatSettingsUpdateMessage(const std::string& settingsData);
    static std::string FormatSettingsRequestMessage();
    
    // Helper functions for message processing
    static void ProcessSettingsUpdateForClient(Game& game, ClientNetwork& client, const ParsedMessage& parsed);
    static void ProcessSettingsUpdateForHost(Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender);
    static void ProcessSettingsRequestForHost(Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender);
    static std::string FormatSettingsApplyMessage();
    static bool ParseSettingsApplyMessage(const std::string& message);
};

#endif // SETTINGS_MESSAGE_HANDLER_H