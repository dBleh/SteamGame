#include "Grid.h"
#include <cmath>

Grid::Grid(float cellSize, sf::Color lineColor)
    : cellSize(cellSize), lineColor(lineColor), 
      minorLines(sf::Lines), majorLines(sf::Lines) {
    
    // Initialize major line color as a slightly darker variant of regular line color
    majorLineColor = sf::Color(
        std::max(0, static_cast<int>(lineColor.r * 0.7f)),
        std::max(0, static_cast<int>(lineColor.g * 0.7f)),
        std::max(0, static_cast<int>(lineColor.b * 0.7f)),
        lineColor.a
    );
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
    
    // Draw grid (minor lines first, then major lines on top)
    sf::RenderStates minorStates;
    window.draw(minorLines, minorStates);
    
    sf::RenderStates majorStates;
    window.draw(majorLines, majorStates);
    
    // Draw origin highlight if enabled
    if (highlightOrigin) {
        drawOriginHighlight(window);
    }
}

void Grid::updateGridLines(const sf::FloatRect& viewBounds) {
    minorLines.clear();
    majorLines.clear();
    
    // Add padding to ensure lines extend beyond screen edges
    float padding = cellSize * 2; // Increased padding for smoother scrolling
    sf::FloatRect extendedBounds(
        viewBounds.left - padding,
        viewBounds.top - padding,
        viewBounds.width + 2 * padding,
        viewBounds.height + 2 * padding
    );
    
    // Ensure we're drawing lines at whole number multiples of cellSize
    int startX = static_cast<int>(std::floor(extendedBounds.left / cellSize));
    int endX = static_cast<int>(std::ceil((extendedBounds.left + extendedBounds.width) / cellSize));
    int startY = static_cast<int>(std::floor(extendedBounds.top / cellSize));
    int endY = static_cast<int>(std::ceil((extendedBounds.top + extendedBounds.height) / cellSize));
    
    // Vertical lines
    for (int i = startX; i <= endX; ++i) {
        float x = i * cellSize;
        
        // Determine if this is a major grid line
        if (isMajorLine(i)) {
            sf::Vertex lineStart(sf::Vector2f(x, extendedBounds.top), majorLineColor);
            sf::Vertex lineEnd(sf::Vector2f(x, extendedBounds.top + extendedBounds.height), majorLineColor);
            majorLines.append(lineStart);
            majorLines.append(lineEnd);
        } else {
            sf::Vertex lineStart(sf::Vector2f(x, extendedBounds.top), lineColor);
            sf::Vertex lineEnd(sf::Vector2f(x, extendedBounds.top + extendedBounds.height), lineColor);
            minorLines.append(lineStart);
            minorLines.append(lineEnd);
        }
    }
    
    // Horizontal lines
    for (int i = startY; i <= endY; ++i) {
        float y = i * cellSize;
        
        // Determine if this is a major grid line
        if (isMajorLine(i)) {
            sf::Vertex lineStart(sf::Vector2f(extendedBounds.left, y), majorLineColor);
            sf::Vertex lineEnd(sf::Vector2f(extendedBounds.left + extendedBounds.width, y), majorLineColor);
            majorLines.append(lineStart);
            majorLines.append(lineEnd);
        } else {
            sf::Vertex lineStart(sf::Vector2f(extendedBounds.left, y), lineColor);
            sf::Vertex lineEnd(sf::Vector2f(extendedBounds.left + extendedBounds.width, y), lineColor);
            minorLines.append(lineStart);
            minorLines.append(lineEnd);
        }
    }
}

bool Grid::isMajorLine(int index) const {
    // Consider the origin (0) always a major line
    if (index == 0) return true;
    
    // Otherwise check if it's a multiple of the major line interval
    return (index % majorLineInterval) == 0;
}

void Grid::drawOriginHighlight(sf::RenderWindow& window) {
    // Create a cross at the origin point
    sf::RectangleShape horizontalLine;
    horizontalLine.setSize(sf::Vector2f(originHighlightSize * 2, originHighlightSize / 5));
    horizontalLine.setOrigin(originHighlightSize, originHighlightSize / 10);
    horizontalLine.setPosition(0, 0);
    horizontalLine.setFillColor(originHighlightColor);
    
    sf::RectangleShape verticalLine;
    verticalLine.setSize(sf::Vector2f(originHighlightSize / 5, originHighlightSize * 2));
    verticalLine.setOrigin(originHighlightSize / 10, originHighlightSize);
    verticalLine.setPosition(0, 0);
    verticalLine.setFillColor(originHighlightColor);
    

    
    window.draw(horizontalLine);
    window.draw(verticalLine);
    
}

void Grid::setLineColor(const sf::Color& color) {
    lineColor = color;
    
    // Update major line color based on the new line color
    majorLineColor = sf::Color(
        std::max(0, static_cast<int>(color.r * 0.7f)),
        std::max(0, static_cast<int>(color.g * 0.7f)),
        std::max(0, static_cast<int>(color.b * 0.7f)),
        color.a
    );
}

void Grid::setCellSize(float size) {
    cellSize = size;
}

void Grid::setMajorLineInterval(int interval) {
    majorLineInterval = interval;
}

void Grid::setMajorLineColor(const sf::Color& color) {
    majorLineColor = color;
}

void Grid::setMajorLineThickness(float thickness) {
    majorLineThickness = thickness;
}

void Grid::setMinorLineThickness(float thickness) {
    minorLineThickness = thickness;
}

void Grid::setOriginHighlight(bool highlight) {
    highlightOrigin = highlight;
}

void Grid::setOriginHighlightColor(const sf::Color& color) {
    originHighlightColor = color;
}

void Grid::setOriginHighlightSize(float size) {
    originHighlightSize = size;
}