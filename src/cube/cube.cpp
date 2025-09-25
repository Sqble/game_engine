#include "cube.h"
#include <gl/glew.h>
#include <glm/gtc/matrix_transform.hpp>

//this class is deprecated, use Mesh with a cube obj instead
Cube::Cube(const glm::vec3& position,
         const glm::vec3& size,
         const glm::vec3& color,
         const glm::vec3& rotation,
         bool isActive) 
         : DrawableObject(position, glm::vec3(1), color, rotation, isActive) {

    vertices_ = {
        // x, y, z
        -0.5f, -0.5f, -0.5f, // 0
         0.5f, -0.5f, -0.5f, // 1
         0.5f,  0.5f, -0.5f, // 2
        -0.5f,  0.5f, -0.5f, // 3
        -0.5f, -0.5f,  0.5f, // 4
         0.5f, -0.5f,  0.5f, // 5
         0.5f,  0.5f,  0.5f, // 6
        -0.5f,  0.5f,  0.5f  // 7
    };

    indices_ = {
        0,1,2, 2,3,0, // back
        4,5,6, 6,7,4, // front
        0,4,7, 7,3,0, // left
        1,5,6, 6,2,1, // right
        3,2,6, 6,7,3, // top
        0,1,5, 5,4,0  // bottom
    };

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices_.size() * sizeof(float), vertices_.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_.size() * sizeof(unsigned int), indices_.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0); // position only
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    setSize(size);
    setPosition(position);
    setRotation(rotation);
}