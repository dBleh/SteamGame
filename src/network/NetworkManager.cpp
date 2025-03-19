#include "NetworkManager.h"
#include <steam/steam_api.h>
#include <iostream>
#include <cstring>
#include <sstream>
#include <vector>
#include <chrono>
#include "../core/Game.h"
#include "messages/MessageHandler.h"
#include "messages/PlayerMessageHandler.h"
#include "messages/EnemyMessageHandler.h"
#include "messages/StateMessageHandler.h"
#include "messages/SystemMessageHandler.h"
#include "../states/menu/LobbyCreationState.h"
#define GAME_ID "SteamGame_v1"
#define MAX_PACKET_SIZE 1024

NetworkManager::NetworkManager(Game* gameInstance)
    : game(gameInstance),
      m_cbLobbyCreated(this, &NetworkManager::OnLobbyCreated),
      m_cbGameLobbyJoinRequested(this, &NetworkManager::OnGameLobbyJoinRequested),
      m_cbLobbyEnter(this, &NetworkManager::OnLobbyEnter),
      m_cbP2PSessionRequest(this, &NetworkManager::OnP2PSessionRequest),
      m_cbP2PSessionConnectFail(this, &NetworkManager::OnP2PSessionConnectFail),
      m_cbLobbyMatchList(this, &NetworkManager::OnLobbyMatchList),
      m_currentLobbyID(k_steamIDNil) {
    if (!SteamAPI_Init()) {
        std::cerr << "[ERROR] Steam API initialization failed!" << std::endl;
        return;
    }
    m_networking = SteamNetworking();
    if (!m_networking) {
        std::cerr << "[ERROR] Could not get SteamNetworking interface!" << std::endl;
    }
    
    // Initialize the message handler system
    MessageHandler::Initialize();
}

NetworkManager::~NetworkManager() {
    SteamAPI_Shutdown();
}

void NetworkManager::ReceiveMessages() {
    if (!m_networking || !SteamUser()) return;

    uint32 msgSize;
    while (m_networking->IsP2PPacketAvailable(&msgSize)) {
        char buffer[MAX_PACKET_SIZE];
        CSteamID sender;
        if (msgSize > sizeof(buffer) - 1) {
            std::cerr << "[NETWORK] Packet too large: " << msgSize << "\n";
            continue;
        }
        if (m_networking->ReadP2PPacket(buffer, sizeof(buffer), &msgSize, &sender)) {
            buffer[msgSize] = '\0';
            std::string msg(buffer);
            CSteamID myID = SteamUser()->GetSteamID();
            if (sender == myID && msg.find("T|") != 0) { // Allow chat messages from self
                std::cout << "[NETWORK] Ignoring unexpected self-message: " << msg << "\n";
                continue;
            }

            if (m_connectedClients.find(sender) == m_connectedClients.end()) {
                if (m_networking->AcceptP2PSessionWithUser(sender)) {
                    m_connectedClients[sender] = true;
                    std::cout << "[NETWORK] Accepted new P2P session with " << sender.ConvertToUint64() << "\n";
                } else {
                    std::cerr << "[NETWORK] Failed to accept P2P session with " << sender.ConvertToUint64() << "\n";
                }
            }
            if (messageHandler) {
                messageHandler(msg, sender);
            }
        } else {
            std::cerr << "[NETWORK] Failed to read P2P packet of size " << msgSize << "\n";
        }
    }

    if (m_pendingConnectionMessage && m_pendingHostID != k_steamIDNil) {
        if (SendMessage(m_pendingHostID, m_connectionMessage)) {
            std::cout << "[NETWORK] Retry succeeded: " << m_connectionMessage << "\n";
            m_pendingConnectionMessage = false;
        } else {
            std::cout << "[NETWORK] Retry failed, will try again: " << m_connectionMessage << "\n";
        }
    }
}

