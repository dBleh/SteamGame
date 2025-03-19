#include "EnemyManager.h"
#include "../../core/Game.h"
#include "../PlayerManager.h"
#include "../../network/Host.h"
#include "../../network/Client.h"
#include "../../network/NetworkManager.h"
#include "../../network/messages/MessageHandler.h"
#include "../../network/messages/PlayerMessageHandler.h"
#include "../../network/messages/EnemyMessageHandler.h"
#include "../../network/messages/StateMessageHandler.h"
#include "../../network/messages/SystemMessageHandler.h"
#include <algorithm>
#include <random>
#include <iostream>
#include <cmath>
#include <ctime>
#include <sstream>

EnemyManager::EnemyManager(Game* game, PlayerManager* playerManager)
    : game(game),
      playerManager(playerManager),
      nextEnemyId(1),
      syncTimer(0.0f),
      fullSyncTimer(0.0f),
      validationTimer(0.0f),
      currentWave(0) {
    // Initialize random seed with different values for host and clients
    // to avoid synchronization issues with random number generation
    CSteamID myID = SteamUser()->GetSteamID();
    std::srand(static_cast<unsigned int>(std::time(nullptr) + myID.ConvertToUint64() % 1000));
}

void EnemyManager::Update(float dt) {
    // Skip updates if paused or not in gameplay
    if (game->GetCurrentState() != GameState::Playing) return;
    
    // Update batch spawning if needed (only host should spawn)
    if (remainingEnemiesInWave > 0 && IsLocalPlayerHost()) {
        UpdateSpawning(dt);
    }
    
    // Update position interpolation for smooth movement
    UpdateEnemyInterpolation(dt);
    
    // Update all enemies
    for (auto& pair : enemies) {
        pair.second->Update(dt, *playerManager);
    }
    
    // Check for collisions with players
    CheckPlayerCollisions();
    
    // Get max enemies spawnable from settings if available
    int maxEnemies = MAX_ENEMIES_SPAWNABLE;
    if (game->GetGameSettingsManager()) {
        GameSetting* maxEnemiesSetting = game->GetGameSettingsManager()->GetSetting("max_enemies_spawnable");
        if (maxEnemiesSetting) {
            maxEnemies = maxEnemiesSetting->GetIntValue();
        }
    }
    
    // If enemy count exceeds a threshold, optimize
    if (enemies.size() > maxEnemies / 5) {
        OptimizeEnemyList();
    }
    
    // Get validation check interval from settings if available
    float validationInterval = VALIDATION_CHECK_INTERVAL;
    if (game->GetGameSettingsManager()) {
        GameSetting* validationIntervalSetting = game->GetGameSettingsManager()->GetSetting("validation_check_interval");
        if (validationIntervalSetting) {
            validationInterval = validationIntervalSetting->GetFloatValue();
        }
    }
    
    // Periodic validation of enemy states
    validationTimer += dt;
    if (validationTimer >= validationInterval) {
        ValidateEnemyStates();
        validationTimer = 0.0f;
    }
    
    // Handle network synchronization if we're the host
    if (IsLocalPlayerHost()) {
        // Get sync intervals from settings if available
        float positionSyncInterval = POSITION_SYNC_INTERVAL;
        float fullSyncInterval = FULL_SYNC_INTERVAL;
        
        if (game->GetGameSettingsManager()) {
            GameSetting* syncIntervalSetting = game->GetGameSettingsManager()->GetSetting("enemy_sync_interval");
            if (syncIntervalSetting) {
                positionSyncInterval = syncIntervalSetting->GetFloatValue();
            }
            
            GameSetting* fullSyncIntervalSetting = game->GetGameSettingsManager()->GetSetting("full_sync_interval");
            if (fullSyncIntervalSetting) {
                fullSyncInterval = fullSyncIntervalSetting->GetFloatValue();
            }
        }
        
        // Sync enemy positions more frequently
        syncTimer += dt;
        if (syncTimer >= positionSyncInterval) {
            SyncEnemyPositions();
            syncTimer = 0.0f;
        }
        
        // Send full state less frequently to reduce network traffic
        fullSyncTimer += dt;
        if (fullSyncTimer >= fullSyncInterval) {
            SyncFullState();
            fullSyncTimer = 0.0f;
        }
    }
}

void EnemyManager::Render(sf::RenderWindow& window) {
    for (auto& pair : enemies) {
        pair.second->Render(window);
    }
}

int EnemyManager::AddEnemy(EnemyType type, const sf::Vector2f& position, float health) {
    int id = nextEnemyId++;
    auto enemy = CreateEnemy(type, id, position);
    enemy->SetHealth(health);
    enemies[id] = std::move(enemy);
    
    // Initialize network state for this enemy
    EnemyNetworkState state;
    state.currentPosition = position;
    state.targetPosition = position;
    state.lastUpdateTime = std::chrono::steady_clock::now();
    enemyNetworkStates[id] = state;
    
    // If we're the host, broadcast this new enemy to all clients
    if (IsLocalPlayerHost()) {
        std::ostringstream oss;
        oss << "EA|" << id << "|" << static_cast<int>(type) << "|" 
            << position.x << "," << position.y << "|" << health;
        
        game->GetNetworkManager().BroadcastMessage(oss.str());
    }
    
    return id;
}

