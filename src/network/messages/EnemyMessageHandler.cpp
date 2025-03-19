#include "EnemyMessageHandler.h"
#include "MessageHandler.h"
#include "../Client.h"
#include "../Host.h"
#include "../../core/Game.h"
#include "../../states/PlayingState.h"
#include <sstream>
#include <iostream>

void EnemyMessageHandler::Initialize() {
    // Register enemy message types
    MessageHandler::RegisterMessageType("EA", 
        ParseEnemyAddMessage,
        [](Game& game, ClientNetwork& client, const ParsedMessage& parsed) {
            PlayingState* state = GetPlayingState(&game);
            if (state && state->GetEnemyManager()) {
                state->GetEnemyManager()->RemoteAddEnemy(parsed.enemyId, parsed.enemyType, parsed.position, parsed.health);
            }
        },
        [](Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender) {
            // Host should be the one creating enemies, not receiving
        });

    MessageHandler::RegisterMessageType("ER", 
        ParseEnemyRemoveMessage,
        [](Game& game, ClientNetwork& client, const ParsedMessage& parsed) {
            PlayingState* state = GetPlayingState(&game);
            if (state && state->GetEnemyManager()) {
                state->GetEnemyManager()->RemoteRemoveEnemy(parsed.enemyId);
            }
        },
        [](Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender) {
            // Client notifying host of enemy removal
            PlayingState* state = GetPlayingState(&game);
            if (state && state->GetEnemyManager()) {
                state->GetEnemyManager()->RemoveEnemy(parsed.enemyId);
            }
        });

    MessageHandler::RegisterMessageType("ED", 
        ParseEnemyDamageMessage,
        [](Game& game, ClientNetwork& client, const ParsedMessage& parsed) {
            PlayingState* state = GetPlayingState(&game);
            if (state && state->GetEnemyManager()) {
                EnemyManager* enemyManager = state->GetEnemyManager();
                if (parsed.enemyId >= 0) {
                    auto enemy = enemyManager->FindEnemy(parsed.enemyId);
                    if (enemy) {
                        enemy->SetHealth(parsed.health);
                        if (parsed.health <= 0) {
                            enemyManager->RemoteRemoveEnemy(parsed.enemyId);
                        }
                    }
                }
            }
        },
        [](Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender) {
            // Client informing host of damage to enemy
            PlayingState* state = GetPlayingState(&game);
            if (state && state->GetEnemyManager()) {
                state->GetEnemyManager()->InflictDamage(parsed.enemyId, parsed.damage);
            }
        });

    MessageHandler::RegisterMessageType("ES", 
        ParseEnemyStateMessage,
        [](Game& game, ClientNetwork& client, const ParsedMessage& parsed) {
            // Handle enemy state
            PlayingState* state = GetPlayingState(&game);
            if (state && state->GetEnemyManager()) {
                for (size_t i = 0; i < parsed.enemyIds.size(); ++i) {
                    int id = parsed.enemyIds[i];
                    auto enemyManager = state->GetEnemyManager();
                    auto enemy = enemyManager->FindEnemy(id);
                    
                    if (enemy) {
                        // Update existing enemy
                        enemy->SetPosition(parsed.enemyPositions[i]);
                    } else if (i < parsed.enemyTypes.size() && i < parsed.enemyHealths.size()) {
                        // Create new enemy
                        EnemyType type = static_cast<EnemyType>(parsed.enemyTypes[i]);
                        enemyManager->RemoteAddEnemy(id, type, parsed.enemyPositions[i], 
                                                   parsed.enemyHealths[i]);
                    }
                }
            }
        },
        [](Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender) {
            // Host typically sends state, not receives it
        });

    MessageHandler::RegisterMessageType("ESR", 
        ParseEnemyStateRequestMessage,
        [](Game& game, ClientNetwork& client, const ParsedMessage& parsed) {
            // Client requesting enemy state
        },
        [](Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender) {
            // Host handling state request - should send current enemy states
            PlayingState* state = GetPlayingState(&game);
            if (state && state->GetEnemyManager()) {
                // Implementation would send current enemy state to the requesting client
            }
        });
        MessageHandler::RegisterMessageType("EP", 
            ParseEnemyPositionUpdateMessage,
            [](Game& game, ClientNetwork& client, const ParsedMessage& parsed) {
                PlayingState* state = GetPlayingState(&game);
                if (state && state->GetEnemyManager()) {
                    EnemyManager* enemyManager = state->GetEnemyManager();
                    for (size_t i = 0; i < parsed.enemyIds.size(); ++i) {
                        int id = parsed.enemyIds[i];
                        auto enemy = enemyManager->FindEnemy(id);
                        
                        if (enemy) {
                            // Update existing enemy position
                            sf::Vector2f velocity = i < parsed.enemyVelocities.size() ? 
                                                  parsed.enemyVelocities[i] : sf::Vector2f(0.0f, 0.0f);
                            enemyManager->SetEnemyTargetPosition(id, parsed.enemyPositions[i], velocity);
                        }
                    }
                }
            },
            [](Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender) {           
            });
    MessageHandler::RegisterMessageType("EC", 
        ParseEnemyClearMessage,
        [](Game& game, ClientNetwork& client, const ParsedMessage& parsed) {
            PlayingState* state = GetPlayingState(&game);
            if (state && state->GetEnemyManager()) {
                state->GetEnemyManager()->ClearEnemies();
            }
        },
        [](Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender) {
        });
}