bool NetworkManager::SendMessage(CSteamID target, const std::string& msg) {
    if (!m_networking || !SteamUser()) return false;
    uint32 msgSize = static_cast<uint32>(msg.size() + 1);
    bool success = m_networking->SendP2PPacket(target, msg.c_str(), msgSize, k_EP2PSendReliable);
    if (!success) {
        std::cout << "[NETWORK] Failed to send message to " << target.ConvertToUint64() << ": " << msg << "\n";
    }
    return success;
}

void NetworkManager::SendConnectionMessageOnJoin(CSteamID hostID) {
    CSteamID myID = SteamUser()->GetSteamID();
    std::string steamIDStr = std::to_string(myID.ConvertToUint64());
    std::string steamName = SteamFriends()->GetPersonaName();
    sf::Color playerColor = sf::Color::Blue;
    std::string connectMsg = PlayerMessageHandler::FormatConnectionMessage(steamIDStr, steamName, playerColor, false, false);

    if (SendMessage(hostID, connectMsg)) {
        std::cout << "[NETWORK] Sent connection message to host: " << connectMsg << "\n";
        m_pendingConnectionMessage = false;
    } else {
        std::cout << "[NETWORK] Failed to send connection message initially, queuing for retry: " << connectMsg << "\n";
        m_pendingConnectionMessage = true;
        m_connectionMessage = connectMsg;
        m_pendingHostID = hostID;
    }
}

bool NetworkManager::BroadcastMessage(const std::string& msg) {
    bool success = true;
    CSteamID myID = SteamUser()->GetSteamID();
    if (m_currentLobbyID == k_steamIDNil) {
        std::cout << "[NETWORK] No lobby to broadcast to\n";
        return false;
    }

    if (msg.size() > MAX_PACKET_SIZE) {
        std::string messageType = msg.substr(0, msg.find('|'));
        std::vector<std::string> chunks = SystemMessageHandler::ChunkMessage(msg.substr(msg.find('|') + 1), messageType);
                
        int numMembers = SteamMatchmaking()->GetNumLobbyMembers(m_currentLobbyID);
        for (int i = 0; i < numMembers; ++i) {
            CSteamID memberID = SteamMatchmaking()->GetLobbyMemberByIndex(m_currentLobbyID, i);
            if (memberID != myID) {
                for (const auto& chunk : chunks) {
                    if (!SendMessage(memberID, chunk)) {
                        std::cout << "[NETWORK] Failed to send chunk to " 
                                  << memberID.ConvertToUint64() << "\n";
                        success = false;
                    }
                }
            }
        }
        return success;
    }
    
    int numMembers = SteamMatchmaking()->GetNumLobbyMembers(m_currentLobbyID);
    for (int i = 0; i < numMembers; ++i) {
        CSteamID memberID = SteamMatchmaking()->GetLobbyMemberByIndex(m_currentLobbyID, i);
        if (memberID != myID) {
            if (!SendMessage(memberID, msg)) {
                std::cout << "[NETWORK] Failed to broadcast to " << memberID.ConvertToUint64() << "\n";
                success = false;
            }
        }
    }
    return success;
}

void NetworkManager::SendChatMessage(CSteamID target, const std::string& message) {
    CSteamID myID = SteamUser()->GetSteamID();
    std::string steamIDStr = std::to_string(myID.ConvertToUint64());
    std::string formattedMsg = SystemMessageHandler::FormatChatMessage(steamIDStr, message);
    SendMessage(target, formattedMsg);
}

void NetworkManager::ProcessCallbacks() {
    SteamAPI_RunCallbacks();
}

bool NetworkManager::IsInitialized() const {
    return SteamUser() && SteamUser()->BLoggedOn();
}

void NetworkManager::AcceptSession(CSteamID remoteID) {
    if (m_networking && m_networking->AcceptP2PSessionWithUser(remoteID)) {
        m_connectedClients[remoteID] = true;
    }
}

void NetworkManager::SetMessageHandler(std::function<void(const std::string&, CSteamID)> handler) {
    messageHandler = handler;
}

