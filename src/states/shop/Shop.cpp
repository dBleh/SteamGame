#include "Shop.h"
#include "../utils/config/Config.h"
#include <steam/steam_api.h>
#include <iostream>

// ShopItem implementation
ShopItem::ShopItem(ShopItemType type, const std::string& name, const std::string& description, 
                   int baseCost, int level, int maxLevel)
    : type(type), name(name), description(description), baseCost(baseCost), 
      level(level), maxLevel(maxLevel), costIncrement(SHOP_COST_INCREMENT) {
    
    background.setSize(sf::Vector2f(380.f, 80.f));
    background.setFillColor(sf::Color(60, 60, 60, 230));
    background.setOutlineColor(sf::Color(100, 100, 100));
    background.setOutlineThickness(1.f);
    
    nameText.setCharacterSize(18);
    nameText.setFillColor(sf::Color::White);
    nameText.setStyle(sf::Text::Bold);
    
    descriptionText.setCharacterSize(14);
    descriptionText.setFillColor(sf::Color(200, 200, 200));
    
    costText.setCharacterSize(16);
    costText.setFillColor(sf::Color(255, 215, 0)); // Gold color
    
    levelText.setCharacterSize(14);
    levelText.setFillColor(sf::Color(150, 150, 255));
}

void ShopItem::SetFont(const sf::Font& font) {
    nameText.setFont(font);
    descriptionText.setFont(font);
    costText.setFont(font);
    levelText.setFont(font);
}

void ShopItem::Render(sf::RenderWindow& window, sf::Vector2f position, bool isHighlighted) {
    // Update position
    background.setPosition(position);
    
    // Update text for current level and cost
    nameText.setString(name);
    descriptionText.setString(description);
    
    if (IsMaxLevel()) {
        costText.setString("MAX LEVEL");
        costText.setFillColor(sf::Color(100, 255, 100)); // Green for max level
    } else {
        costText.setString("Cost: " + std::to_string(GetCost()));
        costText.setFillColor(sf::Color(255, 215, 0)); // Gold color
    }
    
    levelText.setString("Level: " + std::to_string(level) + "/" + std::to_string(maxLevel));
    
    // Position text elements
    nameText.setPosition(position.x + 10.f, position.y + 10.f);
    descriptionText.setPosition(position.x + 10.f, position.y + 35.f);
    costText.setPosition(position.x + 10.f, position.y + 55.f);
    levelText.setPosition(position.x + 270.f, position.y + 55.f);
    
    // Highlight if selected
    if (isHighlighted) {
        background.setFillColor(sf::Color(80, 80, 100, 230));
        background.setOutlineColor(sf::Color(150, 150, 255));
        background.setOutlineThickness(2.f);
    } else {
        background.setFillColor(sf::Color(60, 60, 60, 230));
        background.setOutlineColor(sf::Color(100, 100, 100));
        background.setOutlineThickness(1.f);
    }
    
    // Store item bounds for click detection
    bounds = background.getGlobalBounds();
    
    // Draw the item components
    window.draw(background);
    window.draw(nameText);
    window.draw(descriptionText);
    window.draw(costText);
    window.draw(levelText);
}

