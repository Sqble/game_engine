#pragma once
#include <glm/glm.hpp>
#include "../shader/shader.h"
#include "../sceneobject/drawableobject.h"

class Plane : public DrawableObject {
public:
    Plane(const glm::vec3& position = glm::vec3(0.0f), 
          const glm::vec2& size = glm::vec2(1.0f),
          const glm::vec3& rotation = glm::vec3(0.0f),
          const glm::vec3& color = glm::vec3(1.0f),
          bool isActive = true);
    
    bool isPlane() const override { return true; }
};