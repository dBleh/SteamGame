#pragma once

#include <vector>
#include <unordered_map>
#include <functional>
#include <string>
#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/Rect.hpp>

// Forward declaration
class TriangleEnemy;

// Spatial partitioning grid for efficient collision detection
class SpatialGrid {
public:
    // Constructor with cell size and world bounds
    SpatialGrid(float cellSize = 100.f, sf::Vector2f worldSize = sf::Vector2f(3000.f, 3000.f));
    
    // Add enemy to the grid
    void AddEnemy(TriangleEnemy* enemy);
    
    // Remove enemy from the grid
    void RemoveEnemy(TriangleEnemy* enemy);
    
    // Update enemy position in the grid
    void UpdateEnemyPosition(TriangleEnemy* enemy);
    
    // Clear all enemies from the grid
    void Clear();
    
    // Get enemies in a cell
    std::vector<TriangleEnemy*> GetEnemiesInCell(int cellX, int cellY) const;
    
    // Get enemies near a position (within a radius)
    std::vector<TriangleEnemy*> GetEnemiesNearPosition(const sf::Vector2f& position, float radius) const;
    
    // Get all enemies in a rectangle area
    std::vector<TriangleEnemy*> GetEnemiesInRect(const sf::FloatRect& rect) const;
    
    // Query the grid using a custom function
    std::vector<TriangleEnemy*> Query(const std::function<bool(TriangleEnemy*)>& predicate) const;
    
    // Get total enemy count
    size_t GetEnemyCount() const;
    
private:
    float cellSize;
    sf::Vector2f worldSize;
    
    // 2D grid: Maps cell coordinates to a list of enemies in that cell
    // Using a string key "x,y" for simplicity
    std::unordered_map<std::string, std::vector<TriangleEnemy*>> grid;
    
    // Mapping from enemy pointer to its current cell
    std::unordered_map<TriangleEnemy*, std::string> enemyCells;
    
    // Helper methods
    void PositionToCell(const sf::Vector2f& position, int& cellX, int& cellY) const;
    std::string GetCellKey(int cellX, int cellY) const;
    std::vector<std::string> GetNeighboringCells(const sf::Vector2f& position, float radius) const;
};