#ifndef SPATIAL_GRID_H
#define SPATIAL_GRID_H

#include <SFML/Graphics.hpp>
#include <unordered_map>
#include <vector>
#include <cmath>
#include <sstream>
#include <functional>
#include <algorithm>
#include <iostream>
#include "../entities/EnemyBase.h"

// Forward declaration of base enemy class
class EnemyBase;

// A spatial partitioning system for efficient collision detection
class SpatialGrid {
public:
    // Constructor with cell size and world size
    SpatialGrid(float cellSize = 100.f, const sf::Vector2f& worldSize = sf::Vector2f(3000.f, 3000.f))
        : cellSize(cellSize), worldSize(worldSize) {
        // Calculate grid dimensions
        gridWidth = static_cast<int>(std::ceil(worldSize.x / cellSize));
        gridHeight = static_cast<int>(std::ceil(worldSize.y / cellSize));
        
        // Ensure minimum grid size
        gridWidth = std::max(10, gridWidth);
        gridHeight = std::max(10, gridHeight);
        
        std::cout << "SpatialGrid initialized with " << gridWidth << "x" << gridHeight << " cells" << std::endl;
        
        // Initialize the grid
        Clear();
    }
    
    // Add an enemy to the grid
    void AddEnemy(EnemyBase* enemy) {
        if (!enemy) return;
        
        try {
            // Get enemy position
            sf::Vector2f position = enemy->GetPosition();
            
            // Calculate grid cell coordinates
            int cellX = static_cast<int>(std::floor(position.x / cellSize));
            int cellY = static_cast<int>(std::floor(position.y / cellSize));
            
            // Ensure within grid bounds
            cellX = std::max(0, std::min(gridWidth - 1, cellX));
            cellY = std::max(0, std::min(gridHeight - 1, cellY));
            
            // Calculate cell index
            int cellIndex = cellY * gridWidth + cellX;
            
            // Ensure cells vector is large enough
            if (cellIndex >= cells.size()) {
                cells.resize(gridWidth * gridHeight);
            }
            
            // Add to cell vector if not already there
            auto& cellVec = cells[cellIndex];
            if (std::find(cellVec.begin(), cellVec.end(), enemy) == cellVec.end()) {
                cellVec.push_back(enemy);
            }
            
            // Store which cell the enemy is in for fast lookups
            enemyCellIndices[enemy] = cellIndex;
        }
        catch (const std::exception& e) {
            std::cout << "Exception in AddEnemy: " << e.what() << std::endl;
        }
    }
    
    // Remove an enemy from the grid
    void RemoveEnemy(EnemyBase* enemy) {
        if (!enemy) return;
        
        try {
            // Check if the enemy is in our tracking map
            auto it = enemyCellIndices.find(enemy);
            if (it == enemyCellIndices.end()) return;
            
            // Get the cell index and remove enemy from that cell
            int cellIndex = it->second;
            
            if (cellIndex >= 0 && cellIndex < cells.size()) {
                auto& cellVec = cells[cellIndex];
                cellVec.erase(
                    std::remove(cellVec.begin(), cellVec.end(), enemy),
                    cellVec.end()
                );
            }
            
            // Remove from tracking map
            enemyCellIndices.erase(it);
        }
        catch (const std::exception& e) {
            std::cout << "Exception in RemoveEnemy: " << e.what() << std::endl;
        }
    }
    
