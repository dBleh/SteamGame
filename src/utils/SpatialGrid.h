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
        
        // Convert position to cell coordinates
        int cellX, cellY;
        PositionToCell(position, cellX, cellY);
        
        // Get cell key
        std::string cellKey = GetCellKey(cellX, cellY);
        
        // Add enemy to the cell
        grid[cellKey].push_back(enemy);
        
        // Store cell for the enemy
        enemyCells[enemy] = cellKey;
    }
    
    // Remove an enemy from the grid
    void RemoveEnemy(EnemyBase* enemy) {
        if (!enemy) return;
        
        // Get the cell the enemy is in
        auto it = enemyCells.find(enemy);
        if (it == enemyCells.end()) return;
        
        std::string cellKey = it->second;
        
        // Remove enemy from the cell
        auto& cell = grid[cellKey];
        cell.erase(std::remove(cell.begin(), cell.end(), enemy), cell.end());
        
        // Remove cell from enemy mapping
        enemyCells.erase(it);
    }
    
    // Update an enemy's position in the grid
    void UpdateEnemyPosition(EnemyBase* enemy) {
        if (!enemy) return;
        
        // Get enemy position
        sf::Vector2f position = enemy->GetPosition();
        
        // Convert position to cell coordinates
        int cellX, cellY;
        PositionToCell(position, cellX, cellY);
        
        // Get new cell key
        std::string newCellKey = GetCellKey(cellX, cellY);
        
        // Get the current cell the enemy is in
        auto it = enemyCells.find(enemy);
        if (it == enemyCells.end()) {
            // Enemy not in grid, add it
            AddEnemy(enemy);
            return;
        }
        
        std::string currentCellKey = it->second;
        
        // If the cell hasn't changed, do nothing
        if (currentCellKey == newCellKey) return;
        
        // Remove from current cell
        auto& currentCell = grid[currentCellKey];
        currentCell.erase(std::remove(currentCell.begin(), currentCell.end(), enemy), currentCell.end());
        
        // Add to new cell
        grid[newCellKey].push_back(enemy);
        
        // Update cell mapping
        enemyCells[enemy] = newCellKey;
    }
    
    // Clear the grid
    void Clear() {
        grid.clear();
        enemyCells.clear();
    }
    
    // Get enemies in a specific cell
    std::vector<EnemyBase*> GetEnemiesInCell(int cellX, int cellY) const {
        std::string cellKey = GetCellKey(cellX, cellY);
        
        auto it = grid.find(cellKey);
        if (it != grid.end()) {
            return it->second;
        }
        
        return std::vector<EnemyBase*>();
    }
    
    // Get enemies near a position within a radius
    std::vector<EnemyBase*> GetEnemiesNearPosition(const sf::Vector2f& position, float radius) const {
        std::vector<EnemyBase*> result;
        
        // Get neighboring cells
        std::vector<std::string> cells = GetNeighboringCells(position, radius);
        
        // Collect enemies from all neighboring cells
        for (const auto& cellKey : cells) {
            auto it = grid.find(cellKey);
            if (it != grid.end()) {
                const auto& cellEnemies = it->second;
                result.insert(result.end(), cellEnemies.begin(), cellEnemies.end());
            }
        }
        
        // Remove duplicates
        std::sort(result.begin(), result.end());
        result.erase(std::unique(result.begin(), result.end()), result.end());
        
        // Filter by actual distance
        result.erase(
            std::remove_if(result.begin(), result.end(), 
                          [&position, radius](EnemyBase* enemy) {
                              sf::Vector2f enemyPos = enemy->GetPosition();
                              float dx = enemyPos.x - position.x;
                              float dy = enemyPos.y - position.y;
                              float distSquared = dx * dx + dy * dy;
                              return distSquared > radius * radius;
                          }),
            result.end()
        );
        
        return result;
    }
    
    // Get enemies in a rectangle
    std::vector<EnemyBase*> GetEnemiesInRect(const sf::FloatRect& rect) const {
        std::vector<EnemyBase*> result;
        
        // Calculate the range of cells covered by the rectangle
        int minCellX, minCellY, maxCellX, maxCellY;
        
        PositionToCell({rect.left, rect.top}, minCellX, minCellY);
        PositionToCell({rect.left + rect.width, rect.top + rect.height}, maxCellX, maxCellY);
        
        // Collect enemies from all cells in the range
        for (int x = minCellX; x <= maxCellX; ++x) {
            for (int y = minCellY; y <= maxCellY; ++y) {
                std::string cellKey = GetCellKey(x, y);
                auto it = grid.find(cellKey);
                if (it != grid.end()) {
                    const auto& cellEnemies = it->second;
                    result.insert(result.end(), cellEnemies.begin(), cellEnemies.end());
                }
            }
        }
        
        // Remove duplicates
        std::sort(result.begin(), result.end());
        result.erase(std::unique(result.begin(), result.end()), result.end());
        
        // Keep only enemies that are within the view bounds
        result.erase(
            std::remove_if(result.begin(), result.end(), 
                          [&rect](EnemyBase* enemy) {
                              return !rect.contains(enemy->GetPosition());
                          }),
            result.end()
        );
        
        return result;
    }
    
    // Query enemies matching a predicate
    std::vector<EnemyBase*> Query(const std::function<bool(EnemyBase*)>& predicate) const {
        std::vector<EnemyBase*> result;
        
        // Collect all enemies that satisfy the predicate
        for (const auto& pair : grid) {
            const auto& cellEnemies = pair.second;
            for (EnemyBase* enemy : cellEnemies) {
                if (predicate(enemy)) {
                    result.push_back(enemy);
                }
            }
        }
        
        // Remove duplicates
        std::sort(result.begin(), result.end());
        result.erase(std::unique(result.begin(), result.end()), result.end());
        
        return result;
    }
    
    // Get the number of enemies in the grid
    size_t GetEnemyCount() const {
        return enemyCells.size();
    }
    
private:
    float cellSize;
    sf::Vector2f worldSize;
    int gridWidth;
    int gridHeight;
    
    // Map of cell keys to enemies in that cell
    std::unordered_map<std::string, std::vector<EnemyBase*>> grid;
    
    // Map of enemies to their cell keys
    std::unordered_map<EnemyBase*, std::string> enemyCells;
    
    // Convert a world position to cell coordinates
    void PositionToCell(const sf::Vector2f& position, int& cellX, int& cellY) const {
        cellX = static_cast<int>(std::floor(position.x / cellSize));
        cellY = static_cast<int>(std::floor(position.y / cellSize));
    }
    
    // Get a string key for a cell
    std::string GetCellKey(int cellX, int cellY) const {
        std::stringstream ss;
        ss << cellX << "," << cellY;
        return ss.str();
    }
    
    // Get all cell keys within a radius of a position
    std::vector<std::string> GetNeighboringCells(const sf::Vector2f& position, float radius) const {
        std::vector<std::string> cells;
        
        // Calculate the range of cells within the radius
        int centerCellX, centerCellY;
        PositionToCell(position, centerCellX, centerCellY);
        
        int cellRadius = static_cast<int>(std::ceil(radius / cellSize));
        
        for (int x = centerCellX - cellRadius; x <= centerCellX + cellRadius; ++x) {
            for (int y = centerCellY - cellRadius; y <= centerCellY + cellRadius; ++y) {
                cells.push_back(GetCellKey(x, y));
            }
        }
        
        return cells;
    }
};

#endif // SPATIAL_GRID_H