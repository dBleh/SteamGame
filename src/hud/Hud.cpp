#include "HUD.h"
#include <cmath>
#include <random>

//-------------------------------------------------------------------------
// Constructor
//-------------------------------------------------------------------------
HUD::HUD(sf::Font& font)
    : m_font(font)
{
    // Initialize random number generator with a time-based seed
    std::random_device rd;
    m_rng.seed(rd());
}

//-------------------------------------------------------------------------
// HUD Element Management
//-------------------------------------------------------------------------
void HUD::addElement(const std::string& id,
    const std::string& content,
    unsigned int size,
    sf::Vector2f pos,
    GameState visibleState,
    RenderMode mode,
    bool hoverable,
    const std::string& lineAboveId,
    const std::string& lineBelowId)
{
sf::Text text;
text.setFont(m_font);
text.setString(content);
text.setCharacterSize(size); // Fixed character size that won't change with scaling
text.setFillColor(sf::Color::Black);
text.setStyle(sf::Text::Regular);

// Make sure text is not affected by any scaling issues
// This can help prevent text from being resized when the view changes
text.setScale(1.0f, 1.0f);

HUDElement element;
element.text         = text;
element.pos          = pos;
element.visibleState = visibleState;
element.mode         = mode;
element.hoverable    = hoverable;
element.baseColor    = sf::Color::Black;
element.hoverColor   = sf::Color(100, 100, 100);
element.lineAbove    = lineAboveId;
element.lineBelow    = lineBelowId;
element.isHovered    = false;

// Store the original size to ensure it remains constant
element.originalCharSize = size;

m_elements[id] = element;
}

void HUD::addGradientLine(const std::string& id,
                        float xPos,
                        float yPos,
                        float width,
                        float thickness,
                        const sf::Color& color,
                        GameState visibleState,
                        RenderMode mode,
                        int segments)
{
    // Create a gradient line with segments that fade from center to edges
    GradientLineElement element;
    element.visibleState = visibleState;
    element.mode = mode;
    element.basePosition = sf::Vector2f(xPos, yPos);
    element.animationIntensity = 0.0f;
    element.animationTimer = 0.0f;
    
    // Calculate segment width
    float segmentWidth = width / segments;
    float halfSegments = segments / 2.0f;
    
    // Create segments from center outward with decreasing opacity
    for (int i = 0; i < segments; i++) {
        // Calculate opacity based on distance from center (0-255)
        float distanceFromCenter = std::abs(i - halfSegments + 0.5f) / halfSegments;
        float opacity = 255.0f * (1.0f - distanceFromCenter);
        
        // Create segment
        sf::RectangleShape segment(sf::Vector2f(segmentWidth, thickness));
        
        // Position from left to right, starting at the specified xPos
        float segmentXPos = xPos + (i * segmentWidth);
        segment.setPosition(segmentXPos, yPos);
        
        // Set color with calculated opacity
        sf::Color segmentColor = color;
        segmentColor.a = static_cast<sf::Uint8>(opacity);
        segment.setFillColor(segmentColor);
        
        // Add to segments vector
        element.segments.push_back(segment);
    }
    
    m_gradientLines[id] = element;
}

void HUD::setConnectedLines(const std::string& id, 
                          const std::string& lineAboveId, 
                          const std::string& lineBelowId)
{
    auto it = m_elements.find(id);
    if (it != m_elements.end()) {
        it->second.lineAbove = lineAboveId;
        it->second.lineBelow = lineBelowId;
    }
}

void HUD::updateText(const std::string& id, const std::string& content)
{
    auto it = m_elements.find(id);
    if (it != m_elements.end()) {
        it->second.text.setString(content);
    }
}

void HUD::updateBaseColor(const std::string& id, const sf::Color& color)
{
    auto it = m_elements.find(id);
    if (it != m_elements.end()) {
        it->second.baseColor = color;
        it->second.text.setFillColor(color);
    }
}

void HUD::updateElementPosition(const std::string& id, const sf::Vector2f& pos)
{
    auto it = m_elements.find(id);
    if (it != m_elements.end()) {
        it->second.pos = pos;
    }
}

void HUD::animateLine(const std::string& lineId, float intensity)
{
    auto it = m_gradientLines.find(lineId);
    if (it != m_gradientLines.end()) {
        it->second.animationIntensity = intensity;
    }
}

//-------------------------------------------------------------------------
// Update Method for Animation
//-------------------------------------------------------------------------


