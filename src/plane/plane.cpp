#include "plane.h"
#include <gl/glew.h>
#include <glm/gtc/matrix_transform.hpp>

Plane::Plane(const glm::vec3& position, const glm::vec2& size, const glm::vec3& rotation, const glm::vec3& color, bool isActive)
: DrawableObject(position, glm::vec3(1), color, rotation, isActive) {

    vertices_ = {
        // x, y, z
        -0.5f, 0.0f, -0.5f,
         0.5f, 0.0f, -0.5f,
         0.5f, 0.0f,  0.5f,
        -0.5f, 0.0f,  0.5f
    };

    indices_ = {
        0, 1, 2,
        2, 3, 0
    };

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices_.size() * sizeof(float), vertices_.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_.size() * sizeof(unsigned int), indices_.data(), GL_STATIC_DRAW);

    // send to vertex shader: only position now
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0); // position
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    setSize(glm::vec3(size.x, 0, size.y));
    setPosition(position_);
    setRotation(rotation);
}