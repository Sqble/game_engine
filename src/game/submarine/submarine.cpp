
#include "submarine.h"

Submarine::Submarine() : health(100.0f) {}

void Submarine::update(float dt) {
    // Update submarine logic here
}

void Submarine::takeDamage(float amount) {
    health -= amount;
    if (health < 0) health = 0;
}

float Submarine::getHealth() const {
    return health;
}
