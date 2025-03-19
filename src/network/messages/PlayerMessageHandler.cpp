#include "PlayerMessageHandler.h"
#include "MessageHandler.h"
#include "../Client.h"
#include "../Host.h"
#include "../../core/Game.h"
#include <sstream>
#include <iostream>

void PlayerMessageHandler::Initialize() {
    // Register message types with their parsers and handlers
    MessageHandler::RegisterMessageType("C", 
                        ParseConnectionMessage,
                        [](Game& game, ClientNetwork& client, const ParsedMessage& parsed) {
                            client.ProcessConnectionMessage(game, client, parsed);
                        },
                        [](Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender) {
                            host.ProcessConnectionMessage(game, host, parsed, sender);
                        });
    
    MessageHandler::RegisterMessageType("M", 
                        ParseMovementMessage,
                        [](Game& game, ClientNetwork& client, const ParsedMessage& parsed) {
                            client.ProcessMovementMessage(game, client, parsed);
                        },
                        [](Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender) {
                            host.ProcessMovementMessage(game, host, parsed, sender);
                        });
    
    MessageHandler::RegisterMessageType("B", 
                        ParseBulletMessage,
                        [](Game& game, ClientNetwork& client, const ParsedMessage& parsed) {
                            client.ProcessBulletMessage(game, client, parsed);
                        },
                        [](Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender) {
                            host.ProcessBulletMessage(game, host, parsed, sender);
                        });

    MessageHandler::RegisterMessageType("D", 
                        ParsePlayerDeathMessage,
                        [](Game& game, ClientNetwork& client, const ParsedMessage& parsed) {
                            client.ProcessPlayerDeathMessage(game, client, parsed);
                        },
                        [](Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender) {
                            host.ProcessPlayerDeathMessage(game, host, parsed, sender);
                        });

    MessageHandler::RegisterMessageType("RS", 
                        ParsePlayerRespawnMessage,
                        [](Game& game, ClientNetwork& client, const ParsedMessage& parsed) {
                            client.ProcessPlayerRespawnMessage(game, client, parsed);
                        },
                        [](Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender) {
                            host.ProcessPlayerRespawnMessage(game, host, parsed, sender);
                        });

    MessageHandler::RegisterMessageType("PD", 
                        ParsePlayerDamageMessage,
                        [](Game& game, ClientNetwork& client, const ParsedMessage& parsed) {
                            client.ProcessPlayerDamageMessage(game, client, parsed);
                        },
                        [](Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender) {
                            host.ProcessPlayerDamageMessage(game, host, parsed, sender);
                        });
                        
    MessageHandler::RegisterMessageType("KL", 
                            ParseKillMessage,
                            [](Game& game, ClientNetwork& client, const ParsedMessage& parsed) {
                                client.ProcessKillMessage(game, client, parsed);
                            },
                            [](Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender) {
                                host.ProcessKillMessage(game, host, parsed, sender);
                            });
    MessageHandler::RegisterMessageType("KILL", 
                                ParseKillMessage,
                                [](Game& game, ClientNetwork& client, const ParsedMessage& parsed) {
                                    client.ProcessKillMessage(game, client, parsed);
                                },
                                [](Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender) {
                                    host.ProcessKillMessage(game, host, parsed, sender);
                                });                        
    MessageHandler::RegisterMessageType("FZ", 
                        ParseForceFieldZapMessage,
                        [](Game& game, ClientNetwork& client, const ParsedMessage& parsed) {
                            // Handle force field zap on client
                            client.ProcessForceFieldZapMessage(game, client, parsed);
                        },
                        [](Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender) {
                            // Handle force field zap on host
                            host.ProcessForceFieldZapMessage(game, host, parsed, sender);
                        });
                        
    MessageHandler::RegisterMessageType("FFU", 
                      ParseForceFieldUpdateMessage,
                      [](Game& game, ClientNetwork& client, const ParsedMessage& parsed) {
                          // Handle force field update on client
                          client.ProcessForceFieldUpdateMessage(game, client, parsed);
                      },
                      [](Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender) {
                          // Handle force field update on host
                          host.ProcessForceFieldUpdateMessage(game, host, parsed, sender);
                      });
}