// Shop implementation
Shop::Shop(Game* game, PlayerManager* playerManager)
    : game(game), playerManager(playerManager), isOpen(false), selectedIndex(0) {
    
    // Set up shop background
    shopBackground.setSize(sf::Vector2f(400.f, 500.f));
    shopBackground.setFillColor(sf::Color(40, 40, 40, 230));
    shopBackground.setOutlineColor(sf::Color(100, 100, 255, 150));
    shopBackground.setOutlineThickness(2.f);
    
    // Set up shop title
    shopTitle.setFont(game->GetFont());
    shopTitle.setString("UPGRADE SHOP");
    shopTitle.setCharacterSize(24);
    shopTitle.setFillColor(sf::Color(220, 220, 220));
    shopTitle.setStyle(sf::Text::Bold);
    
    // Set up player money display
    playerMoneyText.setFont(game->GetFont());
    playerMoneyText.setCharacterSize(18);
    playerMoneyText.setFillColor(sf::Color(255, 215, 0)); // Gold color
    
    // Set up instructions text
    instructionsText.setFont(game->GetFont());
    instructionsText.setString("Click to purchase | B to close");
    instructionsText.setCharacterSize(14);
    instructionsText.setFillColor(sf::Color(180, 180, 180));
    
    // Initialize shop items using constants from Config.h
    items.push_back(ShopItem(ShopItemType::BulletSpeed, "Bullet Speed", 
                             "Increases bullet velocity", 
                             SHOP_BULLET_SPEED_BASE_COST, 0, SHOP_DEFAULT_MAX_LEVEL));
    
    items.push_back(ShopItem(ShopItemType::MoveSpeed, "Movement Speed", 
                             "Increases player movement speed", 
                             SHOP_MOVE_SPEED_BASE_COST, 0, SHOP_DEFAULT_MAX_LEVEL));
    
    items.push_back(ShopItem(ShopItemType::Health, "Health Boost", 
                             "Increases maximum health", 
                             SHOP_HEALTH_BASE_COST, 0, SHOP_DEFAULT_MAX_LEVEL));
    
    // Set font for all items
    for (auto& item : items) {
        item.SetFont(game->GetFont());
    }
    
    UpdateLayout();
}

void Shop::Toggle() {
    isOpen = !isOpen;
    if (isOpen) {
        UpdateLayout();
        UpdateMoneyDisplay();
    }
}

void Shop::UpdateLayout() {
    // Position the shop in the center of the screen
    float centerX = BASE_WIDTH / 2.0f;
    float centerY = BASE_HEIGHT / 2.0f;
    
    shopBackground.setPosition(
        centerX - shopBackground.getSize().x / 2.0f,
        centerY - shopBackground.getSize().y / 2.0f
    );
    
    // Position the title at the top of the shop
    sf::FloatRect titleBounds = shopTitle.getLocalBounds();
    shopTitle.setOrigin(
        titleBounds.left + titleBounds.width / 2.0f, 
        titleBounds.top + titleBounds.height / 2.0f
    );
    shopTitle.setPosition(
        centerX,
        centerY - shopBackground.getSize().y / 2.0f + 30.0f
    );
    
    // Position player money text
    playerMoneyText.setPosition(
        centerX - shopBackground.getSize().x / 2.0f + 20.0f,
        centerY - shopBackground.getSize().y / 2.0f + 60.0f
    );
    
    // Position instructions text at the bottom
    sf::FloatRect instructionsBounds = instructionsText.getLocalBounds();
    instructionsText.setOrigin(
        instructionsBounds.left + instructionsBounds.width / 2.0f,
        instructionsBounds.top + instructionsBounds.height / 2.0f
    );
    instructionsText.setPosition(
        centerX,
        centerY + shopBackground.getSize().y / 2.0f - 20.0f
    );
}

void Shop::UpdateMoneyDisplay() {
    if (!playerManager) return;
    
    // Get local player's money
    const auto& localPlayer = playerManager->GetLocalPlayer();
    playerMoneyText.setString("Money: " + std::to_string(localPlayer.money));
}

void Shop::Update(float dt) {
    if (!isOpen) return;
    
    UpdateMoneyDisplay();
}

void Shop::Render(sf::RenderWindow& window) {
    if (!isOpen) return;
    
    // Store original view
    sf::View originalView = window.getView();
    
    // Set UI view for shop rendering
    window.setView(game->GetUIView());
    
    // Draw semi-transparent overlay
    sf::RectangleShape overlay;
    overlay.setSize(sf::Vector2f(BASE_WIDTH, BASE_HEIGHT));
    overlay.setFillColor(sf::Color(0, 0, 0, 150));
    window.draw(overlay);
    
    // Draw shop background
    window.draw(shopBackground);
    window.draw(shopTitle);
    window.draw(playerMoneyText);
    window.draw(instructionsText);
    
    // Draw shop items
    float itemYOffset = 120.0f;
    float itemSpacing = 90.0f;
    
    float centerX = BASE_WIDTH / 2.0f;
    float startY = (BASE_HEIGHT - shopBackground.getSize().y) / 2.0f + itemYOffset;
    
    for (size_t i = 0; i < items.size(); i++) {
        float itemY = startY + (i * itemSpacing);
        sf::Vector2f itemPos(centerX - 190.0f, itemY);
        items[i].Render(window, itemPos, i == selectedIndex);
    }
    
    // Restore original view
    window.setView(originalView);
}

