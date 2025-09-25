#pragma once
#include <glm/glm.hpp>

class Submarine {
public:
    Submarine();

    void update(float dt);
    void takeDamage(float amount);

    float getHealth() const;

private:
    float health = 100;
};