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

class SpotLight : public PointLight {
public:
    SpotLight(const glm::vec3& pos, const glm::vec3& dir, const glm::vec3& col, float light_level = 1.0f, float cutoff = 12.5f, bool isActive = true)
        : PointLight(pos, col, light_level, isActive), direction_(dir), cutoff_(cutoff) {}

    void draw(Shader& shader, int index) {
        std::string idx = std::to_string(index);
        shader.setVec3("spotLights[" + idx + "].position", position_);
        shader.setVec3("spotLights[" + idx + "].direction", direction_);
        shader.setVec3("spotLights[" + idx + "].color", color_ * intensity_);
        shader.setFloat("spotLights[" + idx + "].cutoff", glm::cos(glm::radians(cutoff_)));
    }
    void draw(Shader& shader) override {
        draw(shader, 0); // Default index
    }
    bool isSpotLight() const override { return true; }
    bool isPointLight() const override { return false; }

private:
    glm::vec3 direction_;
    float cutoff_;
};