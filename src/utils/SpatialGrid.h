#ifndef SPATIAL_GRID_H
#define SPATIAL_GRID_H

#include <SFML/Graphics.hpp>
#include <unordered_map>
#include <vector>
#include <cmath>
#include <iostream>
#include <algorithm>
#include <unordered_set>
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
        
        // Initialize cells vector with the right size
        cells.resize(gridWidth * gridHeight);
        
        std::cout << "SpatialGrid initialized with " << gridWidth << "x" << gridHeight << " cells (" 
                 << cells.size() << " total cells)" << std::endl;
    }
    
    // Add an enemy to the grid
    void AddEnemy(EnemyBase* enemy) {
        if (!enemy) return;
        
        try {
            // Get enemy position
            sf::Vector2f position = enemy->GetPosition();
            
            // Calculate cell index
            int cellX = GetCellX(position.x);
            int cellY = GetCellY(position.y);
            int cellIndex = GetCellIndex(cellX, cellY);
            
            // Add to the appropriate cell
            if (cellIndex >= 0 && cellIndex < cells.size()) {
                // Add to cell if not already there
                auto& cellEnemies = cells[cellIndex];
                if (std::find(cellEnemies.begin(), cellEnemies.end(), enemy) == cellEnemies.end()) {
                    cellEnemies.push_back(enemy);
                }
                
                // Update tracking map
                enemyCellIndices[enemy] = cellIndex;
            }
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
            
            // Get the cell index
            int cellIndex = it->second;
            
            // Remove from that cell
            if (cellIndex >= 0 && cellIndex < cells.size()) {
                auto& cellEnemies = cells[cellIndex];
                cellEnemies.erase(
                    std::remove(cellEnemies.begin(), cellEnemies.end(), enemy),
                    cellEnemies.end()
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
            // Remove from old cell
            RemoveEnemy(enemy);
            
            // Add to new cell based on current position
            AddEnemy(enemy);
        }
        catch (const std::exception& e) {
            std::cout << "Exception in UpdateEnemyPosition: " << e.what() << std::endl;
        }
    }
    
    // Get enemies near a position within a radius
    std::vector<EnemyBase*> GetNearbyEnemies(const sf::Vector2f& position, float radius) {
        // Use a set to automatically prevent duplicates
        std::unordered_set<EnemyBase*> enemySet;
        
        try {
            // Calculate the grid cells that overlap with the search area
            int minCellX = GetCellX(position.x - radius);
            int maxCellX = GetCellX(position.x + radius);
            int minCellY = GetCellY(position.y - radius);
            int maxCellY = GetCellY(position.y + radius);
            
            // Limit the search area to avoid excessive computation
            const int MAX_CELLS = 5;
            if (maxCellX - minCellX > MAX_CELLS) {
                maxCellX = minCellX + MAX_CELLS;
            }
            if (maxCellY - minCellY > MAX_CELLS) {
                maxCellY = minCellY + MAX_CELLS;
            }
            
            // Collect enemies from cells in the search area
            for (int y = minCellY; y <= maxCellY; y++) {
                for (int x = minCellX; x <= maxCellX; x++) {
                    int cellIndex = GetCellIndex(x, y);
                    if (cellIndex >= 0 && cellIndex < cells.size()) {
                        const auto& cellEnemies = cells[cellIndex];
                        enemySet.insert(cellEnemies.begin(), cellEnemies.end());
                        
                        // Stop if we're collecting too many enemies
                        if (enemySet.size() > 100) {
                            // Convert set to vector and return
                            return std::vector<EnemyBase*>(enemySet.begin(), enemySet.end());
                        }
                    }
                }
            }
        }
        catch (const std::exception& e) {
            std::cout << "Exception in GetNearbyEnemies: " << e.what() << std::endl;
            return std::vector<EnemyBase*>();
        }
        
        // Convert set to vector
        std::vector<EnemyBase*> result(enemySet.begin(), enemySet.end());
        
        // Filter by actual distance if needed
        if (radius > cellSize) {
            result.erase(
                std::remove_if(result.begin(), result.end(),
                             [&position, radius](EnemyBase* enemy) {
                                 if (!enemy) return true;
                                 sf::Vector2f enemyPos = enemy->GetPosition();
                                 float dx = enemyPos.x - position.x;
                                 float dy = enemyPos.y - position.y;
                                 float distSquared = dx * dx + dy * dy;
                                 return distSquared > radius * radius;
                             }),
                result.end()
            );
        }
        
        return result;
    }
    
    // Get enemies in a specified view rectangle
    std::vector<EnemyBase*> GetEnemiesInView(const sf::FloatRect& viewBounds) {
        // Use a set for automatic deduplication
        std::unordered_set<EnemyBase*> enemySet;
        
        try {
            // Calculate the range of cells covered by the view
            int minCellX = GetCellX(viewBounds.left);
            int maxCellX = GetCellX(viewBounds.left + viewBounds.width);
            int minCellY = GetCellY(viewBounds.top);
            int maxCellY = GetCellY(viewBounds.top + viewBounds.height);
            
            // Collect enemies from cells in the view
            for (int y = minCellY; y <= maxCellY; y++) {
                for (int x = minCellX; x <= maxCellX; x++) {
                    int cellIndex = GetCellIndex(x, y);
                    if (cellIndex >= 0 && cellIndex < cells.size()) {
                        const auto& cellEnemies = cells[cellIndex];
                        enemySet.insert(cellEnemies.begin(), cellEnemies.end());
                    }
                }
            }
        }
        catch (const std::exception& e) {
            std::cout << "Exception in GetEnemiesInView: " << e.what() << std::endl;
            return std::vector<EnemyBase*>();
        }
        
        // Convert set to vector
        return std::vector<EnemyBase*>(enemySet.begin(), enemySet.end());
    }
    
    // Clear the grid
    void Clear() {
        try {
            for (auto& cell : cells) {
                cell.clear();
            }
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
    
    // Helper functions to convert positions to cell coordinates
    inline int GetCellX(float x) const {
        int cellX = static_cast<int>(std::floor(x / cellSize));
        return std::max(0, std::min(gridWidth - 1, cellX));
    }
    
    inline int GetCellY(float y) const {
        int cellY = static_cast<int>(std::floor(y / cellSize));
        return std::max(0, std::min(gridHeight - 1, cellY));
    }
    
    inline int GetCellIndex(int cellX, int cellY) const {
        // Ensure coordinates are in bounds
        cellX = std::max(0, std::min(gridWidth - 1, cellX));
        cellY = std::max(0, std::min(gridHeight - 1, cellY));
        return cellY * gridWidth + cellX;
    }
};

#endif // SPATIAL_GRID_H