#ifndef HOST_H
#define HOST_H

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
class TriangleEnemyManager;
class HostNetwork {
public:
    explicit HostNetwork(Game* game, PlayerManager* manager);
    ~HostNetwork();
    void ProcessMessage(const std::string& msg, CSteamID sender);
    std::unordered_map<std::string, RemotePlayer>& GetRemotePlayers() { return remotePlayers; }
    void BroadcastFullPlayerList();
    void BroadcastPlayersList();
    void ProcessChatMessage(const std::string& message, CSteamID sender);
    void Update(); // Time-based
    void ProcessBulletMessage(const ParsedMessage& parsed);
    
private:
    // Message handlers
    void ProcessConnectionMessage(const ParsedMessage& parsed);
    void ProcessMovementMessage(const ParsedMessage& parsed, CSteamID sender);
    void ProcessReadyStatusMessage(const ParsedMessage& parsed);
    void ProcessPlayerDeathMessage(const ParsedMessage& parsed);
    void ProcessPlayerRespawnMessage(const ParsedMessage& parsed);
    
    // Enemy-related message handlers - unified for all enemy types
    void ProcessEnemyHitMessage(const ParsedMessage& parsed);
    void ProcessEnemyDeathMessage(const ParsedMessage& parsed);
    void ProcessWaveStartMessage(const ParsedMessage& parsed);
    void ProcessWaveCompleteMessage(const ParsedMessage& parsed);
    void ProcessEnemyValidationRequestMessage(const ParsedMessage& parsed);

    Game* game;
    PlayerManager* playerManager;
    std::unordered_map<std::string, RemotePlayer> remotePlayers;
    std::chrono::steady_clock::time_point lastBroadcastTime;
    static constexpr float BROADCAST_INTERVAL = 0.1f; // Match client send rate
};

#endif // HOST_H