// Enemy message parsing functions
ParsedMessage EnemyMessageHandler::ParseEnemyAddMessage(const std::vector<std::string>& parts) {
    ParsedMessage parsed;
    parsed.type = MessageType::EnemyAdd;
    
    if (parts.size() >= 5) {
        parsed.enemyId = std::stoi(parts[1]);
        parsed.enemyType = static_cast<EnemyType>(std::stoi(parts[2]));
        
        std::string posStr = parts[3];
        size_t commaPos = posStr.find(',');
        if (commaPos != std::string::npos) {
            parsed.position.x = std::stof(posStr.substr(0, commaPos));
            parsed.position.y = std::stof(posStr.substr(commaPos + 1));
        }
        
        parsed.health = std::stof(parts[4]);
    }
    
    return parsed;
}

ParsedMessage EnemyMessageHandler::ParseEnemyRemoveMessage(const std::vector<std::string>& parts) {
    ParsedMessage parsed;
    parsed.type = MessageType::EnemyRemove;
    
    if (parts.size() >= 2) {
        parsed.enemyId = std::stoi(parts[1]);
    }
    
    return parsed;
}

ParsedMessage EnemyMessageHandler::ParseEnemyDamageMessage(const std::vector<std::string>& parts) {
    ParsedMessage parsed;
    parsed.type = MessageType::EnemyDamage;
    
    if (parts.size() >= 4) {
        parsed.enemyId = std::stoi(parts[1]);
        parsed.damage = std::stof(parts[2]);
        parsed.health = std::stof(parts[3]);
    }
    
    return parsed;
}

ParsedMessage EnemyMessageHandler::ParseEnemyPositionUpdateMessage(const std::vector<std::string>& parts) {
    ParsedMessage parsed;
    parsed.type = MessageType::EnemyPositionUpdate;
    
    // Format: EP|id,x,y,vx,vy|id,x,y,vx,vy|...
    for (size_t i = 1; i < parts.size(); ++i) {
        std::string chunk = parts[i];
        std::vector<std::string> subParts = MessageHandler::SplitString(chunk, ',');
        
        if (subParts.size() >= 5) {  // id,x,y,vx,vy
            int id = std::stoi(subParts[0]);
            float x = std::stof(subParts[1]);
            float y = std::stof(subParts[2]);
            float vx = std::stof(subParts[3]);
            float vy = std::stof(subParts[4]);
            
            parsed.enemyIds.push_back(id);
            parsed.enemyPositions.push_back(sf::Vector2f(x, y));
            parsed.enemyVelocities.push_back(sf::Vector2f(vx, vy));
        } 
    }
    
    return parsed;
}

