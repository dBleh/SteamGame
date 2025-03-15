#ifndef CLIENT_H
#define CLIENT_H

#include <SFML/Graphics.hpp>
#include <steam/steam_api.h>
#include <string>
#include "../utils/MessageHandler.h"
#include "../entities/Player.h"
#include "../utils/SteamHelpers.h"
#include "../entities/PlayerManager.h"
#include "../entities/EnemyManager.h"

class Game;
class EnemyManager;
class PlayingState;

// Forward declaration helper (same as in Host.h)
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

private:
    // Message handlers
    void ProcessChatMessage(const ParsedMessage& parsed);
    void ProcessConnectionMessage(const ParsedMessage& parsed);
    void ProcessReadyStatusMessage(const ParsedMessage& parsed);
    void ProcessMovementMessage(const ParsedMessage& parsed);
    void ProcessBulletMessage(const ParsedMessage& parsed);
    void ProcessPlayerDeathMessage(const ParsedMessage& parsed);
    void ProcessPlayerRespawnMessage(const ParsedMessage& parsed);
    void ProcessEnemyPositionsMessage(const ParsedMessage& parsed);
    void ProcessEnemyValidationMessage(const ParsedMessage& parsed);
    void ProcessTriangleWaveStartMessage(const ParsedMessage& parsed);

    // Enemy-related message handlers - unified for all enemy types
    void ProcessEnemySpawnMessage(const ParsedMessage& parsed);
    void ProcessEnemyBatchSpawnMessage(const ParsedMessage& parsed);
    void ProcessEnemyHitMessage(const ParsedMessage& parsed);
    void ProcessEnemyDeathMessage(const ParsedMessage& parsed);
    void ProcessWaveStartMessage(const ParsedMessage& parsed);
    void ProcessWaveCompleteMessage(const ParsedMessage& parsed);

    Game* game;
    std::chrono::steady_clock::time_point lastSendTime;
    PlayerManager* playerManager;
    CSteamID hostID;
    bool pendingConnectionMessage{false};
    std::string pendingMessage;
    std::string pendingReadyMessage;
    std::unordered_map<std::string, RemotePlayer> remotePlayers;
    static constexpr float SEND_INTERVAL = 0.1f;
    float m_validationRequestTimer = -1.0f;
    std::chrono::steady_clock::time_point m_lastValidationTime = std::chrono::steady_clock::now();
    float m_periodicValidationTimer = 10.0f;
};

#endif // CLIENT_H