void NetworkManager::JoinLobbyFromNetwork(CSteamID lobby) {
    if (game->IsInLobby()) return;
    SteamAPICall_t call = SteamMatchmaking()->JoinLobby(lobby);
    if (call == k_uAPICallInvalid) {
        std::cerr << "[LOBBY] JoinLobby call failed immediately for "
                  << lobby.ConvertToUint64() << "\n";
        game->SetCurrentState(GameState::MainMenu);
    }
}

void NetworkManager::OnLobbyCreated(LobbyCreated_t* pParam) {
    const int maxRetries = 3;
    const float retryDelay = 2.0f;

    if (pParam->m_eResult != k_EResultOK) {
        std::cerr << "[LOBBY] Failed to create lobby. EResult=" << pParam->m_eResult << "\n";
        // Check if current state is LobbyCreationState
        if (auto* lobbyState = dynamic_cast<LobbyCreationState*>(game->GetState())) {
            if (pParam->m_eResult == k_EResultNoConnection && lobbyState->retryCount < maxRetries) {
                lobbyState->retryCount++;
                std::cout << "[LOBBY] No connection to Steam servers, retrying (" << lobbyState->retryCount << "/" << maxRetries << ") in " << retryDelay << " seconds...\n";
                game->GetHUD().updateText("statusText", "No connection to Steam, retrying (" + std::to_string(lobbyState->retryCount) + "/" + std::to_string(maxRetries) + ")...");
                lobbyState->retryTimer = retryDelay;
                lobbyState->m_creationInProgress = false; // Allow retry
                return;
            }
            // Reset creation flag even on final failure
            lobbyState->m_creationInProgress = false;
        }
        
        game->GetHUD().updateText("statusText", "Failed to create lobby. Please try again later.");
        game->SetCurrentState(GameState::MainMenu);
        return;
    }

    // Success case
    m_currentLobbyID = CSteamID(pParam->m_ulSteamIDLobby);
    SteamMatchmaking()->SetLobbyData(m_currentLobbyID, "name", game->GetLobbyNameInput().c_str());
    SteamMatchmaking()->SetLobbyData(m_currentLobbyID, "game_id", GAME_ID);
    CSteamID myID = SteamUser()->GetSteamID();
    std::string hostStr = std::to_string(myID.ConvertToUint64());
    SteamMatchmaking()->SetLobbyData(m_currentLobbyID, "host_steam_id", hostStr.c_str());
    SteamMatchmaking()->SetLobbyJoinable(m_currentLobbyID, true);
    
    game->SetInLobby(true);
    m_connectedClients[myID] = true;
    
    std::cout << "[LOBBY] Created lobby " << m_currentLobbyID.ConvertToUint64() << std::endl;
    
    // Reset creation flag on success
    if (auto* lobbyState = dynamic_cast<LobbyCreationState*>(game->GetState())) {
        lobbyState->m_creationInProgress = false;
    }
    
    if (game->GetCurrentState() == GameState::LobbyCreation) {
        game->SetCurrentState(GameState::Lobby);
    }
}

void NetworkManager::ResetLobbyState() {
    std::cout << "[NETWORK] Beginning lobby state reset..." << std::endl;
    
    messageHandler = nullptr;
    
    std::cout << "[NETWORK] Current lobby ID before reset: " 
              << (m_currentLobbyID == k_steamIDNil ? "None" : std::to_string(m_currentLobbyID.ConvertToUint64())) 
              << std::endl;
    
    std::cout << "[NETWORK] Connected clients before reset: " << m_connectedClients.size() << std::endl;
    
    m_currentLobbyID = k_steamIDNil;
    m_connectedClients.clear();
    isConnectedToHost = false;
    m_pendingConnectionMessage = false;
    m_pendingHostID = k_steamIDNil;
    m_connectionMessage = "";
    lobbyListUpdated = false;
    
    for (int i = 0; i < 10; i++) {
        SteamAPI_RunCallbacks();
    }
    
    std::cout << "[NETWORK] Lobby state reset complete" << std::endl;
}