ParsedMessage EnemyMessageHandler::ParseEnemyStateMessage(const std::vector<std::string>& parts) {
    ParsedMessage parsed;
    parsed.type = MessageType::EnemyState; 
    // First, let's handle the case where we have a full message with multiple entities
    // Format might be: ES|entity1|entity2|entity3|...
    for (size_t i = 1; i < parts.size(); ++i) {
        std::string entityData = parts[i];
        
        // Skip empty parts
        if (entityData.empty()) continue;
        
        // Try to parse individual entity data
        std::vector<std::string> fields = MessageHandler::SplitString(entityData, ',');
        
        // Expected format: id,type,x,y,health,vx,vy
        if (fields.size() >= 5) {
            try {
                int id = std::stoi(fields[0]);
                int type = std::stoi(fields[1]);
                float x = std::stof(fields[2]);
                float y = std::stof(fields[3]);
                float health = std::stof(fields[4]);
                
                parsed.enemyIds.push_back(id);
                parsed.enemyTypes.push_back(type);
                parsed.enemyPositions.push_back(sf::Vector2f(x, y));
                parsed.enemyHealths.push_back(health);
                
                // Parse velocity if available (fields 5 and 6)
                if (fields.size() >= 7) {
                    float vx = std::stof(fields[5]);
                    float vy = std::stof(fields[6]);
                    parsed.enemyVelocities.push_back(sf::Vector2f(vx, vy));

                } else {
                    // Add a zero velocity if not available
                    parsed.enemyVelocities.push_back(sf::Vector2f(0.0f, 0.0f));

                }
            }
            catch (const std::exception& e) {

            }
        }
       
    }

}

ParsedMessage EnemyMessageHandler::ParseEnemyStateRequestMessage(const std::vector<std::string>& parts) {
    ParsedMessage parsed;
    parsed.type = MessageType::EnemyStateRequest;
    return parsed;
}

ParsedMessage EnemyMessageHandler::ParseEnemyClearMessage(const std::vector<std::string>& parts) {
    ParsedMessage parsed;
    parsed.type = MessageType::EnemyClear;
    return parsed;
}

// Enemy message formatting functions
std::string EnemyMessageHandler::FormatEnemyAddMessage(int enemyId, EnemyType type, const sf::Vector2f& position, float health) {
    std::ostringstream oss;
    oss << "EA|" << enemyId << "|" << static_cast<int>(type) << "|" 
        << position.x << "," << position.y << "|" << health;
    return oss.str();
}

std::string EnemyMessageHandler::FormatEnemyRemoveMessage(int enemyId) {
    std::ostringstream oss;
    oss << "ER|" << enemyId;
    return oss.str();
}

std::string EnemyMessageHandler::FormatEnemyDamageMessage(int enemyId, float damage, float remainingHealth) {
    std::ostringstream oss;
    oss << "ED|" << enemyId << "|" << damage << "|" << remainingHealth;
    return oss.str();
}

std::string EnemyMessageHandler::FormatEnemyPositionUpdateMessage(const std::vector<int>& enemyIds, const std::vector<sf::Vector2f>& positions) {
    std::ostringstream oss;
    oss << "EP";
    
    size_t count = std::min(enemyIds.size(), positions.size());
    for (size_t i = 0; i < count; ++i) {
        oss << "|" << enemyIds[i] << "," << positions[i].x << "," << positions[i].y;
    }
    
    return oss.str();
}

std::string EnemyMessageHandler::FormatEnemyStateMessage(const std::vector<int>& enemyIds, const std::vector<EnemyType>& types, 
                                                  const std::vector<sf::Vector2f>& positions, const std::vector<float>& healths) {
    std::ostringstream oss;
    oss << "ES";
    
    size_t count = std::min(enemyIds.size(), std::min(positions.size(), healths.size()));
    for (size_t i = 0; i < count; ++i) {
        oss << "|" << enemyIds[i] << "," << static_cast<int>(types[i]) << "," 
            << positions[i].x << "," << positions[i].y << "," << healths[i];
    }
    
    return oss.str();
}

std::string EnemyMessageHandler::FormatEnemyClearMessage() {
    return "EC";
}