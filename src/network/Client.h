#ifndef CLIENT_H
#define CLIENT_H

#include <SFML/Graphics.hpp>
#include <steam/steam_api.h>
#include <string>
#include <unordered_map>
#include "messages/MessageHandler.h"
#include "../entities/Player.h"
#include "../utils/SteamHelpers.h"
#include "../entities/PlayerManager.h"
#include "../states/GameSettingsManager.h"
#include "messages/SettingsMessageHandler.h"  // Add settings message handler

class Game;
class PlayingState;

PlayingState* GetPlayingState(Game* game);

class ClientNetwork {
public:
    explicit ClientNetwork(Game* game, PlayerManager* manager);
    ~ClientNetwork();
    void ProcessMessage(const std::string& msg, CSteamID sender);
    std::unordered_map<std::string, RemotePlayer>& GetRemotePlayers() { return remotePlayers; }
    void SendMovementUpdate(const sf::Vector2f& position);
    void SendChatMessage(const std::string& message);
    void SendConnectionMessage();
    void SendReadyStatus(bool isReady);
    void Update();
    CSteamID GetHostID() const { return hostID; }
    
    // Settings management
    void ApplySettings();
    void RequestGameSettings();
    void ProcessSettingsUpdateMessage(Game& game, ClientNetwork& client, const ParsedMessage& parsed);

    // Updated handler signatures
    void ProcessConnectionMessage(Game& game, ClientNetwork& client, const ParsedMessage& parsed);
    void ProcessChatMessage(Game& game, ClientNetwork& client, const ParsedMessage& parsed);
    void ProcessReadyStatusMessage(Game& game, ClientNetwork& client, const ParsedMessage& parsed);
    void ProcessMovementMessage(Game& game, ClientNetwork& client, const ParsedMessage& parsed);
    void ProcessBulletMessage(Game& game, ClientNetwork& client, const ParsedMessage& parsed);
    void ProcessPlayerDeathMessage(Game& game, ClientNetwork& client, const ParsedMessage& parsed);
    void ProcessPlayerRespawnMessage(Game& game, ClientNetwork& client, const ParsedMessage& parsed);
    void ProcessStartGameMessage(Game& game, ClientNetwork& client, const ParsedMessage& parsed);
    void ProcessPlayerDamageMessage(Game& game, ClientNetwork& client, const ParsedMessage& parsed);
    void ProcessUnknownMessage(Game& game, ClientNetwork& client, const ParsedMessage& parsed);
    void ProcessKillMessage(Game& game, ClientNetwork& client, const ParsedMessage& parsed);
    
    bool HasReceivedInitialSettings() const { return m_initialSettingsReceived; }
private:
    Game* game;
    std::chrono::steady_clock::time_point lastSendTime;
    PlayerManager* playerManager;
    CSteamID hostID;
    bool pendingConnectionMessage{false};
    std::string pendingMessage;
    std::string pendingReadyMessage;
    std::string pendingSettingsRequest;
    std::unordered_map<std::string, RemotePlayer> remotePlayers;
    static constexpr float SEND_INTERVAL = 0.1f;
    float m_validationRequestTimer = -1.0f;
    std::chrono::steady_clock::time_point m_lastValidationTime = std::chrono::steady_clock::now();
    float m_periodicValidationTimer = 10.0f;
    float m_settingsRequestTimer = 1.0f;  // Request settings 1 second after connecting
    bool m_initialSettingsReceived = false;
    bool m_calledFromMessageHandler{false};

};

#endif // CLIENT_H