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
            std::cout << "[HOST] Received enemy add message from client, ignoring\n";
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
                    // Create a set of valid enemy IDs from this message
                    std::unordered_set<int> validIds;
                    for (size_t i = 0; i < parsed.enemyIds.size(); ++i) {
                        validIds.insert(parsed.enemyIds[i]);
                    }
                    auto enemyManager = state->GetEnemyManager();
                    // First pass: Update or create enemies
                    for (size_t i = 0; i < parsed.enemyIds.size(); ++i) {
                        int id = parsed.enemyIds[i];
                        
                        auto enemy = enemyManager->FindEnemy(id);
                        
                        if (enemy) {
                            // Update existing enemy
                            enemy->SetPosition(parsed.enemyPositions[i]);
                            
                            // Update health if available
                            if (i < parsed.enemyHealths.size()) {
                                enemy->SetHealth(parsed.enemyHealths[i]);
                            }
                            
                            // Update velocity if available
                            if (i < parsed.enemyVelocities.size()) {
                                enemy->SetVelocity(parsed.enemyVelocities[i]);
                            }
                        } else if (i < parsed.enemyTypes.size() && i < parsed.enemyHealths.size()) {
                            // Create new enemy
                            EnemyType type = static_cast<EnemyType>(parsed.enemyTypes[i]);
                            enemyManager->RemoteAddEnemy(id, type, parsed.enemyPositions[i], 
                                                       parsed.enemyHealths[i]);
                        }
                    }
                    
                    // Second pass: Remove enemies not in the valid set
                    // This ensures client and host stay in sync
                    enemyManager->RemoveEnemiesNotInList(std::vector<int>(validIds.begin(), validIds.end()));
                }
            },
            [](Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender) {
                // Host typically sends state, not receives it
            });
        MessageHandler::RegisterMessageType("EP", 
            ParseEnemyPositionUpdateMessage,
            [](Game& game, ClientNetwork& client, const ParsedMessage& parsed) {
                PlayingState* state = GetPlayingState(&game);
                if (state && state->GetEnemyManager()) {
                    // Update enemy positions from network message
                    for (size_t i = 0; i < parsed.enemyIds.size(); ++i) {
                        int id = parsed.enemyIds[i];
                        Enemy* enemy = state->GetEnemyManager()->FindEnemy(id);
                        if (enemy) {
                            // Update existing enemy position
                            enemy->SetPosition(parsed.enemyPositions[i]);
                            
                            // If velocity is provided, update it too
                            if (i < parsed.enemyVelocities.size()) {
                                enemy->SetVelocity(parsed.enemyVelocities[i]);
                            }
                        }
                    }
                }
            },
            [](Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender) {
                // Host doesn't usually receive EP messages, but could process them if needed
                std::cout << "[HOST] Received enemy position update from client, ignoring\n";
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

    MessageHandler::RegisterMessageType("EC", 
        ParseEnemyClearMessage,
        [](Game& game, ClientNetwork& client, const ParsedMessage& parsed) {
            PlayingState* state = GetPlayingState(&game);
            if (state && state->GetEnemyManager()) {
                state->GetEnemyManager()->ClearEnemies();
            }
        },
        [](Game& game, HostNetwork& host, const ParsedMessage& parsed, CSteamID sender) {
            // Host initiates clear, not receives it
            std::cout << "[HOST] Received enemy clear from client, ignoring\n";
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
    // Or potentially older format: EP|id,x,y|id,x,y|...
    for (size_t i = 1; i < parts.size(); ++i) {
        std::string chunk = parts[i];
        std::vector<std::string> subParts = MessageHandler::SplitString(chunk, ',');
        
        // Check if we have at least id,x,y (3 components)
        if (subParts.size() >= 3) {
            try {
                int id = std::stoi(subParts[0]);
                float x = std::stof(subParts[1]);
                float y = std::stof(subParts[2]);
                
                // Default velocities
                float vx = 0.0f;
                float vy = 0.0f;
                
                // If we have velocity components, use them
                if (subParts.size() >= 5) {
                    vx = std::stof(subParts[3]);
                    vy = std::stof(subParts[4]);
                }
                
                parsed.enemyIds.push_back(id);
                parsed.enemyPositions.push_back(sf::Vector2f(x, y));
                parsed.enemyVelocities.push_back(sf::Vector2f(vx, vy));
                
                std::cout << "[MessageHandler] Parsed enemy position update: id=" << id 
                          << ", pos=(" << x << "," << y << ")"
                          << ", vel=(" << vx << "," << vy << ")" << std::endl;
            } catch (const std::exception& e) {
                std::cout << "[MessageHandler] Error parsing enemy position data: " 
                          << chunk << " - Error: " << e.what() << std::endl;
            }
        } else {
            std::cout << "[MessageHandler] Ignoring malformed enemy position data: " 
                      << chunk << " (need at least 3 components for id,x,y, got " 
                      << subParts.size() << ")" << std::endl;
        }
    }
    
    return parsed;
}

ParsedMessage EnemyMessageHandler::ParseEnemyStateMessage(const std::vector<std::string>& parts) {
    ParsedMessage parsed;
    parsed.type = MessageType::EnemyState;
    
    std::cout << "[MessageHandler] Parsing ES message with " << parts.size() << " parts\n";
    
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
                    
                    std::cout << "[MessageHandler] Parsed enemy: id=" << id 
                              << ", pos=(" << x << "," << y << ")"
                              << ", vel=(" << vx << "," << vy << ")\n";
                } else {
                    // Add a zero velocity if not available
                    parsed.enemyVelocities.push_back(sf::Vector2f(0.0f, 0.0f));
                    
                    std::cout << "[MessageHandler] Parsed enemy: id=" << id 
                              << ", pos=(" << x << "," << y << ")"
                              << ", vel=(0,0) [default]\n";
                }
            }
            catch (const std::exception& e) {
                std::cout << "[MessageHandler] Error parsing enemy data: " << entityData 
                          << " - Error: " << e.what() << "\n";
            }
        }
        else {
            std::cout << "[MessageHandler] Skipping malformed enemy data: " << entityData 
                      << " (fields: " << fields.size() << ")\n";
        }
    }
    
    std::cout << "[MessageHandler] Successfully parsed " << parsed.enemyIds.size() << " enemies\n";
    return parsed;
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

std::string EnemyMessageHandler::FormatEnemyPositionUpdateMessage(
    const std::vector<int>& enemyIds, 
    const std::vector<sf::Vector2f>& positions,
    const std::vector<sf::Vector2f>& velocities) {
    
    std::ostringstream oss;
    oss << "EP";
    
    size_t count = std::min(enemyIds.size(), positions.size());
    for (size_t i = 0; i < count; ++i) {
        // Add velocity parameters (defaulting to 0,0 if velocities aren't provided)
        float vx = 0.0f;
        float vy = 0.0f;
        
        if (i < velocities.size()) {
            vx = velocities[i].x;
            vy = velocities[i].y;
        }
        
        oss << "|" << enemyIds[i] << "," << positions[i].x << "," << positions[i].y 
            << "," << vx << "," << vy;
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