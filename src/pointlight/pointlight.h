#pragma once
#include <glm/glm.hpp>
#include "../shader/shader.h"
#include "../sceneobject/lightobject.h"

class PointLight : public LightObject {
public:
    PointLight(const glm::vec3& pos, const glm::vec3& col, float light_level = 1.0f, bool isActive = true)
        : LightObject(pos, col, light_level, isActive) {}

    void draw(Shader& shader, int index) {
        std::string idx = std::to_string(index);
        shader.setVec3("lights[" + idx + "].position", position_);
        shader.setVec3("lights[" + idx + "].color", color_ * intensity_);
    }
    void draw(Shader& shader) override {
        draw(shader, 0); // Default index
    }

    bool isPointLight() const override { return true; }
};