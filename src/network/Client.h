#ifndef CLIENT_H
#define CLIENT_H

#include <SFML/Graphics.hpp>
#include <steam/steam_api.h>
#include <string>
#include <unordered_map>
#include <chrono>
#include "messages/MessageHandler.h"
#include "../entities/player/Player.h"
#include "../utils/SteamHelpers.h"
#include "../entities/player/PlayerManager.h"

class Game;
class PlayingState;

PlayingState* GetPlayingState(Game* game);

class ClientNetwork {
public:
    explicit ClientNetwork(Game* game, PlayerManager* manager);
    ~ClientNetwork();
    
    // Message processing
    void ProcessMessage(const std::string& msg, CSteamID sender);
    
    // Getters
    std::unordered_map<std::string, RemotePlayer>& GetRemotePlayers() { return remotePlayers; }
    CSteamID GetHostID() const { return hostID; }
    
    // Network communication methods
    void SendMovementUpdate(const sf::Vector2f& position);
    void SendChatMessage(const std::string& message);
    void SendConnectionMessage();
    void SendReadyStatus(bool isReady);
    void RequestEnemyState();
    
    // Regular update
    void Update();

    // Message handler methods
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
    void ProcessForceFieldZapMessage(Game& game, ClientNetwork& client, const ParsedMessage& parsed);
    void ProcessForceFieldUpdateMessage(Game& game, ClientNetwork& client, const ParsedMessage& parsed);
    void ProcessKillMessage(Game& game, ClientNetwork& client, const ParsedMessage& parsed);
    
private:
    // Core references
    Game* game;
    PlayerManager* playerManager;
    
    // Network state
    CSteamID hostID;
    std::unordered_map<std::string, RemotePlayer> remotePlayers;
    
    // Pending messages
    bool pendingConnectionMessage{false};
    std::string pendingMessage;
    std::string pendingReadyMessage;
    
    // Timers and intervals
    std::chrono::steady_clock::time_point lastSendTime;
    std::chrono::steady_clock::time_point m_lastValidationTime;
    std::chrono::steady_clock::time_point m_lastStateRequestTime;
    float m_validationRequestTimer = -1.0f;
    float m_periodicValidationTimer = 10.0f;
    float m_stateRequestCooldown = 2.0f;
    bool m_stateRequestPending = false;
    int m_consecutiveStateRequests = 0;
    
    // Constants
    static constexpr float SEND_INTERVAL = 0.1f;
    static constexpr float MIN_STATE_REQUEST_COOLDOWN = 2.0f;
    static constexpr float MAX_STATE_REQUEST_COOLDOWN = 30.0f;
    static constexpr int MAX_CONSECUTIVE_REQUESTS = 5;
};

#endif // CLIENT_H