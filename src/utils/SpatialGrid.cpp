#include "SpatialGrid.h"
#include "../entities/TriangleEnemy.h"
#include <sstream>
#include <cmath>
#include <algorithm>

SpatialGrid::SpatialGrid(float cellSize, sf::Vector2f worldSize)
    : cellSize(cellSize), worldSize(worldSize)
{
}

void SpatialGrid::AddEnemy(TriangleEnemy* enemy)
{
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

void SpatialGrid::RemoveEnemy(TriangleEnemy* enemy)
{
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

void SpatialGrid::UpdateEnemyPosition(TriangleEnemy* enemy)
{
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

void SpatialGrid::Clear()
{
    grid.clear();
    enemyCells.clear();
}

std::vector<TriangleEnemy*> SpatialGrid::GetEnemiesInCell(int cellX, int cellY) const
{
    std::string cellKey = GetCellKey(cellX, cellY);
    
    auto it = grid.find(cellKey);
    if (it != grid.end()) {
        return it->second;
    }
    
    return std::vector<TriangleEnemy*>();
}

std::vector<TriangleEnemy*> SpatialGrid::GetEnemiesNearPosition(const sf::Vector2f& position, float radius) const
{
    std::vector<TriangleEnemy*> result;
    
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
    
    // Remove duplicates (an enemy might be in multiple cells)
    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());
    
    // Filter by actual distance (cells might include enemies outside the radius)
    result.erase(
        std::remove_if(result.begin(), result.end(), 
                      [&position, radius](TriangleEnemy* enemy) {
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

std::vector<TriangleEnemy*> SpatialGrid::GetEnemiesInRect(const sf::FloatRect& rect) const
{
    std::vector<TriangleEnemy*> result;
    
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
    
    // Filter by actual intersection
    result.erase(
        std::remove_if(result.begin(), result.end(), 
                      [&rect](TriangleEnemy* enemy) {
                          return !rect.intersects(enemy->GetShape().getGlobalBounds());
                      }),
        result.end()
    );
    
    return result;
}

std::vector<TriangleEnemy*> SpatialGrid::Query(const std::function<bool(TriangleEnemy*)>& predicate) const
{
    std::vector<TriangleEnemy*> result;
    
    // Collect all enemies that satisfy the predicate
    for (const auto& pair : grid) {
        const auto& cellEnemies = pair.second;
        for (TriangleEnemy* enemy : cellEnemies) {
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

size_t SpatialGrid::GetEnemyCount() const
{
    return enemyCells.size();
}

void SpatialGrid::PositionToCell(const sf::Vector2f& position, int& cellX, int& cellY) const
{
    cellX = static_cast<int>(std::floor(position.x / cellSize));
    cellY = static_cast<int>(std::floor(position.y / cellSize));
}

std::string SpatialGrid::GetCellKey(int cellX, int cellY) const
{
    std::stringstream ss;
    ss << cellX << "," << cellY;
    return ss.str();
}

std::vector<std::string> SpatialGrid::GetNeighboringCells(const sf::Vector2f& position, float radius) const
{
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