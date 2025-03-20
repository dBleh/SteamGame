#ifndef SHOP_H
#define SHOP_H

#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <memory>
#include "../core/Game.h"
#include "../entities/player/PlayerManager.h"

// Expanded enum to include ForceField upgrades
enum class ShopItemType {
    BulletSpeed,
    MoveSpeed,
    Health,
    ForceFieldRadius,
    ForceFieldDamage,
    ForceFieldCooldown,
    ForceFieldChain,
    ForceFieldType
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
    void SendForceFieldUpdateToNetwork(const Player& player);
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
    float scrollOffset = 0.0f;
    
    // Layout constants
    static constexpr float ITEM_Y_OFFSET = 110.0f;
    static constexpr float ITEM_SPACING = 90.0f;
    
    void UpdateLayout();
    void PurchaseSelectedItem();
    bool CanAffordItem(const ShopItem& item) const;
    void UpdateMoneyDisplay();
    
    // Constants for ForceField upgrades
    static constexpr float FORCE_FIELD_RADIUS_BASE = 150.0f;
    static constexpr float FORCE_FIELD_RADIUS_INCREMENT = 50.0f;
    static constexpr float FORCE_FIELD_DAMAGE_BASE = 25.0f;
    static constexpr float FORCE_FIELD_DAMAGE_INCREMENT = 5.0f;
    static constexpr float FORCE_FIELD_COOLDOWN_BASE = 0.3f;
    static constexpr float FORCE_FIELD_COOLDOWN_DECREMENT = 0.03f;
    static constexpr int FORCE_FIELD_CHAIN_BASE = 3;
    static constexpr int FORCE_FIELD_CHAIN_INCREMENT = 1;
};

#endif // SHOP_H