void EnemyManager::RemoveEnemy(int id) {
    auto it = enemies.find(id);
    if (it != enemies.end()) {
        // If we're the host, broadcast this removal to all clients
        if (IsLocalPlayerHost()) {
            std::ostringstream oss;
            oss << "ER|" << id;
            game->GetNetworkManager().BroadcastMessage(oss.str());
            
        }
        
        enemies.erase(it);
        
        // Also remove from network state tracking
        auto stateIt = enemyNetworkStates.find(id);
        if (stateIt != enemyNetworkStates.end()) {
            enemyNetworkStates.erase(stateIt);
        }
    }
}

void EnemyManager::ClearEnemies() {
    enemies.clear();
    enemyNetworkStates.clear();
    
    // If we're the host, broadcast a clear command
    if (IsLocalPlayerHost()) {
        game->GetNetworkManager().BroadcastMessage("EC");
    }
}

bool EnemyManager::InflictDamage(int enemyId, float damage) {
    auto it = enemies.find(enemyId);
    if (it == enemies.end()) {
        return false;
    }
    
    float oldHealth = it->second->GetHealth();
    bool killed = it->second->TakeDamage(damage);
    float newHealth = it->second->GetHealth();
    
    // If we're the host, broadcast this damage to all clients
    if (IsLocalPlayerHost()) {
        std::string damageMsg = EnemyMessageHandler::FormatEnemyDamageMessage(
            enemyId, damage, newHealth);
        game->GetNetworkManager().BroadcastMessage(damageMsg);
        if (killed) {
            // Host removes the enemy
            RemoveEnemy(enemyId);
        }
    }   
    return killed;
}

void EnemyManager::CheckPlayerCollisions() {
    auto& players = playerManager->GetPlayers();
    float triangleDamage = TRIANGLE_DAMAGE;
    
    // Get the triangle damage setting if available
    if (game->GetGameSettingsManager()) {
        GameSetting* damageSetting = game->GetGameSettingsManager()->GetSetting("triangle_damage");
        if (damageSetting) {
            triangleDamage = damageSetting->GetFloatValue();
        }
    }
    
    for (auto enemyIt = enemies.begin(); enemyIt != enemies.end();) {
        bool enemyRemoved = false;
        
        for (auto& playerPair : players) {
            if (playerPair.second.player.IsDead()) continue;
            
            if (enemyIt->second->CheckPlayerCollision(playerPair.second.player.GetShape())) {
                // Apply damage to player - use the enemy's damage value instead of the constant
                float actualDamage = enemyIt->second->GetDamage();
                playerPair.second.player.TakeDamage(actualDamage);
                
                if (playerPair.second.player.IsDead()) {
                    playerManager->PlayerDied(playerPair.first, "");
                }
                
                // Remove enemy after collision
                int enemyId = enemyIt->first;
                enemyIt = enemies.erase(enemyIt);
                
                // Also remove from network state tracking
                auto stateIt = enemyNetworkStates.find(enemyId);
                if (stateIt != enemyNetworkStates.end()) {
                    enemyNetworkStates.erase(stateIt);
                }
                
                enemyRemoved = true;
                
                // If we're the host, broadcast this removal and player damage
                if (IsLocalPlayerHost()) {
                    std::ostringstream oss;
                    oss << "ER|" << enemyId;
                    game->GetNetworkManager().BroadcastMessage(oss.str());
                    
                    // Also inform about player damage
                    oss.str("");
                    oss << "PD|" << playerPair.first << "|" << actualDamage << "|" << enemyId;
                    game->GetNetworkManager().BroadcastMessage(oss.str());
                }
                
                break;
            }
        }
        
        if (!enemyRemoved) {
            ++enemyIt;
        }
    }
}

bool EnemyManager::CheckBulletCollision(const sf::Vector2f& bulletPos, float bulletRadius, int& outEnemyId) {
    for (auto& pair : enemies) {
        if (pair.second->CheckBulletCollision(bulletPos, bulletRadius)) {
            outEnemyId = pair.first;
            return true;
        }
    }
    
    outEnemyId = -1;
    return false;
}

void EnemyManager::SyncEnemyPositions() {
    if (enemies.empty()) return;
    
    // Get priority list of enemies to sync
    std::vector<int> priorities = GetEnemyUpdatePriorities();
    
    // Limit to a reasonable number of enemies per update
    size_t updateCount = std::min(priorities.size(), static_cast<size_t>(MAX_ENEMIES_PER_UPDATE));
    
    // Create vectors for batch update
    std::vector<int> enemyIds;
    std::vector<sf::Vector2f> positions;
    std::vector<sf::Vector2f> velocities;
    
    for (size_t i = 0; i < updateCount; ++i) {
        int enemyId = priorities[i];
        auto it = enemies.find(enemyId);
        if (it != enemies.end()) {
            enemyIds.push_back(enemyId);
            positions.push_back(it->second->GetPosition());
            velocities.push_back(it->second->GetVelocity());
        }
    }
    
    // Send the message using the existing EP format
    std::string epMessage = EnemyMessageHandler::FormatEnemyPositionUpdateMessage(enemyIds, positions);
    game->GetNetworkManager().BroadcastMessage(epMessage);
}

