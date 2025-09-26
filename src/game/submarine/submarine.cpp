
#include "submarine.h"
#include <iostream>

Submarine::Submarine(Scene* scene) : health(100.0f) {
    scene_ = scene;
    light_ = static_cast<PointLight*>(scene->getFirstObjectWithTag("light1"));
    if (!light_) {
        std::cerr << "Error: No light with tag 'light1' found in scene." << std::endl;
    }
    time = 0; //glfwGetTime();
}

void Submarine::update(float dt) {
    //light flickers on and off every second
    time += dt;
    if (time >= 1) {
        time = 0;
        if (light_) {
            light_->setActive(!light_->isActive());
            //light_->setIntensity(light_->getIntensity() > 1.0f ? 0.75f : 1.25f);
        }
    }
}

void Submarine::takeDamage(float amount) {
    health -= amount;
    if (health < 0) health = 0;
}

float Submarine::getHealth() const {
    return health;
}