    // Update enemy position in the grid
    void UpdateEnemyPosition(EnemyBase* enemy, const sf::Vector2f& oldPosition) {
        if (!enemy) return;
        
        try {
            // Calculate old cell
            int oldCellX = static_cast<int>(std::floor(oldPosition.x / cellSize));
            int oldCellY = static_cast<int>(std::floor(oldPosition.y / cellSize));
            
            // Calculate new cell based on current position
            sf::Vector2f newPosition = enemy->GetPosition();
            int newCellX = static_cast<int>(std::floor(newPosition.x / cellSize));
            int newCellY = static_cast<int>(std::floor(newPosition.y / cellSize));
            
            // If position is invalid, skip update
            if (std::isnan(newPosition.x) || std::isnan(newPosition.y) ||
                std::isinf(newPosition.x) || std::isinf(newPosition.y)) {
                std::cout << "Warning: Invalid position detected in UpdateEnemyPosition" << std::endl;
                return;
            }
            
            // If cell hasn't changed, nothing to do
            if (oldCellX == newCellX && oldCellY == newCellY) return;
            
            // Ensure bounds for old cell
            oldCellX = std::max(0, std::min(gridWidth - 1, oldCellX));
            oldCellY = std::max(0, std::min(gridHeight - 1, oldCellY));
            
            // Calculate old cell index
            int oldCellIndex = oldCellY * gridWidth + oldCellX;
            
            // Ensure bounds for new cell
            newCellX = std::max(0, std::min(gridWidth - 1, newCellX));
            newCellY = std::max(0, std::min(gridHeight - 1, newCellY));
            
            // Calculate new cell index
            int newCellIndex = newCellY * gridWidth + newCellX;
            
            // Make sure cell indices are valid
            if (oldCellIndex < 0 || oldCellIndex >= cells.size() ||
                newCellIndex < 0 || newCellIndex >= cells.size()) {
                return;
            }
            
            // Get auto iterate handle from map
            auto it = enemyCellIndices.find(enemy);
            
            // If enemy is already in the grid
            if (it != enemyCellIndices.end()) {
                // Get current cell index from the map
                int currentCellIndex = it->second;
                
                // Remove from current cell if it's different from the new cell
                if (currentCellIndex != newCellIndex && currentCellIndex >= 0 && currentCellIndex < cells.size()) {
                    auto& currentCellVec = cells[currentCellIndex];
                    currentCellVec.erase(
                        std::remove(currentCellVec.begin(), currentCellVec.end(), enemy),
                        currentCellVec.end()
                    );
                }
                
                // Add to new cell
                cells[newCellIndex].push_back(enemy);
                
                // Update the map with new cell index
                it->second = newCellIndex;
            }
            else {
                // Enemy wasn't found in the map, add it
                cells[newCellIndex].push_back(enemy);
                enemyCellIndices[enemy] = newCellIndex;
            }
        }
        catch (const std::exception& e) {
            std::cout << "Exception in UpdateEnemyPosition: " << e.what() << std::endl;
        }
    }
    