void EnemyManager::SyncFullState() {
    if (enemies.empty()) return;
    
    // Create vectors for batch update
    std::vector<int> enemyIds;
    std::vector<EnemyType> types;
    std::vector<sf::Vector2f> positions;
    std::vector<float> healths;
    
    // Include all enemies in the full state update
    for (const auto& pair : enemies) {
        enemyIds.push_back(pair.first);
        types.push_back(pair.second->GetType());
        positions.push_back(pair.second->GetPosition());
        healths.push_back(pair.second->GetHealth());
    }
    
    // Create full state message
    std::string fullStateMsg = EnemyMessageHandler::FormatEnemyStateMessage(enemyIds, types, positions, healths);
    
    // Use chunking for large updates
    if (fullStateMsg.length() > MAX_PACKET_SIZE) {
        std::vector<std::string> chunks = SystemMessageHandler::ChunkMessage(fullStateMsg, "ES");
        for (const auto& chunk : chunks) {
            game->GetNetworkManager().BroadcastMessage(chunk);
        }
    } else {
        game->GetNetworkManager().BroadcastMessage(fullStateMsg);
    }
}

void EnemyManager::ApplyNetworkUpdate(int enemyId, const sf::Vector2f& position, float health) {
    ApplyNetworkUpdate(enemyId, position, sf::Vector2f(0.0f, 0.0f), health);
}

void EnemyManager::ApplyNetworkUpdate(int enemyId, const sf::Vector2f& position, const sf::Vector2f& velocity, float health) {
    auto it = enemies.find(enemyId);
    if (it != enemies.end()) {
        // Update health immediately
        it->second->SetHealth(health);
        
        // For position, set up interpolation rather than direct setting
        SetEnemyTargetPosition(enemyId, position, velocity);
    } else {
        // Enemy doesn't exist, create it with default type
        RemoteAddEnemy(enemyId, EnemyType::Triangle, position, health);
    }
}

void EnemyManager::SetEnemyTargetPosition(int enemyId, const sf::Vector2f& position, const sf::Vector2f& velocity) {
    auto stateIt = enemyNetworkStates.find(enemyId);
    if (stateIt != enemyNetworkStates.end()) {
        auto& state = stateIt->second;
        auto enemyIt = enemies.find(enemyId);
        
        if (enemyIt != enemies.end()) {
            // Store current position as the starting point for interpolation
            state.currentPosition = enemyIt->second->GetPosition();
            state.currentVelocity = enemyIt->second->GetVelocity();
            
            // Set the target position for interpolation
            state.targetPosition = position;
            state.targetVelocity = velocity;
            state.lastUpdateTime = std::chrono::steady_clock::now();
            state.needsInterpolation = true;
            state.interpolationTime = 0.0f;
            
            // Log significant position changes
            sf::Vector2f diff = position - state.currentPosition;
            float distance = std::sqrt(diff.x * diff.x + diff.y * diff.y);
            if (distance > 50.0f) {
                LogPositionUpdate(enemyId, state.currentPosition, position, "network");
            }
        }
    } else {
        // State doesn't exist, create it
        EnemyNetworkState state;
        auto enemyIt = enemies.find(enemyId);
        
        if (enemyIt != enemies.end()) {
            state.currentPosition = enemyIt->second->GetPosition();
            state.currentVelocity = enemyIt->second->GetVelocity();
        } else {
            state.currentPosition = position;
            state.currentVelocity = velocity;
        }
        
        state.targetPosition = position;
        state.targetVelocity = velocity;
        state.lastUpdateTime = std::chrono::steady_clock::now();
        state.needsInterpolation = true;
        state.interpolationTime = 0.0f;
        
        enemyNetworkStates[enemyId] = state;
    }
}

void EnemyManager::UpdateEnemyInterpolation(float dt) {
    for (auto& pair : enemyNetworkStates) {
        int enemyId = pair.first;
        auto& state = pair.second;
        
        if (state.needsInterpolation) {
            InterpolateEnemyPosition(enemyId, dt);
        }
    }
}

