// Grid.cpp
#include "Grid.h"
#include <cmath>

Grid::Grid(float cellSize, sf::Color lineColor)
    : cellSize(cellSize), lineColor(lineColor), lines(sf::Lines) {
}

void Grid::render(sf::RenderWindow& window, const sf::View& view) {
    // Get the visible area
    sf::Vector2f viewCenter = view.getCenter();
    sf::Vector2f viewSize = view.getSize();
    sf::FloatRect viewBounds(
        viewCenter.x - viewSize.x / 2.f,
        viewCenter.y - viewSize.y / 2.f,
        viewSize.x,
        viewSize.y
    );
    
    // Update grid lines based on current view
    updateGridLines(viewBounds);
    
    // Draw grid
    sf::RenderStates states;
    window.draw(lines, states);
}

void Grid::updateGridLines(const sf::FloatRect& viewBounds) {
    lines.clear();
    
    // Add padding to ensure lines extend beyond screen edges
    float padding = cellSize;
    sf::FloatRect extendedBounds(
        viewBounds.left - padding,
        viewBounds.top - padding,
        viewBounds.width + 2 * padding,
        viewBounds.height + 2 * padding
    );
    
    // Calculate the start and end indices for grid lines
    int startX = static_cast<int>(std::floor(extendedBounds.left / cellSize));
    int endX = static_cast<int>(std::ceil((extendedBounds.left + extendedBounds.width) / cellSize));
    int startY = static_cast<int>(std::floor(extendedBounds.top / cellSize));
    int endY = static_cast<int>(std::ceil((extendedBounds.top + extendedBounds.height) / cellSize));
    
    // Vertical lines
    for (int i = startX; i <= endX; ++i) {
        float x = i * cellSize;
        sf::Vertex lineStart(sf::Vector2f(x, extendedBounds.top), lineColor);
        sf::Vertex lineEnd(sf::Vector2f(x, extendedBounds.top + extendedBounds.height), lineColor);
        lines.append(lineStart);
        lines.append(lineEnd);
    }
    
    // Horizontal lines
    for (int i = startY; i <= endY; ++i) {
        float y = i * cellSize;
        sf::Vertex lineStart(sf::Vector2f(extendedBounds.left, y), lineColor);
        sf::Vertex lineEnd(sf::Vector2f(extendedBounds.left + extendedBounds.width, y), lineColor);
        lines.append(lineStart);
        lines.append(lineEnd);
    }
}

void Grid::setLineColor(const sf::Color& color) {
    lineColor = color;
}

void Grid::setCellSize(float size) {
    cellSize = size;
}