// Connection message parsing
ParsedMessage PlayerMessageHandler::ParseConnectionMessage(const std::vector<std::string>& parts) {
    ParsedMessage parsed;
    parsed.type = MessageType::Connection;
    if (parts.size() >= 6) {
        parsed.steamID = parts[1];
        parsed.steamName = parts[2];
        std::istringstream colorStream(parts[3]);
        int r, g, b;
        char comma;
        colorStream >> r >> comma >> g >> comma >> b;
        parsed.color = sf::Color(r, g, b);
        parsed.isReady = (parts[4] == "1");
        parsed.isHost = (parts[5] == "1");
    }
    return parsed;
}

// Movement message parsing
ParsedMessage PlayerMessageHandler::ParseMovementMessage(const std::vector<std::string>& parts) {
    ParsedMessage parsed;
    parsed.type = MessageType::Movement;
    if (parts.size() >= 3) {
        parsed.steamID = parts[1];
        std::istringstream posStream(parts[2]);
        float x, y;
        char comma;
        posStream >> x >> comma >> y;
        parsed.position = sf::Vector2f(x, y);
    }
    return parsed;
}

// Bullet message parsing
ParsedMessage PlayerMessageHandler::ParseBulletMessage(const std::vector<std::string>& parts) {
    ParsedMessage parsed;
    parsed.type = MessageType::Bullet;
    if (parts.size() >= 5) {
        parsed.steamID = parts[1];
        std::istringstream posStream(parts[2]);
        float px, py;
        char comma;
        posStream >> px >> comma >> py;
        parsed.position = sf::Vector2f(px, py);
        std::istringstream dirStream(parts[3]);
        float dx, dy;
        dirStream >> dx >> comma >> dy;
        parsed.direction = sf::Vector2f(dx, dy);
        parsed.velocity = std::stof(parts[4]);
    }
    return parsed;
}

// Player death message parsing
ParsedMessage PlayerMessageHandler::ParsePlayerDeathMessage(const std::vector<std::string>& parts) {
    ParsedMessage parsed;
    parsed.type = MessageType::PlayerDeath;
    if (parts.size() >= 3) {
        parsed.steamID = parts[1];
        parsed.killerID = parts[2];
    }
    return parsed;
}

// Player respawn message parsing
ParsedMessage PlayerMessageHandler::ParsePlayerRespawnMessage(const std::vector<std::string>& parts) {
    ParsedMessage parsed;
    parsed.type = MessageType::PlayerRespawn;
    if (parts.size() >= 3) {
        parsed.steamID = parts[1];
        std::istringstream posStream(parts[2]);
        float x, y;
        char comma;
        posStream >> x >> comma >> y;
        parsed.position = sf::Vector2f(x, y);
    }
    return parsed;
}

// Player damage message parsing
ParsedMessage PlayerMessageHandler::ParsePlayerDamageMessage(const std::vector<std::string>& parts) {
    ParsedMessage parsed;
    parsed.type = MessageType::PlayerDamage;
    if (parts.size() >= 4) {
        parsed.steamID = parts[1];
        parsed.damage = std::stoi(parts[2]);
        parsed.enemyId = std::stoi(parts[3]);
    }
    return parsed;
}

// Kill message parsing
ParsedMessage PlayerMessageHandler::ParseKillMessage(const std::vector<std::string>& parts) {
    ParsedMessage parsed;
    parsed.type = MessageType::Kill;
    if (parts.size() >= 3) {
        parsed.steamID = parts[1];  // Killer ID
        parsed.enemyId = std::stoi(parts[2]);  // Enemy ID
    }
    return parsed;
}

// Force Field Zap message parsing
ParsedMessage PlayerMessageHandler::ParseForceFieldZapMessage(const std::vector<std::string>& parts) {
    ParsedMessage parsed;
    parsed.type = MessageType::ForceFieldZap;
    
    if (parts.size() >= 4) {
        parsed.steamID = parts[1];  // Player ID who owns the force field
        parsed.enemyId = std::stoi(parts[2]);  // Enemy ID that was zapped
        parsed.damage = std::stof(parts[3]);  // Damage applied
    }
    
    return parsed;
}

