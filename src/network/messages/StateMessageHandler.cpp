#include "StateMessageHandler.h"
#include "MessageHandler.h"
#include "../Client.h"
#include "../Host.h"
#include "../../core/Game.h"
#include "../../states/PlayingState.h" 
#include "../../states/PlayingStateUI.h"  // Add this include for the PlayingStateUI class
#include <sstream>
#include <iostream>

void StateMessageHandler::Initialize() {
    // Register state message types
    MessageHandler::RegisterMessageType("R", 
                        ParseReadyStatusMessage,
                        [](Game& game, ClientNetwork& client, const ParsedMessage& parsed) {
                            client.ProcessReadyStatusMessage(game, client, parsed);
                        },
                        [](Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender) {
                            host.ProcessReadyStatusMessage(game, host, parsed, sender);
                        });

    MessageHandler::RegisterMessageType("SG", 
                        ParseStartGameMessage,
                        [](Game& game, ClientNetwork& client, const ParsedMessage& parsed) {
                            client.ProcessStartGameMessage(game, client, parsed);
                        },
                        [](Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender) {
                            host.ProcessStartGameMessage(game, host, parsed, sender);
                        });
    MessageHandler::RegisterMessageType("WS", 
                            ParseWaveStartMessage,
                            [](Game& game, ClientNetwork& client, const ParsedMessage& parsed) {
                                PlayingState* state = GetPlayingState(&game);
                                if (state && state->GetEnemyManager()) {
                                    // Just update the wave number, don't spawn enemies
                                    state->GetEnemyManager()->SetCurrentWave(parsed.waveNumber);
                                    
                                    // Update UI
                                    if (state->GetUI()) {
                                        state->GetUI()->UpdateWaveInfo();
                                    }
                                    
                                    std::cout << "[CLIENT] Received wave start message for wave " 
                                             << parsed.waveNumber << " with " << parsed.enemyCount << " enemies\n";
                                }
                            },
                            [](Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender) {
                                // Host initiates waves, not receives messages about them
                            });
}

// Ready status message parsing
ParsedMessage StateMessageHandler::ParseReadyStatusMessage(const std::vector<std::string>& parts) {
    ParsedMessage parsed;
    parsed.type = MessageType::ReadyStatus;
    if (parts.size() >= 3) {
        parsed.steamID = parts[1];
        parsed.isReady = (parts[2] == "1");
    }
    return parsed;
}

// Start game message parsing
ParsedMessage StateMessageHandler::ParseStartGameMessage(const std::vector<std::string>& parts) {
    ParsedMessage parsed;
    parsed.type = MessageType::StartGame;
    if (parts.size() >= 2) {
        parsed.steamID = parts[1];
    }
    return parsed;
}

// Wave start message parsing
ParsedMessage StateMessageHandler::ParseWaveStartMessage(const std::vector<std::string>& parts) {
    ParsedMessage parsed;
    parsed.type = MessageType::WaveStart;
    
    if (parts.size() >= 3) {
        parsed.waveNumber = std::stoi(parts[1]);
        parsed.enemyCount = std::stoi(parts[2]);
    }
    
    return parsed;
}

// Message formatting functions
std::string StateMessageHandler::FormatReadyStatusMessage(const std::string& steamID, 
                                                     bool isReady) {
    std::ostringstream oss;
    oss << "R|" << steamID << "|" << (isReady ? "1" : "0");
    return oss.str();
}

std::string StateMessageHandler::FormatStartGameMessage(const std::string& hostID) {
    std::ostringstream oss;
    oss << "SG|" << hostID;
    return oss.str();
}

std::string StateMessageHandler::FormatWaveStartMessage(int waveNumber, int enemyCount) {
    std::ostringstream oss;
    oss << "WS|" << waveNumber << "|" << enemyCount;
    return oss.str();
}