void EnemyManager::InterpolateEnemyPosition(int enemyId, float dt) {
    auto stateIt = enemyNetworkStates.find(enemyId);
    auto enemyIt = enemies.find(enemyId);
    
    if (stateIt != enemyNetworkStates.end() && enemyIt != enemies.end()) {
        auto& state = stateIt->second;
        
        // Skip interpolation if positions are already very close
        sf::Vector2f diff = state.targetPosition - state.currentPosition;
        float distance = std::sqrt(diff.x * diff.x + diff.y * diff.y);
        
        if (distance < 0.1f) {
            // Positions are close enough, just set directly
            enemyIt->second->SetPosition(state.targetPosition);
            enemyIt->second->SetVelocity(state.targetVelocity);
            state.needsInterpolation = false;
            state.currentPosition = state.targetPosition;
            state.currentVelocity = state.targetVelocity;
            return;
        }
        
        // Check if time since update is too long (teleport instead of interpolate)
        auto now = std::chrono::steady_clock::now();
        float secondsSinceUpdate = std::chrono::duration<float>(now - state.lastUpdateTime).count();
        
        if (secondsSinceUpdate > 1.0f) {
            // Too long since update, just teleport
            enemyIt->second->SetPosition(state.targetPosition);
            enemyIt->second->SetVelocity(state.targetVelocity);
            state.needsInterpolation = false;
            state.currentPosition = state.targetPosition;
            state.currentVelocity = state.targetVelocity;
            
            LogPositionUpdate(enemyId, state.currentPosition, state.targetPosition, "teleport");
            return;
        }
        
        // Update interpolation time
        state.interpolationTime += dt * POSITION_INTERPOLATION_SPEED;
        
        // Clamp interpolation factor between 0 and 1
        float t = std::min(1.0f, state.interpolationTime);
        
        // Perform actual interpolation
        sf::Vector2f newPosition = state.currentPosition + (state.targetPosition - state.currentPosition) * t;
        sf::Vector2f newVelocity = state.currentVelocity + (state.targetVelocity - state.currentVelocity) * t;
        
        enemyIt->second->SetPosition(newPosition);
        enemyIt->second->SetVelocity(newVelocity);
        
        // Check if interpolation is complete
        if (t >= 1.0f) {
            state.needsInterpolation = false;
            state.currentPosition = state.targetPosition;
            state.currentVelocity = state.targetVelocity;
        }
    }
}

void EnemyManager::RemoteAddEnemy(int enemyId, EnemyType type, const sf::Vector2f& position, float health) {
    // Create the enemy with the given ID
    auto enemy = CreateEnemy(type, enemyId, position);
    enemy->SetHealth(health);
    enemies[enemyId] = std::move(enemy);
    
    // Initialize network state
    EnemyNetworkState state;
    state.currentPosition = position;
    state.targetPosition = position;
    state.lastUpdateTime = std::chrono::steady_clock::now();
    enemyNetworkStates[enemyId] = state;
    
    // Update nextEnemyId if necessary
    if (enemyId >= nextEnemyId) {
        nextEnemyId = enemyId + 1;
    }
}

void EnemyManager::RemoteRemoveEnemy(int enemyId) {
    auto it = enemies.find(enemyId);
    if (it != enemies.end()) {
        enemies.erase(it);
        
        // Also remove from network state tracking
        auto stateIt = enemyNetworkStates.find(enemyId);
        if (stateIt != enemyNetworkStates.end()) {
            enemyNetworkStates.erase(stateIt);
        }
    }
}

void EnemyManager::StartNewWave(int enemyCount, EnemyType type) {
    // Only the host should clear enemies and start waves
    if (!IsLocalPlayerHost()) return;
    
    // Clear any remaining enemies
    ClearEnemies();
    
    // Increment wave counter
    currentWave++;
    
    // Get the base enemy count from settings if available
    int baseEnemyCount = enemyCount;
    int additionalEnemies = 0;
    float waveCooldownTime = WAVE_COOLDOWN_TIME;
    
    // Use settings from GameSettingsManager if available
    if (game->GetGameSettingsManager()) {
        // For first wave, use first_wave_enemy_count setting
        if (currentWave == 1) {
            GameSetting* firstWaveSetting = game->GetGameSettingsManager()->GetSetting("first_wave_enemy_count");
            if (firstWaveSetting) {
                baseEnemyCount = firstWaveSetting->GetIntValue();
            }
        } else {
            // For subsequent waves, use base_enemies_per_wave + scaling factor
            GameSetting* baseEnemiesSetting = game->GetGameSettingsManager()->GetSetting("base_enemies_per_wave");
            GameSetting* scaleFactorSetting = game->GetGameSettingsManager()->GetSetting("enemies_scale_per_wave");
            
            if (baseEnemiesSetting && scaleFactorSetting) {
                baseEnemyCount = baseEnemiesSetting->GetIntValue();
                float scaleFactor = (currentWave - 1) * (scaleFactorSetting->GetIntValue() / 100.0f);
                additionalEnemies = static_cast<int>(baseEnemyCount * scaleFactor);
            }
        }
        
        // Get wave cooldown setting
        GameSetting* cooldownSetting = game->GetGameSettingsManager()->GetSetting("wave_cooldown_time");
        if (cooldownSetting) {
            waveCooldownTime = cooldownSetting->GetFloatValue();
        }
    }
    
    // Calculate total enemies for this wave
    int totalEnemies = baseEnemyCount + additionalEnemies;
    
    // Apply max enemy limit if available from settings
    if (game->GetGameSettingsManager()) {
        GameSetting* maxEnemiesSetting = game->GetGameSettingsManager()->GetSetting("max_enemies_spawnable");
        if (maxEnemiesSetting && totalEnemies > maxEnemiesSetting->GetIntValue()) {
            totalEnemies = maxEnemiesSetting->GetIntValue();
        }
    }
    
    // Store for batch spawning
    remainingEnemiesInWave = totalEnemies;
    currentWaveEnemyType = type;
    
    // Cache player positions for spawning
    playerPositionsCache.clear();
    auto& players = playerManager->GetPlayers();
    
    for (const auto& pair : players) {
        if (!pair.second.player.IsDead()) {
            playerPositionsCache.push_back(pair.second.player.GetPosition());
        }
    }
    
    // If no players, use origin
    if (playerPositionsCache.empty()) {
        playerPositionsCache.push_back(sf::Vector2f(0.0f, 0.0f));
    }
    
    // Broadcast wave start
    std::ostringstream oss;
    oss << "WS|" << currentWave << "|" << totalEnemies;
    game->GetNetworkManager().BroadcastMessage(oss.str());
    
    // Schedule wave cooldown if needed
    // This would need additional implementation if you want to use the cooldown time
}

