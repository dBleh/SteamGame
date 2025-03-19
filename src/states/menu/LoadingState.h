// LoadingState.h
#ifndef LOADING_STATE_H
#define LOADING_STATE_H

#include "../base/State.h"
#include <SFML/Graphics.hpp>

class Game;

class LoadingState : public State {
public:
    explicit LoadingState(Game* game);
    void Update(float dt) override;
    void Render() override;
    void ProcessEvent(const sf::Event& event) override;
};

#endif // LOADING_STATE_H