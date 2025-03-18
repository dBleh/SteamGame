#ifndef HUD_H
#define HUD_H

#include <SFML/Graphics.hpp>
#include <string>
#include <unordered_map>
#include <vector>
#include <random>
#include "../core/GameState.h"
#include "../utils/SteamHelpers.h"
#include "../utils/config/Config.h"
#include "../entities/Player.h"

/**
 * @brief Class for managing Heads-Up Display (HUD) elements.
 *
 * Provides functionality to add, update, and render HUD elements on screen.
 */
class HUD
{
public:
    /**
     * @brief Rendering modes for HUD elements.
     */
    enum class RenderMode {
        ScreenSpace, ///< Render relative to the screen.
        ViewSpace    ///< Render relative to the game view.
    };

    /**
     * @brief Structure representing a single HUD element.
     */
    /**
 * @brief Structure representing a single HUD element.
 */
struct HUDElement {
    sf::Text text;          ///< Text for the HUD element.
    sf::Vector2f pos;       ///< Position of the element.
    GameState visibleState; ///< Game state in which the element is visible.
    RenderMode mode;        ///< Rendering mode.
    bool hoverable;         ///< Whether the element responds to mouse hover.
    sf::Color baseColor;    ///< Default text color.
    sf::Color hoverColor;   ///< Text color when hovered.
    std::string lineAbove;  ///< ID of the line above this element (for animation)
    std::string lineBelow;  ///< ID of the line below this element (for animation)
    bool isHovered;         ///< Current hover state
    unsigned int originalCharSize; ///< Original character size to maintain constant size
};

    /**
     * @brief Structure representing a gradient line UI element.
     */
    struct GradientLineElement {
        std::vector<sf::RectangleShape> segments; ///< Segments that make up the gradient line
        GameState visibleState;                   ///< Game state in which the line is visible.
        RenderMode mode;                          ///< Rendering mode.
        sf::Vector2f basePosition;                ///< Base position of the line
        float animationIntensity;                 ///< Current shake intensity (0 = no shake)
        float animationTimer;                     ///< Timer for animation
    };

    /**
     * @brief Constructor.
     * @param font Reference to the font used for HUD elements.
     */
    explicit HUD(sf::Font& font);

    /**
     * @brief Checks if the HUD is fully loaded.
     * @return True if at least one HUD element exists.
     */
    bool isFullyLoaded() const;

    /**
     * @brief Adds a HUD element.
     * @param id Unique identifier for the element.
     * @param content Text content.
     * @param size Character size.
     * @param pos Position vector.
     * @param visibleState Game state when visible.
     * @param mode Render mode.
     * @param hoverable Whether it is hoverable.
     * @param lineAboveId ID of the line above this element (for animation)
     * @param lineBelowId ID of the line below this element (for animation)
     */
    void addElement(const std::string& id,
                    const std::string& content,
                    unsigned int size,
                    sf::Vector2f pos,
                    GameState visibleState,
                    RenderMode mode = RenderMode::ScreenSpace,
                    bool hoverable = false,
                    const std::string& lineAboveId = "",
                    const std::string& lineBelowId = "");

    /**
     * @brief Adds a gradient (tapered) horizontal line to the HUD.
     * @param id Unique identifier for the line.
     * @param xPos X position of the line (left edge).
     * @param yPos Y position of the line.
     * @param width Width of the line.
     * @param thickness Thickness of the line.
     * @param color Base color of the line.
     * @param visibleState Game state when visible.
     * @param mode Render mode.
     * @param segments Number of segments to create the gradient effect (higher = smoother).
     */
    void addGradientLine(const std::string& id,
                        float xPos,
                        float yPos,
                        float width,
                        float thickness,
                        const sf::Color& color,
                        GameState visibleState,
                        RenderMode mode = RenderMode::ScreenSpace,
                        int segments = 20);

    /**
     * @brief Updates the text content of a HUD element.
     * @param id Element ID.
     * @param content New text content.
     */
    void updateText(const std::string& id, const std::string& content);

    /**
     * @brief Updates the base color of a HUD element.
     * @param id Element ID.
     * @param color New base color.
     */
    void updateBaseColor(const std::string& id, const sf::Color& color);

    /**
     * @brief Updates the position of a HUD element.
     * @param id Element ID.
     * @param pos New position.
     */
    void updateElementPosition(const std::string& id, const sf::Vector2f& pos);
    
    /**
     * @brief Gets the position of a HUD element.
     * @param id Element ID.
     * @return Position of the element.
     */
    sf::Vector2f getElementPosition(const std::string& id) const;
    
    /**
     * @brief Sets connected line IDs for a menu item for animation.
     * @param id Element ID.
     * @param lineAboveId ID of the line above.
     * @param lineBelowId ID of the line below.
     */
    void setConnectedLines(const std::string& id, 
                          const std::string& lineAboveId, 
                          const std::string& lineBelowId);
    
    /**
     * @brief Update animations and hover states.
     * @param window Reference to the render window.
     * @param currentState Current game state.
     * @param dt Delta time for animations.
     */
    void update(sf::RenderWindow& window, GameState currentState, float dt);
    
    /**
     * @brief Renders HUD elements on the window.
     * @param window Render window.
     * @param view Current game view.
     * @param currentState Current game state.
     */
    void render(sf::RenderWindow& window, const sf::View& view, GameState currentState);

    /**
     * @brief Returns a constant reference to the HUD elements.
     * @return Map of HUD elements.
     */
    const std::unordered_map<std::string, HUDElement>& getElements() const { return m_elements; }

    /**
     * @brief Configures HUD elements for gameplay.
     * @param winSize Window size.
     */
    void configureGameplayHUD(const sf::Vector2u& winSize);

    /**
     * @brief Configures HUD elements for the store.
     * @param winSize Window size.
     */
    void configureStoreHUD(const sf::Vector2u& winSize);
    /**
     * @brief Animates a line with a shaking effect.
     * @param lineId ID of the line to animate.
     * @param intensity Intensity of the shake.
     */
    void animateLine(const std::string& lineId, float intensity);

private:
    sf::Font& m_font; ///< Reference to the font used for HUD elements.
    std::unordered_map<std::string, HUDElement> m_elements; ///< Map of HUD elements.
    std::unordered_map<std::string, GradientLineElement> m_gradientLines; ///< Map of gradient line elements.
    std::mt19937 m_rng; ///< Random number generator for animations

    /**
     * @brief Draws a white background on the window.
     * @param window Render window.
     */
    void drawWhiteBackground(sf::RenderWindow& window);

    /**
     * @brief Checks if the mouse is over the given text.
     * @param window Render window.
     * @param text SFML text object.
     * @return True if the mouse is over the text.
     */
    bool isMouseOverText(const sf::RenderWindow& window, const sf::Text& text);
    
    
    
    /**
     * @brief Converts window mouse coordinates to UI view coordinates.
     * @param window Reference to the render window.
     * @param mousePos Mouse position in window coordinates.
     * @return Position in UI view coordinates or (-1,-1) if outside the viewport.
     */
    sf::Vector2f windowMouseToUICoordinates(const sf::RenderWindow& window, sf::Vector2i mousePos) const;
};

#endif // HUD_H