void EnemyManager::OptimizeEnemyList() {
    // Remove enemies that are very far from all players
    auto& players = playerManager->GetPlayers();
    std::vector<int> enemiesToRemove;
    
    // If no players or not host, don't remove any enemies
    if (players.empty() || !IsLocalPlayerHost()) return;
    
    for (const auto& pair : enemies) {
        const sf::Vector2f& enemyPos = pair.second->GetPosition();
        bool tooFar = true;
        
        for (const auto& playerPair : players) {
            if (playerPair.second.player.IsDead()) continue;
            
            sf::Vector2f playerPos = playerPair.second.player.GetPosition();
            float distSquared = std::pow(enemyPos.x - playerPos.x, 2) + 
                                std::pow(enemyPos.y - playerPos.y, 2);
            
            // Keep enemies within culling distance of any player
            if (distSquared < enemyCullingDistance * enemyCullingDistance) {
                tooFar = false;
                break;
            }
        }
        
        if (tooFar) {
            enemiesToRemove.push_back(pair.first);
        }
    }
    
    // Remove the enemies that are too far
    for (int id : enemiesToRemove) {
        RemoveEnemy(id);
    }
}

void EnemyManager::ValidateEnemyStates() {
    if (!IsLocalPlayerHost()) return;
    
    // Check for inconsistencies in enemy state
    std::vector<int> invalidEnemies;
    
    for (const auto& pair : enemies) {
        int id = pair.first;
        const sf::Vector2f& pos = pair.second->GetPosition();
        
        // Check for NaN or infinite position values
        if (std::isnan(pos.x) || std::isnan(pos.y) || 
            std::isinf(pos.x) || std::isinf(pos.y)) {
            invalidEnemies.push_back(id);
        }
        
        // Check for very extreme positions (likely a bug)
        if (std::abs(pos.x) > 10000.0f || std::abs(pos.y) > 10000.0f) {
            invalidEnemies.push_back(id);
        }
    }
    
    // Remove enemies with invalid states
    for (int id : invalidEnemies) {
        RemoveEnemy(id);
    }
}

bool EnemyManager::IsValidSpawnPosition(const sf::Vector2f& position) {
    // Get min spawn distance from settings if available
    float minSpawnDistance = TRIANGLE_MIN_SPAWN_DISTANCE;
    if (game->GetGameSettingsManager()) {
        GameSetting* minDistanceSetting = game->GetGameSettingsManager()->GetSetting("triangle_min_spawn_distance");
        if (minDistanceSetting) {
            minSpawnDistance = minDistanceSetting->GetFloatValue();
        }
    }
    
    // Check if the position is too close to any player
    auto& players = playerManager->GetPlayers();
    
    for (const auto& pair : players) {
        if (pair.second.player.IsDead()) continue;
        
        sf::Vector2f playerPos = pair.second.player.GetPosition();
        float distSquared = std::pow(position.x - playerPos.x, 2) + 
                            std::pow(position.y - playerPos.y, 2);
        
        if (distSquared < minSpawnDistance * minSpawnDistance) {
            return false;
        }
    }
    
    // Calculate enemy spacing based on enemy size
    float enemySpacing = 30.0f;
    if (game->GetGameSettingsManager()) {
        GameSetting* enemySizeSetting = game->GetGameSettingsManager()->GetSetting("enemy_size");
        if (enemySizeSetting) {
            enemySpacing = enemySizeSetting->GetFloatValue() * 1.5f;
        }
    }
    
    // Also check if the position is too close to other enemies
    for (const auto& pair : enemies) {
        sf::Vector2f enemyPos = pair.second->GetPosition();
        float distSquared = std::pow(position.x - enemyPos.x, 2) + 
                            std::pow(position.y - enemyPos.y, 2);
        
        // Too close to another enemy - prevent clumping
        if (distSquared < enemySpacing * enemySpacing) {
            return false;
        }
    }
    
    return true;
}

