#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <steam/steam_api.h>
#include <steam/isteamnetworking.h>
#include <string>
#include <unordered_map>
#include <functional>
#include "../utils/SteamHelpers.h"
#include "../utils/MessageHandler.h"


class Game;
class NetworkManager {
public:
    NetworkManager(Game* gameInstance);
    ~NetworkManager();

    void ReceiveMessages();
    bool SendMessage(CSteamID target, const std::string& msg);
    bool BroadcastMessage(const std::string& msg);
    void ProcessCallbacks();
    bool IsInitialized() const;
    bool IsLoaded() const { return isConnectedToHost; }
    void AcceptSession(CSteamID remoteID);
    const std::unordered_map<CSteamID, bool, CSteamIDHash>& GetConnectedClients() const { return m_connectedClients; }
    void SetMessageHandler(std::function<void(const std::string&, CSteamID)> handler);

    // Lobby-related functions
    void JoinLobbyFromNetwork(CSteamID lobby);
    const std::vector<std::pair<CSteamID, std::string>>& GetLobbyList() const { return lobbyList; }
    bool IsLobbyListUpdated() const { return lobbyListUpdated; }
    void ResetLobbyListUpdated() { lobbyListUpdated = false; }

    // New method for chat messages
    void SendChatMessage(CSteamID target, const std::string& message);
    CSteamID GetCurrentLobbyID() const { return m_currentLobbyID; } // Add getter
    // Getter for current lobby ID
    void SendConnectionMessageOnJoin(CSteamID hostID); // New method for retry logic

    void ResetLobbyState() {
        m_currentLobbyID = k_steamIDNil;
        m_connectedClients.clear();
        isConnectedToHost = false;
        m_pendingConnectionMessage = false;
        lobbyListUpdated = false;
        
        std::cout << "[NETWORK] Reset lobby state and cleared client connections" << std::endl;
    }
private:
    // Callback handlers
    void ProcessNetworkMessages(const std::string& msg, CSteamID sender);
    bool m_pendingConnectionMessage{false}; // Flag for retry
    std::string m_connectionMessage;        // Store message for retry
    CSteamID m_pendingHostID;               // Host to send to
    Game* game;
    ISteamNetworking* m_networking;
    bool isConnectedToHost{false};
    std::unordered_map<CSteamID, bool, CSteamIDHash> m_connectedClients;
    std::vector<std::pair<CSteamID, std::string>> lobbyList;
    bool lobbyListUpdated{false};
    std::function<void(const std::string&, CSteamID)> messageHandler;
    CSteamID m_currentLobbyID; // Store the current lobby ID

    // STEAM_CALLBACK for synchronous events
    STEAM_CALLBACK(NetworkManager, OnLobbyCreated, LobbyCreated_t, m_cbLobbyCreated);
    STEAM_CALLBACK(NetworkManager, OnGameLobbyJoinRequested, GameLobbyJoinRequested_t, m_cbGameLobbyJoinRequested);
    STEAM_CALLBACK(NetworkManager, OnLobbyEnter, LobbyEnter_t, m_cbLobbyEnter);
    STEAM_CALLBACK(NetworkManager, OnP2PSessionRequest, P2PSessionRequest_t, m_cbP2PSessionRequest);
    STEAM_CALLBACK(NetworkManager, OnP2PSessionConnectFail, P2PSessionConnectFail_t, m_cbP2PSessionConnectFail);
    STEAM_CALLBACK(NetworkManager, OnLobbyMatchList, LobbyMatchList_t, m_cbLobbyMatchList);
};

#endif // NETWORKMANAGER_H