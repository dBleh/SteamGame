#ifndef HOST_H
#define HOST_H

#include <SFML/Graphics.hpp>
#include <steam/steam_api.h>
#include <string>
#include <unordered_map>
#include "messages/MessageHandler.h"
#include "../entities/Player.h"
#include "../utils/SteamHelpers.h"
#include "../entities/PlayerManager.h"
#include "../states/GameSettingsManager.h"
#include "messages/SettingsMessageHandler.h" 
class Game;
class EnemyManager;
class PlayingState;

inline PlayingState* GetPlayingState(Game* game);

class HostNetwork {
public:
    explicit HostNetwork(Game* game, PlayerManager* manager);
    ~HostNetwork();
    void ProcessMessage(const std::string& msg, CSteamID sender);
    std::unordered_map<std::string, RemotePlayer>& GetRemotePlayers() { return remotePlayers; }
    void BroadcastFullPlayerList();
    void BroadcastPlayersList();
    void ProcessChatMessage(const std::string& message, CSteamID sender);
    void Update();
    
    // Settings management
    void BroadcastGameSettings();
    void ProcessGameSettingsMessage(Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender);
    void ApplySettings();

    // Updated handler signatures
    void ProcessConnectionMessage(Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender);
    void ProcessMovementMessage(Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender);
    void ProcessChatMessageParsed(Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender);
    void ProcessReadyStatusMessage(Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender);
    void ProcessBulletMessage(Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender);
    void ProcessPlayerDeathMessage(Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender);
    void ProcessPlayerRespawnMessage(Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender);
    void ProcessStartGameMessage(Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender);
    void ProcessPlayerDamageMessage(Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender);
    void ProcessUnknownMessage(Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender);
    void ProcessForceFieldZapMessage(Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender);
    void ProcessForceFieldUpdateMessage(Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender);
    void ProcessKillMessage(Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender);
    void SendReturnToLobbyCommand();
private:
    Game* game;
    PlayerManager* playerManager;
    std::unordered_map<std::string, RemotePlayer> remotePlayers;
    std::chrono::steady_clock::time_point lastBroadcastTime;
    std::chrono::steady_clock::time_point lastSettingsBroadcastTime;
    static constexpr float BROADCAST_INTERVAL = 0.1f;
    static constexpr float SETTINGS_BROADCAST_INTERVAL = 5.0f; // Broadcast settings every 5 seconds
};

#endif // HOST_H