void Shop::ProcessEvent(const sf::Event& event) {
    if (!isOpen) return;
    
    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::B || event.key.code == sf::Keyboard::Escape) {
            Close();
        } else if (event.key.code == sf::Keyboard::Up) {
            selectedIndex = (selectedIndex > 0) ? selectedIndex - 1 : items.size() - 1;
        } else if (event.key.code == sf::Keyboard::Down) {
            selectedIndex = (selectedIndex < items.size() - 1) ? selectedIndex + 1 : 0;
        } else if (event.key.code == sf::Keyboard::Return || event.key.code == sf::Keyboard::Space) {
            PurchaseSelectedItem();
        }
    } else if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        // Get mouse position in UI coordinates
        sf::Vector2i mousePos(event.mouseButton.x, event.mouseButton.y);
        sf::Vector2f mouseUIPos = game->WindowToUICoordinates(mousePos);
        
        // Check if any item was clicked
        for (size_t i = 0; i < items.size(); i++) {
            if (items[i].GetBounds().contains(mouseUIPos)) {
                selectedIndex = i;
                PurchaseSelectedItem();
                break;
            }
        }
    } else if (event.type == sf::Event::MouseMoved) {
        // Get mouse position in UI coordinates
        sf::Vector2i mousePos(event.mouseMove.x, event.mouseMove.y);
        sf::Vector2f mouseUIPos = game->WindowToUICoordinates(mousePos);
        
        // Highlight item under mouse
        for (size_t i = 0; i < items.size(); i++) {
            if (items[i].GetBounds().contains(mouseUIPos)) {
                selectedIndex = i;
                break;
            }
        }
    }
}

void Shop::PurchaseSelectedItem() {
    if (selectedIndex >= 0 && selectedIndex < items.size()) {
        ShopItem& item = items[selectedIndex];
        
        // Check if max level or can't afford
        if (item.IsMaxLevel() || !CanAffordItem(item)) {
            return;
        }
        
        // Get player
        auto& localPlayer = playerManager->GetLocalPlayer();
        
        // Subtract cost
        localPlayer.money -= item.GetCost();
        
        // Increase item level
        item.IncreaseLevel();
        
        // Apply upgrade immediately
        ApplyUpgrades(localPlayer.player);
        
        // Update display
        UpdateMoneyDisplay();
    }
}

bool Shop::CanAffordItem(const ShopItem& item) const {
    if (!playerManager) return false;
    
    const auto& localPlayer = playerManager->GetLocalPlayer();
    return localPlayer.money >= item.GetCost();
}

void Shop::ApplyUpgrades(Player& player) {
    // Apply all upgrades based on current levels
    for (const auto& item : items) {
        float level = static_cast<float>(item.GetLevel());
        
        switch (item.GetType()) {
            case ShopItemType::BulletSpeed:
                // Use constant for bullet speed multiplier
                player.SetBulletSpeedMultiplier(1.0f + (level * SHOP_BULLET_SPEED_MULTIPLIER));
                break;
                
            case ShopItemType::MoveSpeed:
                // Use constant for move speed multiplier
                player.SetSpeed(PLAYER_SPEED * (1.0f + (level * SHOP_MOVE_SPEED_MULTIPLIER)));
                break;
                
            case ShopItemType::Health:
                // Use constant for health increase per level
                player.SetMaxHealth(PLAYER_HEALTH + (level * SHOP_HEALTH_INCREASE));
                // Also heal the player when they buy health upgrades
                player.SetHealth(player.GetMaxHealth());
                break;
        }
    }
}