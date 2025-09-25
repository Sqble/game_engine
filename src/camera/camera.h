#pragma once

#include <iostream>
#include <string>
#include <vector>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "../sceneobject/sceneobject.h"

#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923
#endif

class Camera : public SceneObject {
    public:
        Camera(int screenWidth, int screenHeight, glm::vec3 position={0.f,0.f,0.f}, glm::vec3 looking={0.f,0.f,-1.f})
        : SceneObject(position, glm::vec3(1), glm::vec3(1), looking),
        screenWidth_(screenWidth), screenHeight_(screenHeight) {
            recalculateMatrix();

            projection_ = glm::perspective(
                (float)M_PI_2,                            // Field of view (90 degrees in radians)
                (float)screenWidth / (float)screenHeight, // Aspect ratio (width / height)
                0.01f,                                    // Near clipping plane
                100.0f                                    // Far clipping plane
            );
        }

        void setPosition(glm::vec3 position) {
            position_ = position;
            recalculateMatrix();
        }

        void move(glm::vec3 movementVector) { 
            setPosition(position_ + movementVector);
        }

        void setRotation(glm::vec3 looking) {
            rotation_ = looking;
            recalculateMatrix();
        }

        void lookAt(const glm::vec3& target) { // untested
            rotation_ = glm::normalize(target - position_);
            recalculateMatrix();
        }

        glm::mat4 getViewMatrix() const {
            return view_;
        }

        glm::mat4 getProjectionMatrix() const {
            return projection_;
        }

        void rotateViewDirection(const glm::vec2& rotationDelta) {
            // Yaw (around Y axis)
            rotation_.y -= rotationDelta.y;
            // Pitch (around X axis)
            rotation_.x += rotationDelta.x;
            recalculateMatrix();
        }

        void move(const glm::vec2& movement) {
            glm::vec3 forwardDir = getForwardDirection();
            glm::vec3 rightDir = glm::normalize(glm::cross(forwardDir, glm::vec3(0.f, 1.f, 0.f)));
            glm::vec3 newPos = position_ + forwardDir * movement.x + rightDir * movement.y;
            setPosition(newPos);
        }

        // Moves camera in the horizontal plane relative to its view direction: x=forward/backward, y=left/right
        void move2D(const glm::vec2& movement) {
            glm::vec3 forwardDir = getForwardDirection();
            forwardDir.y = 0; // Project onto horizontal plane
            forwardDir = glm::normalize(forwardDir);
            glm::vec3 rightDir = glm::normalize(glm::cross(forwardDir, glm::vec3(0.f, 1.f, 0.f)));
            glm::vec3 newPos = position_ + forwardDir * movement.x + rightDir * movement.y;
            setPosition(newPos);
        }

        glm::vec3 getPosition() const { return position_; }

        void moveVertical(float amount) {
            position_.y += amount;
            recalculateMatrix();
        }

        glm::vec3 getForwardDirection() const {
            // Calculate direction from Euler angles (rotation_)
            float pitch = rotation_.x;
            float yaw = rotation_.y;
            glm::vec3 direction;
            direction.x = cos(pitch) * sin(yaw);
            direction.y = sin(pitch);
            direction.z = -cos(pitch) * cos(yaw);
            return glm::normalize(direction);
        }

        bool isCamera() const override { return true; }
        void draw(class Shader& shader) override {
            // Camera does not draw anything
            (void)shader; // suppress unused parameter warning
        }

    private:
        void recalculateMatrix() {
            glm::vec3 forward = getForwardDirection();
            view_ = glm::lookAt(
                position_,                       // Camera position
                position_ + forward, // Target position (camera looks here)
                glm::vec3(0.f, 1.f, 0.f)              // Up vector (positive Y-axis)
            );
        }

        glm::mat4 view_; // View matrix
        glm::mat4 projection_; // Projection matrix

        int screenWidth_;
        int screenHeight_;

};