#ifndef GRID_H
#define GRID_H

#include <SFML/Graphics.hpp>

class Grid {
public:
    Grid(float cellSize = 50.f, sf::Color lineColor = sf::Color(200, 200, 200));
    
    void render(sf::RenderWindow& window, const sf::View& view);
    void setLineColor(const sf::Color& color);
    void setCellSize(float size);
    
    // New styling options
    void setMajorLineInterval(int interval);
    void setMajorLineColor(const sf::Color& color);
    void setMajorLineThickness(float thickness);
    void setMinorLineThickness(float thickness);
    void setOriginHighlight(bool highlight);
    void setOriginHighlightColor(const sf::Color& color);
    void setOriginHighlightSize(float size);
    
private:
    float cellSize;
    sf::Color lineColor;
    sf::Color majorLineColor;
    sf::VertexArray minorLines;
    sf::VertexArray majorLines;
    
    int majorLineInterval = 5;      // Draw thicker line every X cells
    float minorLineThickness = 1.0f;
    float majorLineThickness = 2.0f;
    
    bool highlightOrigin = true;
    sf::Color originHighlightColor = sf::Color(255, 0, 0, 100); // Transparent red
    float originHighlightSize = 10.0f;
    
    void updateGridLines(const sf::FloatRect& viewBounds);
    bool isMajorLine(int index) const;
    void drawOriginHighlight(sf::RenderWindow& window);
};

#endif // GRID_H