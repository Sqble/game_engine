#pragma once
#include <glm/glm.hpp>

#include "../../scene/scene.h"
#include "../../mesh/mesh.h"
#include "../../pointlight/pointlight.h"

class Submarine {
public:
    Submarine(Scene* scene);

    void update(float dt);
    void takeDamage(float amount);

    float getHealth() const;

    void setScene(Scene* scene) { scene_ = scene; }

private:
    float health = 100;

    Scene* scene_;
    
    PointLight* light_ = nullptr;
};