#pragma once
#include "sceneobject.h"
#include <glm/glm.hpp>

class LightObject : public SceneObject {
public:
    LightObject(const glm::vec3& position = glm::vec3(0.0f),
                const glm::vec3& color = glm::vec3(1.0f),
                float intensity = 1.0f,
                bool isActive = true)
        : SceneObject(position, glm::vec3(1.0f), color, glm::vec3(1.0f), isActive),
          intensity_(intensity) {}

    virtual ~LightObject() = default;

    virtual void draw(class Shader& shader) = 0;
    virtual void draw(class Shader& shader, int index) = 0;

    void setIntensity(float intensity) { intensity_ = intensity; }
    float getIntensity() const { return intensity_; }

    virtual bool isLight() const override { return true; }

protected:
    float intensity_;
};