//-------------------------------------------------------------------------
// Rendering Methods
//-------------------------------------------------------------------------
void HUD::render(sf::RenderWindow& window, const sf::View& view, GameState currentState) {
    // Store the original view
    sf::View originalView = window.getView();
    
    // Set the view for UI rendering
    window.setView(view);
    
    // Render gradient line elements
    for (auto& [id, element] : m_gradientLines) {
        if (element.visibleState == currentState) {
            // Draw all segments
            for (const auto& segment : element.segments) {
                window.draw(segment);
            }
        }
    }

    // Render text elements
    for (auto& [id, element] : m_elements) {
        if (element.visibleState == currentState) {
            // Always restore the original character size before rendering
            element.text.setCharacterSize(element.originalCharSize);
            
            // Force scale to be 1.0 to prevent any scaling issues
            element.text.setScale(1.0f, 1.0f);
            
            // Position text at its designated position
            element.text.setPosition(element.pos);
            
            // Apply color based on hover state
            if (element.hoverable && element.isHovered)
                element.text.setFillColor(element.hoverColor);
            else
                element.text.setFillColor(element.baseColor);
            
            window.draw(element.text);
        }
    }
    
    // Restore original view
    window.setView(originalView);
}
void HUD::update(sf::RenderWindow& window, GameState currentState, float dt) {
    // Get window mouse position
    sf::Vector2i mousePos = sf::Mouse::getPosition(window);
    
    // Convert to UI coordinates using Game's conversion function
    // Note: This is conceptual - you would need to modify this to access the right method
    // You might need to pass a Game* reference to HUD or use another approach
    
    // This is a placeholder for the concept - we need window coordinates to UI coordinates conversion
    // sf::Vector2f mousePosUI = game->WindowToUICoordinates(mousePos);
    
    // Since we can't modify the function signature to add a Game reference easily,
    // we'll use a utility function inside HUD to convert mouse position
    sf::Vector2f mousePosUI = windowMouseToUICoordinates(window, mousePos);
    
    // Check for hover state changes
    for (auto& [id, element] : m_elements) {
        if (element.visibleState == currentState && element.hoverable) {
            // Create a text copy to check bounds
            sf::Text textCopy = element.text;
            textCopy.setPosition(element.pos);
            sf::FloatRect bounds = textCopy.getGlobalBounds();
            
            bool isHoveredNow = bounds.contains(mousePosUI);
            
            // If hover state changed
            if (isHoveredNow != element.isHovered) {
                element.isHovered = isHoveredNow;
                
                // Update text color
                element.text.setFillColor(isHoveredNow ? element.hoverColor : element.baseColor);
                
                // Start line animations if hovered
                if (isHoveredNow) {
                    if (!element.lineAbove.empty()) {
                        animateLine(element.lineAbove, 3.0f); // Full intensity
                    }
                    if (!element.lineBelow.empty()) {
                        animateLine(element.lineBelow, 3.0f); // Full intensity
                    }
                }
            }
        }
    }
    
    // Update all line animations
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    
    for (auto& [id, line] : m_gradientLines) {
        if (line.animationIntensity > 0) {
            // Update animation timer
            line.animationTimer += dt * 10.0f; // Controls animation speed
            
            // Apply random offsets to each segment for shake effect
            for (size_t i = 0; i < line.segments.size(); i++) {
                // Calculate random offset based on animation intensity
                float offsetX = dist(m_rng) * line.animationIntensity;
                float offsetY = dist(m_rng) * line.animationIntensity * 0.5f; // Less vertical shake
                
                // Get base position for this segment (original x + segment index * width)
                float baseX = line.basePosition.x + (i * (line.segments[i].getSize().x));
                float baseY = line.basePosition.y;
                
                // Apply offset to position
                line.segments[i].setPosition(baseX + offsetX, baseY + offsetY);
            }
            
            // Reduce intensity over time
            line.animationIntensity -= dt * 5.0f; // Controls fade-out speed
            if (line.animationIntensity < 0) {
                line.animationIntensity = 0;
                
                // Reset segments to base position when animation ends
                for (size_t i = 0; i < line.segments.size(); i++) {
                    float baseX = line.basePosition.x + (i * line.segments[i].getSize().x);
                    line.segments[i].setPosition(baseX, line.basePosition.y);
                }
            }
        }
    }
}
void HUD::drawWhiteBackground(sf::RenderWindow& window)
{
    sf::RectangleShape bg(sf::Vector2f(static_cast<float>(window.getSize().x), 
                                         static_cast<float>(window.getSize().y)));
    bg.setFillColor(sf::Color::White);
    bg.setPosition(0.f, 0.f);
    window.draw(bg);
}

bool HUD::isMouseOverText(const sf::RenderWindow& window, const sf::Text& text)
{
    sf::Vector2i mousePos = sf::Mouse::getPosition(window);
    sf::Vector2f mousePosF(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y));
    sf::FloatRect bounds = text.getGlobalBounds();
    return bounds.contains(mousePosF);
}

bool HUD::isFullyLoaded() const {
    return !m_elements.empty();
}

sf::Vector2f HUD::getElementPosition(const std::string& id) const
{
    auto it = m_elements.find(id);
    if (it != m_elements.end()) {
        return it->second.pos;
    }
    return sf::Vector2f(0.f, 0.f); // Return default position if element not found
}

//-------------------------------------------------------------------------
// HUD Configuration Methods
//-------------------------------------------------------------------------
void HUD::configureGameplayHUD(const sf::Vector2u& winSize) {
    // Implementation unchanged
}

void HUD::configureStoreHUD(const sf::Vector2u& winSize) {
    // Implementation unchanged
}

sf::Vector2f HUD::windowMouseToUICoordinates(const sf::RenderWindow& window, sf::Vector2i mousePos) const {
    // Get the current view
    sf::View currentView = window.getView();
    
    // Get the viewport of the view (in normalized coordinates 0-1)
    sf::FloatRect viewport = currentView.getViewport();
    sf::Vector2u winSize = window.getSize();
    
    // Calculate viewport in pixels
    float viewportLeft = viewport.left * winSize.x;
    float viewportTop = viewport.top * winSize.y;
    float viewportWidth = viewport.width * winSize.x;
    float viewportHeight = viewport.height * winSize.y;
    
    // Check if point is within viewport
    if (mousePos.x >= viewportLeft && 
        mousePos.x <= viewportLeft + viewportWidth &&
        mousePos.y >= viewportTop && 
        mousePos.y <= viewportTop + viewportHeight) {
        
        // Convert to normalized position within viewport (0-1)
        float normalizedX = (mousePos.x - viewportLeft) / viewportWidth;
        float normalizedY = (mousePos.y - viewportTop) / viewportHeight;
        
        // Convert to UI coordinates based on the fixed BASE_WIDTH and BASE_HEIGHT
        return sf::Vector2f(
            normalizedX * BASE_WIDTH,
            normalizedY * BASE_HEIGHT
        );
    }
    
    // Return (-1,-1) if outside viewport
    return sf::Vector2f(-1, -1);
}