// Force Field Update message parsing
ParsedMessage PlayerMessageHandler::ParseForceFieldUpdateMessage(const std::vector<std::string>& parts) {
    ParsedMessage parsed;
    parsed.type = MessageType::ForceFieldUpdate;
    
    if (parts.size() >= 9) {
        parsed.steamID = parts[1];                   // Player ID
        parsed.ffRadius = std::stof(parts[2]);       // Force field radius
        parsed.ffDamage = std::stof(parts[3]);       // Force field damage
        parsed.ffCooldown = std::stof(parts[4]);     // Force field cooldown
        parsed.ffChainTargets = std::stoi(parts[5]); // Chain lightning targets
        parsed.ffType = std::stoi(parts[6]);         // Field type as integer
        parsed.ffPowerLevel = std::stoi(parts[7]);   // Power level
        parsed.ffChainEnabled = (parts[8] == "1");   // Chain lightning enabled flag
    }
    
    return parsed;
}

// Message formatting functions
std::string PlayerMessageHandler::FormatConnectionMessage(const std::string& steamID, 
                                                     const std::string& steamName, 
                                                     const sf::Color& color, 
                                                     bool isReady, 
                                                     bool isHost) {
    std::ostringstream oss;
    oss << "C|" << steamID << "|" << steamName << "|" 
        << static_cast<int>(color.r) << "," 
        << static_cast<int>(color.g) << "," 
        << static_cast<int>(color.b) << "|" 
        << (isReady ? "1" : "0") << "|" 
        << (isHost ? "1" : "0");
    return oss.str();
}

std::string PlayerMessageHandler::FormatMovementMessage(const std::string& steamID, 
                                                    const sf::Vector2f& position) {
    std::ostringstream oss;
    oss << "M|" << steamID << "|" << position.x << "," << position.y;
    return oss.str();
}

std::string PlayerMessageHandler::FormatBulletMessage(const std::string& shooterID, 
                                                 const sf::Vector2f& position, 
                                                 const sf::Vector2f& direction, 
                                                 float velocity) {
    std::ostringstream oss;
    oss << "B|" << shooterID << "|" << position.x << "," << position.y << "|" 
        << direction.x << "," << direction.y << "|" << velocity;
    return oss.str();
}

std::string PlayerMessageHandler::FormatPlayerDeathMessage(const std::string& playerID, 
                                                      const std::string& killerID) {
    std::ostringstream oss;
    oss << "D|" << playerID << "|" << killerID;
    return oss.str();
}

std::string PlayerMessageHandler::FormatPlayerRespawnMessage(const std::string& playerID, 
                                                        const sf::Vector2f& position) {
    std::ostringstream oss;
    oss << "RS|" << playerID << "|" << position.x << "," << position.y;
    return oss.str();
}

std::string PlayerMessageHandler::FormatPlayerDamageMessage(const std::string& playerID, 
                                                       int damage, 
                                                       int enemyId) {
    std::ostringstream oss;
    oss << "PD|" << playerID << "|" << damage << "|" << enemyId;
    return oss.str();
}

std::string PlayerMessageHandler::FormatKillMessage(const std::string& playerID, int enemyId) {
    std::ostringstream oss;
    oss << "KL|" << playerID << "|" << enemyId;  // Change "KILL" to "KL"
    return oss.str();
}

std::string PlayerMessageHandler::FormatKillMessage(const std::string& playerID, int enemyId, uint32_t sequence) {
    std::ostringstream oss;
    oss << "KL|" << playerID << "|" << enemyId << "|" << sequence;  // Change "KILL" to "KL"
    return oss.str();
}

std::string PlayerMessageHandler::FormatForceFieldZapMessage(const std::string& playerID, int enemyId, float damage) {
    std::ostringstream oss;
    oss << "FZ|" << playerID << "|" << enemyId << "|" << damage;
    return oss.str();
}

std::string PlayerMessageHandler::FormatForceFieldUpdateMessage(
    const std::string& playerID,
    float radius,
    float damage,
    float cooldown,
    int chainTargets,
    int fieldType,
    int powerLevel,
    bool chainEnabled)
{
    std::ostringstream oss;
    oss << "FFU|" << playerID 
        << "|" << radius 
        << "|" << damage 
        << "|" << cooldown 
        << "|" << chainTargets 
        << "|" << fieldType 
        << "|" << powerLevel
        << "|" << (chainEnabled ? "1" : "0");
    return oss.str();
}