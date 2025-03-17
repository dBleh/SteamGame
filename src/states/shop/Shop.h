#ifndef SHOP_H
#define SHOP_H

#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <memory>
#include "../core/Game.h"
#include "../entities/PlayerManager.h"

// Enum to define the different shop item types
enum class ShopItemType {
    BulletSpeed,
    MoveSpeed,
    Health,
    // Add more types as needed
};

// Class to represent a single item in the shop
class ShopItem {
public:
    ShopItem(ShopItemType type, const std::string& name, const std::string& description, 
             int baseCost, int level = 0, int maxLevel = SHOP_DEFAULT_MAX_LEVEL);
    
    void Render(sf::RenderWindow& window, sf::Vector2f position, bool isHighlighted);
    void SetFont(const sf::Font& font);
    
    ShopItemType GetType() const { return type; }
    std::string GetName() const { return name; }
    std::string GetDescription() const { return description; }
    int GetCost() const { return baseCost + (level * costIncrement); }
    int GetLevel() const { return level; }
    bool IsMaxLevel() const { return level >= maxLevel; }
    
    void IncreaseLevel() { if (level < maxLevel) level++; }
    
    sf::FloatRect GetBounds() const { return bounds; }
    void SetBounds(const sf::FloatRect& newBounds) { bounds = newBounds; }
    
private:
    ShopItemType type;
    std::string name;
    std::string description;
    int baseCost;
    int costIncrement; // Now set from SHOP_COST_INCREMENT constant
    int level;
    int maxLevel;
    
    sf::Text nameText;
    sf::Text descriptionText;
    sf::Text costText;
    sf::Text levelText;
    sf::RectangleShape background;
    sf::FloatRect bounds;
};

// The main Shop class
class Shop {
public:
    Shop(Game* game, PlayerManager* playerManager);
    ~Shop() = default;
    
    void Toggle(); // Toggle shop visibility
    bool IsOpen() const { return isOpen; }
    void Close() { isOpen = false; }
    
    void Update(float dt);
    void Render(sf::RenderWindow& window);
    void ProcessEvent(const sf::Event& event);
    
    // Apply purchased upgrades to the player
    void ApplyUpgrades(Player& player);
    
private:
    Game* game;
    PlayerManager* playerManager;
    bool isOpen;
    
    std::vector<ShopItem> items;
    int selectedIndex;
    
    sf::RectangleShape shopBackground;
    sf::Text shopTitle;
    sf::Text playerMoneyText;
    sf::Text instructionsText;
    
    void UpdateLayout();
    void PurchaseSelectedItem();
    bool CanAffordItem(const ShopItem& item) const;
    void UpdateMoneyDisplay();
};

#endif // SHOP_H