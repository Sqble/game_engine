#pragma once
#include "sceneobject.h"
#include "../shader/shader.h"
#include <vector>
#include <glm/gtc/matrix_transform.hpp>

class DrawableObject : public SceneObject {
    public:
        DrawableObject(const glm::vec3& position = glm::vec3(0.0f),
                    const glm::vec3& size = glm::vec3(1.0f),
                    const glm::vec3& color = glm::vec3(1.0f),
                    const glm::vec3& rotation = glm::vec3(0.0f),
                    bool isActive = true)
            : SceneObject(position, size, color, rotation, isActive) {
                glGenVertexArrays(1, &VAO);
                glGenBuffers(1, &VBO);
                glGenBuffers(1, &EBO);
                updateModelMatrix();
            }
    
        void draw(Shader& shader) override {
            if (!this->isEffectivelyActive()) return;

            updateModelMatrix();
            
            shader.setMat4("u_model", model_);
            shader.setVec3("meshColor", color_);
            glBindVertexArray(VAO);
            glDrawElements(GL_TRIANGLES, indices_.size(), GL_UNSIGNED_INT, 0);

        }

        ~DrawableObject() {
            glDeleteVertexArrays(1, &VAO);
            glDeleteBuffers(1, &VBO);
            glDeleteBuffers(1, &EBO);
        }

        void setPosition(const glm::vec3& position) override {
            SceneObject::setPosition(position);
            updateModelMatrix();
        }

        void move(const glm::vec3& movementVector) {
            setPosition(getPosition() + movementVector);
        }

        void setSize(const glm::vec3& size) override {
            SceneObject::setSize(size);
            updateModelMatrix();
        }

        void setRotation(const glm::vec3& rotation) override {
            SceneObject::setRotation(rotation);
            updateModelMatrix();
        }

        void rotateByRadians(const glm::vec3& rotationDelta) {
            rotation_ += glm::degrees(rotationDelta);
            updateModelMatrix();
        }

        void rotateByDegrees(const glm::vec3& rotationDelta) {
            rotation_ += rotationDelta;
            updateModelMatrix();
        }

        bool isDrawable() const override { return true; }

    protected:
        unsigned int VAO, VBO, EBO;
        mutable glm::mat4 model_;
        std::vector<float> vertices_;
        std::vector<unsigned int> indices_;

        void updateModelMatrix() const {
            const glm::vec3 position = getPosition();
            const glm::vec3 rotation = getRotation();
            const glm::vec3 size = getSize();

            model_ = glm::translate(glm::mat4(1.0f), position);
            model_ = glm::rotate(model_, glm::radians(rotation.x), glm::vec3(1,0,0));
            model_ = glm::rotate(model_, glm::radians(rotation.y), glm::vec3(0,1,0));
            model_ = glm::rotate(model_, glm::radians(rotation.z), glm::vec3(0,0,1));
            model_ = glm::scale(model_, size);
        }

        void updateVboVertexData() {
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, vertices_.size() * sizeof(float), vertices_.data());
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }
};
