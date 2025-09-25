#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include "../camera/camera.h"

class Raycast {
public:
    static glm::vec3 getRayFromScreen(float mouseX, float mouseY, int screenWidth, int screenHeight, Camera* camera);
};