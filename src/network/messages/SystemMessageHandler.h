#ifndef SYSTEM_MESSAGE_HANDLER_H
#define SYSTEM_MESSAGE_HANDLER_H

#include <string>
#include <vector>

// Forward declarations
struct ParsedMessage;


class SystemMessageHandler {
public:
    // Initialize and register system message handlers
    static void Initialize();
    
    // Message parsing functions
    static ParsedMessage ParseChatMessage(const std::vector<std::string>& parts);
    static ParsedMessage ParseChunkStartMessage(const std::vector<std::string>& parts);
    static ParsedMessage ParseChunkPartMessage(const std::vector<std::string>& parts);
    static ParsedMessage ParseChunkEndMessage(const std::vector<std::string>& parts);
    
    // Message formatting functions
    static std::string FormatChatMessage(const std::string& steamID, const std::string& message);
    
    // Chunking functions
    static std::vector<std::string> ChunkMessage(const std::string& message, const std::string& messageType);
    static std::string FormatChunkStartMessage(const std::string& messageType, int totalChunks, const std::string& chunkId);
    static std::string FormatChunkPartMessage(const std::string& chunkId, int chunkNum, const std::string& chunkData);
    static std::string FormatChunkEndMessage(const std::string& chunkId);
    
    static void AddChunk(const std::string& chunkId, int chunkNum, const std::string& chunkData);
    static bool IsChunkComplete(const std::string& chunkId, int expectedChunks);
    static std::string GetReconstructedMessage(const std::string& chunkId);
    static void ClearChunks(const std::string& chunkId);
};

#endif // SYSTEM_MESSAGE_HANDLER_H