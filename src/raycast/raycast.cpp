#include "raycast.h"
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>

glm::vec3 Raycast::getRayFromScreen(float mouseX, float mouseY, int screenWidth, int screenHeight, Camera* camera) {
    // Convert to normalized device coordinates
    float x = (2.0f * mouseX) / screenWidth - 1.0f;
    float y = 1.0f - (2.0f * mouseY) / screenHeight;
    glm::vec4 ray_clip = glm::vec4(x, y, -1.0f, 1.0f);

    // Convert to eye space
    glm::mat4 proj = camera->getProjectionMatrix();
    glm::vec4 ray_eye = glm::inverse(proj) * ray_clip;
    ray_eye = glm::vec4(ray_eye.x, ray_eye.y, -1.0f, 0.0f);

    // Convert to world space
    glm::mat4 view = camera->getViewMatrix();
    glm::vec3 ray_world = glm::vec3(glm::inverse(view) * ray_eye);
    ray_world = glm::normalize(ray_world);

    return ray_world;
}