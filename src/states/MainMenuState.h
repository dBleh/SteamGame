#ifndef MAIN_MENU_STATE_H
#define MAIN_MENU_STATE_H

#include "State.h"

class MainMenuState : public State {
public:
    explicit MainMenuState(Game* game);
    void Update(float dt) override;
    void Render() override;
    void ProcessEvent(const sf::Event& event) override;

private:
    void ProcessEvents(const sf::Event& event);
    void addSeparatorLine(Game* game, const std::string& id, float yPos, float windowWidth);
};

#endif // MAIN_MENU_STATE_H