std::vector<int> EnemyManager::GetEnemyUpdatePriorities() {
    std::vector<int> priorityList;
    auto& players = playerManager->GetPlayers();
    
    // If no players, prioritize based on ID
    if (players.empty()) {
        for (const auto& pair : enemies) {
            priorityList.push_back(pair.first);
        }
        return priorityList;
    }
    
    // Create a vector of pairs (enemy ID, priority score)
    std::vector<std::pair<int, float>> enemyPriorities;
    
    for (const auto& pair : enemies) {
        const sf::Vector2f& enemyPos = pair.second->GetPosition();
        float minDistSquared = std::numeric_limits<float>::max();
        
        // Find the closest player
        for (const auto& playerPair : players) {
            if (playerPair.second.player.IsDead()) continue;
            
            sf::Vector2f playerPos = playerPair.second.player.GetPosition();
            float distSquared = std::pow(enemyPos.x - playerPos.x, 2) + 
                                std::pow(enemyPos.y - playerPos.y, 2);
            
            minDistSquared = std::min(minDistSquared, distSquared);
        }
        
        // Calculate base priority (closer = higher priority)
        float priority = 1.0f / (1.0f + std::sqrt(minDistSquared));
        
        // Add variation to priority to ensure all enemies get updated eventually
        // This helps prevent some enemies from never being updated
        static unsigned int counter = 0;
        counter++;
        
        // Add a small distance-independent factor based on enemy ID and global counter
        // This ensures all enemies get scheduled for updates eventually
        float variationFactor = 0.1f * (static_cast<float>((pair.first + counter) % 100) / 100.0f);
        priority += variationFactor;
        
        enemyPriorities.push_back({pair.first, priority});
    }
    
    // Sort by priority (higher first)
    std::sort(enemyPriorities.begin(), enemyPriorities.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });
    
    // Extract only the IDs
    for (const auto& pair : enemyPriorities) {
        priorityList.push_back(pair.first);
    }
    
    return priorityList;
}

sf::Vector2f EnemyManager::GetRandomSpawnPosition(const sf::Vector2f& targetPosition, 
                                               float minDistance, float maxDistance) {
    // Create random number generator
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * 3.14159f);
    std::uniform_real_distribution<float> distDist(minDistance, maxDistance);
    
    // Get spawn radius from settings if available
    float spawnRadius = SPAWN_RADIUS;
    if (game->GetGameSettingsManager()) {
        GameSetting* spawnRadiusSetting = game->GetGameSettingsManager()->GetSetting("spawn_radius");
        if (spawnRadiusSetting) {
            spawnRadius = spawnRadiusSetting->GetFloatValue();
        }
    }
    
    // Try to find a valid spawn position (max 10 attempts)
    const int MAX_ATTEMPTS = 10;
    for (int attempt = 0; attempt < MAX_ATTEMPTS; ++attempt) {
        // Generate a random angle and distance
        float angle = angleDist(gen);
        float distance = distDist(gen);
        
        // Convert to Cartesian coordinates
        float x = targetPosition.x + std::cos(angle) * distance;
        float y = targetPosition.y + std::sin(angle) * distance;
        
        sf::Vector2f spawnPos(x, y);
        
        // Validate the spawn position
        if (IsValidSpawnPosition(spawnPos)) {
            return spawnPos;
        }
    }
    
    // If all attempts failed, increase the distance and try once more
    float angle = angleDist(gen);
    float distance = maxDistance * 1.5f;
    
    float x = targetPosition.x + std::cos(angle) * distance;
    float y = targetPosition.y + std::sin(angle) * distance;
    
    return sf::Vector2f(x, y);
}

bool EnemyManager::IsLocalPlayerHost() const {
    CSteamID myID = SteamUser()->GetSteamID();
    CSteamID hostID = SteamMatchmaking()->GetLobbyOwner(game->GetLobbyID());
    return (myID == hostID);
}

void EnemyManager::LogPositionUpdate(int enemyId, const sf::Vector2f& oldPos, const sf::Vector2f& newPos, const char* source) {
    float distance = std::sqrt(std::pow(newPos.x - oldPos.x, 2) + std::pow(newPos.y - oldPos.y, 2));
    if (distance > 50.0f) {
        std::cout << "[" << (IsLocalPlayerHost() ? "HOST" : "CLIENT") << "] " 
                  << "Enemy " << enemyId << " position changed significantly (" << source << "): "
                  << "(" << oldPos.x << "," << oldPos.y << ") -> "
                  << "(" << newPos.x << "," << newPos.y << "), "
                  << "distance: " << distance << "\n";
    }
}

void EnemyManager::RemoveEnemiesNotInList(const std::vector<int>& validIds) {
    std::vector<int> enemyIdsToRemove;
    
    for (const auto& pair : enemies) {
        bool found = false;
        for (int id : validIds) {
            if (pair.first == id) {
                found = true;
                break;
            }
        }
        
        if (!found) {
            enemyIdsToRemove.push_back(pair.first);
        }
    }
    
    if (!enemyIdsToRemove.empty()) {
        std::cout << "[" << (IsLocalPlayerHost() ? "HOST" : "CLIENT") << "] " 
                  << "Removing " << enemyIdsToRemove.size() 
                  << " enemies not in valid list\n";
                  
        for (int id : enemyIdsToRemove) {
            RemoteRemoveEnemy(id);
        }
    }
}

