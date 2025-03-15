#ifndef SPATIAL_GRID_H
#define SPATIAL_GRID_H

#include <SFML/Graphics.hpp>
#include <unordered_map>
#include <vector>
#include <cmath>
#include <sstream>
#include <functional>
#include <algorithm>
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
        
        // Initialize the grid
        Clear();
    }
    
    // Add an enemy to the grid
    void AddEnemy(EnemyBase* enemy) {
        if (!enemy) return;
        
        // Get enemy position
        sf::Vector2f position = enemy->GetPosition();
        
        // Calculate grid cell coordinates
        int cellX = static_cast<int>(std::floor(position.x / cellSize));
        int cellY = static_cast<int>(std::floor(position.y / cellSize));
        
        // Ensure within grid bounds
        if (cellX < 0 || cellX >= gridWidth || cellY < 0 || cellY >= gridHeight) {
            // Enemy is outside the grid, skip or place at boundary
            cellX = std::max(0, std::min(gridWidth - 1, cellX));
            cellY = std::max(0, std::min(gridHeight - 1, cellY));
        }
        
        // Add to cell vector
        int cellIndex = cellY * gridWidth + cellX;
        if (cells.size() <= cellIndex) {
            cells.resize(gridWidth * gridHeight);
        }
        
        // Avoid adding duplicates
        auto& cellVec = cells[cellIndex];
        if (std::find(cellVec.begin(), cellVec.end(), enemy) == cellVec.end()) {
            cellVec.push_back(enemy);
        }
        
        // Store which cell the enemy is in for fast lookups
        enemyCellIndices[enemy] = cellIndex;
    }
    
    // Remove an enemy from the grid
    void RemoveEnemy(EnemyBase* enemy) {
        if (!enemy) return;
        
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
    
    // Update enemy position in the grid efficiently
    // Update enemy position in the grid efficiently
void UpdateEnemyPosition(EnemyBase* enemy, const sf::Vector2f& oldPosition) {
    if (!enemy) return;
    
    // Calculate old cell
    int oldCellX = static_cast<int>(std::floor(oldPosition.x / cellSize));
    int oldCellY = static_cast<int>(std::floor(oldPosition.y / cellSize));
    
    // Calculate new cell based on current position
    sf::Vector2f newPosition = enemy->GetPosition();
    int newCellX = static_cast<int>(std::floor(newPosition.x / cellSize));
    int newCellY = static_cast<int>(std::floor(newPosition.y / cellSize));
    
    // If cell hasn't changed, nothing to do
    if (oldCellX == newCellX && oldCellY == newCellY) return;
    
    // Ensure bounds for old cell
    oldCellX = std::max(0, std::min(gridWidth - 1, oldCellX));
    oldCellY = std::max(0, std::min(gridHeight - 1, oldCellY));
    
    // Calculate old cell index
    int oldCellIndex = oldCellY * gridWidth + oldCellX;
    
    // Remove from old cell
    if (oldCellIndex >= 0 && oldCellIndex < cells.size()) {
        auto& oldCellVec = cells[oldCellIndex];
        oldCellVec.erase(
            std::remove(oldCellVec.begin(), oldCellVec.end(), enemy),
            oldCellVec.end()
        );
    }
    
    // Ensure bounds for new cell
    newCellX = std::max(0, std::min(gridWidth - 1, newCellX));
    newCellY = std::max(0, std::min(gridHeight - 1, newCellY));
    
    // Calculate new cell index
    int newCellIndex = newCellY * gridWidth + newCellX;
    
    // Add to new cell
    if (newCellIndex >= 0 && newCellIndex < cells.size()) {
        cells[newCellIndex].push_back(enemy);
        // Update the tracking map
        enemyCellIndices[enemy] = newCellIndex;
    }
}
    
    // Get enemies near a position within a radius
    std::vector<EnemyBase*> GetNearbyEnemies(const sf::Vector2f& position, float radius) {
        std::vector<EnemyBase*> nearbyEnemies;
        
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
        
        // Reserve reasonable capacity to avoid frequent reallocations
        nearbyEnemies.reserve((maxCellX - minCellX + 1) * (maxCellY - minCellY + 1) * 4);
        
        // Collect enemies from all relevant cells
        for (int y = minCellY; y <= maxCellY; y++) {
            for (int x = minCellX; x <= maxCellX; x++) {
                int cellIndex = y * gridWidth + x;
                if (cellIndex >= 0 && cellIndex < cells.size()) {
                    const auto& cellEnemies = cells[cellIndex];
                    nearbyEnemies.insert(nearbyEnemies.end(), cellEnemies.begin(), cellEnemies.end());
                }
            }
        }
        
        // Remove duplicates if needed
        if (nearbyEnemies.size() > 1) {
            std::sort(nearbyEnemies.begin(), nearbyEnemies.end());
            nearbyEnemies.erase(std::unique(nearbyEnemies.begin(), nearbyEnemies.end()), nearbyEnemies.end());
        }
        
        // Filter by exact distance if needed (optional for better accuracy)
        if (radius > cellSize) {
            nearbyEnemies.erase(
                std::remove_if(nearbyEnemies.begin(), nearbyEnemies.end(), 
                              [&position, radius](EnemyBase* enemy) {
                                  sf::Vector2f enemyPos = enemy->GetPosition();
                                  float dx = enemyPos.x - position.x;
                                  float dy = enemyPos.y - position.y;
                                  float distSquared = dx * dx + dy * dy;
                                  return distSquared > radius * radius;
                              }),
                nearbyEnemies.end()
            );
        }
        
        return nearbyEnemies;
    }
    
    // Get enemies in a specified view rectangle
    std::vector<EnemyBase*> GetEnemiesInView(const sf::FloatRect& viewBounds) {
        std::vector<EnemyBase*> visibleEnemies;
        
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
        
        return visibleEnemies;
    }
    
    // Clear the grid
    void Clear() {
        cells.clear();
        cells.resize(gridWidth * gridHeight);
        enemyCellIndices.clear();
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