    // Get enemies near a position within a radius
    // Get enemies near a position within a radius
std::vector<EnemyBase*> GetNearbyEnemies(const sf::Vector2f& position, float radius) {
    std::vector<EnemyBase*> nearbyEnemies;
    
    try {
        // Calculate the grid cells that overlap with the search area
        int minCellX = static_cast<int>((position.x - radius) / cellSize);
        int maxCellX = static_cast<int>((position.x + radius) / cellSize);
        int minCellY = static_cast<int>((position.y - radius) / cellSize);
        int maxCellY = static_cast<int>((position.y + radius) / cellSize);
        
        // Clamp to grid boundaries
        minCellX = std::max(0, minCellX);
        maxCellX = std::min(gridWidth - 1, maxCellX);
        minCellY = std::max(0, minCellY);
        maxCellY = std::min(gridHeight - 1, maxCellY);
        
        // Limit the search area to avoid excessive memory allocation
        // Don't search more than 10x10 cells at once
        if (maxCellX - minCellX > 10) {
            maxCellX = minCellX + 10;
        }
        if (maxCellY - minCellY > 10) {
            maxCellY = minCellY + 10;
        }
        
        // Reserve a reasonable amount of memory (maximum 100 enemies per call)
        // This prevents the "vector too long" exception
        nearbyEnemies.reserve(std::min(100, (maxCellX - minCellX + 1) * (maxCellY - minCellY + 1) * 2));
        
        // Track how many enemies we've added to avoid excessive memory use
        int enemyCount = 0;
        const int MAX_ENEMIES = 100;
        
        // Collect enemies from all relevant cells
        for (int y = minCellY; y <= maxCellY && enemyCount < MAX_ENEMIES; y++) {
            for (int x = minCellX; x <= maxCellX && enemyCount < MAX_ENEMIES; x++) {
                int cellIndex = y * gridWidth + x;
                if (cellIndex >= 0 && cellIndex < cells.size()) {
                    const auto& cellEnemies = cells[cellIndex];
                    // Only add enemies if we won't exceed our max count
                    if (enemyCount + cellEnemies.size() <= MAX_ENEMIES) {
                        nearbyEnemies.insert(nearbyEnemies.end(), cellEnemies.begin(), cellEnemies.end());
                        enemyCount += cellEnemies.size();
                    } else {
                        // Add enemies until we reach the max
                        for (auto enemy : cellEnemies) {
                            if (enemyCount < MAX_ENEMIES) {
                                nearbyEnemies.push_back(enemy);
                                enemyCount++;
                            } else {
                                break;
                            }
                        }
                    }
                }
            }
        }
        
        // Remove duplicates if needed
        if (nearbyEnemies.size() > 1) {
            std::sort(nearbyEnemies.begin(), nearbyEnemies.end());
            nearbyEnemies.erase(std::unique(nearbyEnemies.begin(), nearbyEnemies.end()), nearbyEnemies.end());
        }
        
        // Filter by exact distance if needed
        if (radius > cellSize) {
            nearbyEnemies.erase(
                std::remove_if(nearbyEnemies.begin(), nearbyEnemies.end(), 
                             [&position, radius](EnemyBase* enemy) {
                                 if (!enemy) return true;
                                 sf::Vector2f enemyPos = enemy->GetPosition();
                                 float dx = enemyPos.x - position.x;
                                 float dy = enemyPos.y - position.y;
                                 float distSquared = dx * dx + dy * dy;
                                 return distSquared > radius * radius;
                             }),
                nearbyEnemies.end()
            );
        }
    }
    catch (const std::exception& e) {
        std::cout << "Exception in GetNearbyEnemies: " << e.what() << std::endl;
        nearbyEnemies.clear();
    }
    
    return nearbyEnemies;
}
    // Get enemies in a specified view rectangle
    std::vector<EnemyBase*> GetEnemiesInView(const sf::FloatRect& viewBounds) {
        std::vector<EnemyBase*> visibleEnemies;
        
        try {
            // Calculate the range of cells covered by the view
            int minCellX = static_cast<int>(viewBounds.left / cellSize);
            int maxCellX = static_cast<int>((viewBounds.left + viewBounds.width) / cellSize);
            int minCellY = static_cast<int>(viewBounds.top / cellSize);
            int maxCellY = static_cast<int>((viewBounds.top + viewBounds.height) / cellSize);
            
            // Clamp to grid boundaries
            minCellX = std::max(0, minCellX);
            maxCellX = std::min(gridWidth - 1, maxCellX);
            minCellY = std::max(0, minCellY);
            maxCellY = std::min(gridHeight - 1, maxCellY);
            
            // Reserve space for efficiency
            visibleEnemies.reserve((maxCellX - minCellX + 1) * (maxCellY - minCellY + 1) * 4);
            
            // Collect enemies from all cells in the view
            for (int y = minCellY; y <= maxCellY; y++) {
                for (int x = minCellX; x <= maxCellX; x++) {
                    int cellIndex = y * gridWidth + x;
                    if (cellIndex >= 0 && cellIndex < cells.size()) {
                        const auto& cellEnemies = cells[cellIndex];
                        visibleEnemies.insert(visibleEnemies.end(), cellEnemies.begin(), cellEnemies.end());
                    }
                }
            }
            
            // Remove duplicates if needed
            if (visibleEnemies.size() > 1) {
                std::sort(visibleEnemies.begin(), visibleEnemies.end());
                visibleEnemies.erase(std::unique(visibleEnemies.begin(), visibleEnemies.end()), visibleEnemies.end());
            }
        }
        catch (const std::exception& e) {
            std::cout << "Exception in GetEnemiesInView: " << e.what() << std::endl;
            visibleEnemies.clear();
        }
        
        return visibleEnemies;
    }
    
    // Clear the grid
    void Clear() {
        try {
            cells.clear();
            cells.resize(gridWidth * gridHeight);
            enemyCellIndices.clear();
        }
        catch (const std::exception& e) {
            std::cout << "Exception in Clear: " << e.what() << std::endl;
        }
    }
    
private:
    float cellSize;
    sf::Vector2f worldSize;
    int gridWidth;
    int gridHeight;
    
    // Vector of vectors for each cell
    std::vector<std::vector<EnemyBase*>> cells;
    
    // Map of enemies to their cell indices for quick lookups
    std::unordered_map<EnemyBase*, int> enemyCellIndices;
};

#endif // SPATIAL_GRID_H