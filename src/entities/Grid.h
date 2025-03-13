// Grid.h
#ifndef GRID_H
#define GRID_H

#include <SFML/Graphics.hpp>

class Grid {
public:
    Grid(float cellSize = 50.f, sf::Color lineColor = sf::Color(200, 200, 200));
    
    void render(sf::RenderWindow& window, const sf::View& view);
    void setLineColor(const sf::Color& color);
    void setCellSize(float size);
    
private:
    float cellSize;
    sf::Color lineColor;
    sf::VertexArray lines;
    
    void updateGridLines(const sf::FloatRect& viewBounds);
};

#endif // GRID_H