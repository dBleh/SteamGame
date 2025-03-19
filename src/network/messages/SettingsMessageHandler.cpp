#include "SettingsMessageHandler.h"
#include "MessageHandler.h"
#include "../Client.h"
#include "../Host.h"
#include "../../core/Game.h"
#include "../../states/menu/LobbyState.h"
#include "../../states/PlayingState.h"
#include "../../states/GameSettingsManager.h"
#include <sstream>
#include <iostream>

void SettingsMessageHandler::Initialize() {
    // Register settings message types
    MessageHandler::RegisterMessageType("GS", 
        ParseSettingsUpdateMessage,
        [](Game& game, ClientNetwork& client, const ParsedMessage& parsed) {
            ProcessSettingsUpdateForClient(game, client, parsed);
        },
        [](Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender) {
            ProcessSettingsUpdateForHost(game, host, parsed, sender);
        });

    MessageHandler::RegisterMessageType("GSR", 
        ParseSettingsRequestMessage,
        [](Game& game, ClientNetwork& client, const ParsedMessage& parsed) {
            // Clients won't process settings requests
        },
        [](Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender) {
            ProcessSettingsRequestForHost(game, host, parsed, sender);
        });
}

ParsedMessage SettingsMessageHandler::ParseSettingsUpdateMessage(const std::vector<std::string>& parts) {
    ParsedMessage parsed;
    parsed.type = MessageType::SettingsUpdate;
    
    if (parts.size() >= 2) {
        parsed.chatMessage = parts[1]; // Reuse chatMessage field to store settings data
    }
    
    return parsed;
}

ParsedMessage SettingsMessageHandler::ParseSettingsRequestMessage(const std::vector<std::string>& parts) {
    ParsedMessage parsed;
    parsed.type = MessageType::SettingsRequest;
    
    if (parts.size() >= 2) {
        parsed.steamID = parts[1]; // The ID of the client requesting settings
    }
    
    return parsed;
}

std::string SettingsMessageHandler::FormatSettingsUpdateMessage(const std::string& settingsData) {
    std::ostringstream oss;
    oss << "GS|" << settingsData;
    return oss.str();
}

std::string SettingsMessageHandler::FormatSettingsRequestMessage() {
    std::ostringstream oss;
    oss << "GSR";
    return oss.str();
}
void SettingsMessageHandler::ProcessSettingsUpdateForClient(Game& game, ClientNetwork& client, const ParsedMessage& parsed) {
    // Client received settings update from host
    std::cout << "[Client] Received game settings update from host" << std::endl;
    
    if (!parsed.chatMessage.empty()) {
        // Update game settings
        if (game.GetGameSettingsManager()) {
            game.GetGameSettingsManager()->DeserializeSettings(parsed.chatMessage);
            game.GetGameSettingsManager()->ApplySettings();
            
            // Update the UI if we're in the lobby
            if (game.GetCurrentState() == GameState::Lobby) {
                // Use the state directly - don't cast to LobbyState
                State* state = game.GetState();
                if (state && dynamic_cast<LobbyState*>(state)) {
                    dynamic_cast<LobbyState*>(state)->RefreshSettingsUI();
                }
            }
        }
    }
}

void SettingsMessageHandler::ProcessSettingsUpdateForHost(Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender) {
    // Host doesn't normally receive settings updates unless from UI
    std::cout << "[Host] Received settings update request from client " 
              << SteamFriends()->GetFriendPersonaName(sender) << std::endl;
    
    // In case this is a UI change from the host itself
    if (sender == SteamUser()->GetSteamID() && !parsed.chatMessage.empty()) {
        if (game.GetGameSettingsManager()) {
            game.GetGameSettingsManager()->DeserializeSettings(parsed.chatMessage);
            game.GetGameSettingsManager()->ApplySettings();
            
            // Broadcast updated settings to all clients
            std::string msg = FormatSettingsUpdateMessage(
                game.GetGameSettingsManager()->SerializeSettings());
            game.GetNetworkManager().BroadcastMessage(msg);
        }
    }
}

void SettingsMessageHandler::ProcessSettingsRequestForHost(Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender) {
    // Client is requesting current settings, send them
    std::cout << "[Host] Received settings request from client " 
              << SteamFriends()->GetFriendPersonaName(sender) << std::endl;
    
    if (game.GetGameSettingsManager()) {
        std::string msg = FormatSettingsUpdateMessage(
            game.GetGameSettingsManager()->SerializeSettings());
        
        // Send only to the requesting client
        game.GetNetworkManager().SendMessage(sender, msg);
    }
}

std::string SettingsMessageHandler::FormatSettingsApplyMessage() {
    return "SA"; // Simple "Settings Apply" message
}

bool SettingsMessageHandler::ParseSettingsApplyMessage(const std::string& message) {
    return message == "SA";
}