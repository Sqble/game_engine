
#include "submarine.h"

Submarine::Submarine(Scene* scene) : health(100.0f) {
    scene_ = scene;
    light_ = static_cast<PointLight*>(scene->getFirstObjectWithTag("light1"));
}

void Submarine::update(float dt) {
    
}

void Submarine::takeDamage(float amount) {
    health -= amount;
    if (health < 0) health = 0;
}

float Submarine::getHealth() const {
    return health;
}