void NetworkManager::OnLobbyEnter(LobbyEnter_t* pParam) {
    std::cout << "[NETWORK] OnLobbyEnter callback received - response: " 
              << pParam->m_EChatRoomEnterResponse << std::endl;
    
    if (pParam->m_EChatRoomEnterResponse != k_EChatRoomEnterResponseSuccess) {
        std::cerr << "[ERROR] Failed to enter lobby: " << pParam->m_EChatRoomEnterResponse << std::endl;
        game->SetCurrentState(GameState::MainMenu);
        return;
    }
    
    m_currentLobbyID = CSteamID(pParam->m_ulSteamIDLobby);
    std::cout << "Joined lobby " << m_currentLobbyID.ConvertToUint64() << std::endl;
    
    game->SetInLobby(true);
    
    CSteamID myID = SteamUser()->GetSteamID();
    CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(m_currentLobbyID);
    
    if (game->GetCurrentState() != GameState::Lobby) {
        std::cout << "[NETWORK] Transitioning to Lobby state" << std::endl;
        game->SetCurrentState(GameState::Lobby);
    } else {
        std::cout << "[NETWORK] Already in Lobby state - not transitioning" << std::endl;
    }
    
    if (myID != hostID) {
        SendConnectionMessageOnJoin(hostID);
    }
}

void NetworkManager::OnLobbyMatchList(LobbyMatchList_t* pParam) {
    lobbyList.clear();
    std::cout << "[LOBBY] Lobby list received, matching count: " << pParam->m_nLobbiesMatching << "\n";
    for (uint32 i = 0; i < pParam->m_nLobbiesMatching; ++i) {
        CSteamID lobbyID = SteamMatchmaking()->GetLobbyByIndex(i);
        const char* lobbyName = SteamMatchmaking()->GetLobbyData(lobbyID, "name");
        const char* gameID = SteamMatchmaking()->GetLobbyData(lobbyID, "game_id");
        std::cout << "[LOBBY] Lobby " << i << ": ID=" << lobbyID.ConvertToUint64() 
                  << ", Name=" << (lobbyName ? lobbyName : "null") 
                  << ", GameID=" << (gameID ? gameID : "null") << "\n";
        if (lobbyName && *lobbyName) {
            lobbyList.emplace_back(lobbyID, std::string(lobbyName));
        }
    }
    lobbyListUpdated = true;
    std::cout << "[LOBBY] Found " << lobbyList.size() << " lobbies with names\n";
}

void NetworkManager::OnGameLobbyJoinRequested(GameLobbyJoinRequested_t* pParam) {
    JoinLobbyFromNetwork(pParam->m_steamIDLobby);
}

void NetworkManager::OnP2PSessionRequest(P2PSessionRequest_t* pParam) {
    if (m_networking && m_networking->AcceptP2PSessionWithUser(pParam->m_steamIDRemote)) {
        m_connectedClients[pParam->m_steamIDRemote] = true;
        std::cout << "[NETWORK] Accepted P2P session with " << pParam->m_steamIDRemote.ConvertToUint64() << "\n";
        if (SteamUser()->GetSteamID() == SteamMatchmaking()->GetLobbyOwner(m_currentLobbyID)) {
            std::string lobbyName = SteamMatchmaking()->GetLobbyData(m_currentLobbyID, "name");
            SendChatMessage(pParam->m_steamIDRemote, "Welcome to " + lobbyName);
        }
    } else {
        std::cerr << "[NETWORK] Failed to accept P2P session with " << pParam->m_steamIDRemote.ConvertToUint64() << "\n";
    }
}

void NetworkManager::OnP2PSessionConnectFail(P2PSessionConnectFail_t* pParam) {
    std::cerr << "[ERROR] P2P session failed with " << pParam->m_steamIDRemote.ConvertToUint64() << ": " << pParam->m_eP2PSessionError << std::endl;
    m_connectedClients.erase(pParam->m_steamIDRemote);
    if (m_connectedClients.empty()) {
        isConnectedToHost = false;
    }
}

void NetworkManager::ProcessNetworkMessages(const std::string& msg, CSteamID sender) {
    // std::cout << "[NETWORK] Received: " << msg << " from " << sender.ConvertToUint64() << std::endl;
}