void EnemyManager::ApplySettings() {
    if (!game->GetGameSettingsManager()) return;
    
    // Get the relevant settings
    GameSetting* healthSetting = game->GetGameSettingsManager()->GetSetting("enemy_health");
    GameSetting* speedSetting = game->GetGameSettingsManager()->GetSetting("enemy_speed");
    GameSetting* triangleDamageSetting = game->GetGameSettingsManager()->GetSetting("triangle_damage");
    GameSetting* triangleHealthSetting = game->GetGameSettingsManager()->GetSetting("triangle_health");
    GameSetting* triangleSizeSetting = game->GetGameSettingsManager()->GetSetting("triangle_size");
    GameSetting* enemySizeSetting = game->GetGameSettingsManager()->GetSetting("enemy_size");
    
    // Update all existing enemies with new settings
    for (auto& pair : enemies) {
        Enemy* enemy = pair.second.get();
        EnemyType enemyType = enemy->GetType();
        
        // Update speed for all enemies
        if (speedSetting) {
            enemy->SetSpeed(speedSetting->GetFloatValue());
        }
        // Apply triangle size setting
        if (triangleSizeSetting) {
            enemy->SetSize(triangleSizeSetting->GetFloatValue());
        }
        
        // Type-specific updates
        if (enemyType == EnemyType::Triangle) {
            // Update triangle health if not damaged (at full health)
            if (triangleHealthSetting && enemy->GetHealth() == TRIANGLE_HEALTH) {
                enemy->SetHealth(triangleHealthSetting->GetFloatValue());
            }
            
            // Update triangle damage
            if (triangleDamageSetting) {
                enemy->SetDamage(triangleDamageSetting->GetFloatValue());
            }
        } else {
            // For other enemy types, use general health setting if at full health
            if (healthSetting && enemy->GetHealth() == ENEMY_HEALTH) {
                enemy->SetHealth(healthSetting->GetFloatValue());
            }
        }
        
        // Update all existing enemies with new settings
for (auto& pair : enemies) {
    Enemy* enemy = pair.second.get();
    EnemyType enemyType = enemy->GetType();
    
    // Update speed for all enemies
    if (speedSetting) {
        enemy->SetSpeed(speedSetting->GetFloatValue());
    }
    
    // Type-specific updates
    if (enemyType == EnemyType::Triangle) {
        // Update triangle health if not damaged (at full health)
        if (triangleHealthSetting && enemy->GetHealth() == TRIANGLE_HEALTH) {
            enemy->SetHealth(triangleHealthSetting->GetFloatValue());
        }
        
        // Update triangle damage
        if (triangleDamageSetting) {
            enemy->SetDamage(triangleDamageSetting->GetFloatValue());
        }
        
        // Apply triangle size setting
        if (triangleSizeSetting) {
            enemy->SetSize(triangleSizeSetting->GetFloatValue());
        }
    } else {
        // For other enemy types, use general health setting if at full health
        if (healthSetting && enemy->GetHealth() == ENEMY_HEALTH) {
            enemy->SetHealth(healthSetting->GetFloatValue());
        }
        
        // Apply general enemy size setting
        if (enemySizeSetting) {
            enemy->SetSize(enemySizeSetting->GetFloatValue());
        }
    }
}
    }
    
    // Update runtime settings (these will be used instead of the #define constants)
    GameSetting* batchIntervalSetting = game->GetGameSettingsManager()->GetSetting("enemy_spawn_batch_interval");
    if (batchIntervalSetting) {
        spawnBatchInterval = batchIntervalSetting->GetFloatValue();
    }
    
    GameSetting* batchSizeSetting = game->GetGameSettingsManager()->GetSetting("enemy_spawn_batch_size");
    if (batchSizeSetting) {
        spawnBatchSize = batchSizeSetting->GetIntValue();
    }
    
    // Update culling distance
    GameSetting* cullingDistanceSetting = game->GetGameSettingsManager()->GetSetting("enemy_culling_distance");
    if (cullingDistanceSetting) {
        enemyCullingDistance = cullingDistanceSetting->GetFloatValue();
    }
    
    // Triangle spawn distances
    GameSetting* minSpawnDistanceSetting = game->GetGameSettingsManager()->GetSetting("triangle_min_spawn_distance");
    if (minSpawnDistanceSetting) {
        triangleMinSpawnDistance = minSpawnDistanceSetting->GetFloatValue();
    }
    
    GameSetting* maxSpawnDistanceSetting = game->GetGameSettingsManager()->GetSetting("triangle_max_spawn_distance");
    if (maxSpawnDistanceSetting) {
        triangleMaxSpawnDistance = maxSpawnDistanceSetting->GetFloatValue();
    }
    
    // Network synchronization settings
    GameSetting* syncIntervalSetting = game->GetGameSettingsManager()->GetSetting("enemy_sync_interval");
    if (syncIntervalSetting) {
        // Update sync interval constant - need a local variable in the class
        // This would require adding a member variable to replace the constexpr
    }
    
    GameSetting* fullSyncIntervalSetting = game->GetGameSettingsManager()->GetSetting("full_sync_interval");
    if (fullSyncIntervalSetting) {
        // Update full sync interval constant
        // This would require adding a member variable to replace the constexpr
    }
    
    GameSetting* maxEnemiesPerUpdateSetting = game->GetGameSettingsManager()->GetSetting("max_enemies_per_update");
    if (maxEnemiesPerUpdateSetting) {
        // Update max enemies per update constant
        // This would require adding a member variable to replace the constexpr
    }
}

