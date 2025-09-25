#pragma once
#include <glm/glm.hpp>
#include "../shader/shader.h"
#include "../sceneobject/drawableobject.h"

//this class is deprecated, use Mesh with a cube obj instead
class Cube : public DrawableObject {
public:
    Cube(const glm::vec3& position = glm::vec3(0.0f),
         const glm::vec3& size = glm::vec3(1.0f),
         const glm::vec3& color = glm::vec3(1.0f),
         const glm::vec3& rotation = glm::vec3(0.0f),
         bool isActive = true);

    bool isCube() const override { return true; }
};