bool EnemyManager::IsWaveComplete() const {
    // A wave is complete when all enemies have been spawned and eliminated
    return enemies.empty() && remainingEnemiesInWave == 0;
}

void EnemyManager::UpdateSpawning(float dt) {
    // Only the host should spawn enemies
    if (!IsLocalPlayerHost()) return;
    
    // Update batch timer
    batchSpawnTimer += dt;
    
    // Check if it's time to spawn a batch
    if (batchSpawnTimer >= spawnBatchInterval) {
        // Reset timer
        batchSpawnTimer = 0.0f;
        
        // Calculate how many enemies to spawn in this batch
        int spawnCount = std::min(spawnBatchSize, remainingEnemiesInWave);
        
        if (spawnCount > 0) {
            
            // Spawn the batch
            for (int i = 0; i < spawnCount; ++i) {
                // Randomly select a player position to spawn relative to
                sf::Vector2f targetPos = playerPositionsCache[rand() % playerPositionsCache.size()];
                
                // Get a spawn position away from the player
                sf::Vector2f spawnPos = GetRandomSpawnPosition(targetPos, 
                                                             triangleMinSpawnDistance, 
                                                             triangleMaxSpawnDistance);
                
                // Add the enemy
                AddEnemy(currentWaveEnemyType, spawnPos);
            }
            
            // Update remaining count
            remainingEnemiesInWave -= spawnCount;
        }
    }
}

Enemy* EnemyManager::FindEnemy(int id) {
    auto it = enemies.find(id);
    return (it != enemies.end()) ? it->second.get() : nullptr;
}

std::unique_ptr<Enemy> EnemyManager::CreateEnemy(EnemyType type, int id, const sf::Vector2f& position) {
    // Get settings from GameSettingsManager if available
    float enemyHealth = ENEMY_HEALTH;
    float enemySpeed = ENEMY_SPEED;
    float enemySize = ENEMY_SIZE;
    float triangleSize = TRIANGLE_SIZE;
    float triangleHealth = TRIANGLE_HEALTH;
    float triangleDamage = TRIANGLE_DAMAGE;
    
    if (game->GetGameSettingsManager()) {
        GameSetting* healthSetting = game->GetGameSettingsManager()->GetSetting("enemy_health");
        GameSetting* speedSetting = game->GetGameSettingsManager()->GetSetting("enemy_speed");
        GameSetting* enemySizeSetting = game->GetGameSettingsManager()->GetSetting("enemy_size");
        GameSetting* triangleSizeSetting = game->GetGameSettingsManager()->GetSetting("triangle_size");
        GameSetting* triangleHealthSetting = game->GetGameSettingsManager()->GetSetting("triangle_health");
        GameSetting* triangleDamageSetting = game->GetGameSettingsManager()->GetSetting("triangle_damage");
        
        if (healthSetting) enemyHealth = healthSetting->GetFloatValue();
        if (speedSetting) enemySpeed = speedSetting->GetFloatValue();
        if (enemySizeSetting) enemySize = enemySizeSetting->GetFloatValue();
        if (triangleSizeSetting) triangleSize = triangleSizeSetting->GetFloatValue();
        if (triangleHealthSetting) triangleHealth = triangleHealthSetting->GetFloatValue();
        if (triangleDamageSetting) triangleDamage = triangleDamageSetting->GetFloatValue();
    }
    
    // Create the appropriate enemy type with the configured settings
    switch (type) {
        case EnemyType::Triangle: {
            auto enemy = std::unique_ptr<Enemy>(new TriangleEnemy(id, position, triangleHealth, enemySpeed, triangleSize));
            enemy->SetDamage(triangleDamage);
            return enemy;
        }
            
        // Add cases for other enemy types when implemented
        case EnemyType::Circle:
        case EnemyType::Square:
        case EnemyType::Boss:
            // Default to Triangle until other types are implemented
            auto enemy = std::unique_ptr<Enemy>(new TriangleEnemy(id, position, triangleHealth, enemySpeed, triangleSize));
            enemy->SetDamage(triangleDamage);
            return enemy;
    }
    
    // Default case (should never reach here)
    auto enemy = std::unique_ptr<Enemy>(new TriangleEnemy(id, position, triangleHealth, enemySpeed, triangleSize));
    enemy->SetDamage(triangleDamage);
    return enemy;
}