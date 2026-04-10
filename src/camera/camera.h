#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <cmath>

#include <epoxy/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "../sceneobject/sceneobject.h"

#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923
#endif

class Camera : public SceneObject {
    public:
        // Camera rotation is stored as Euler pitch/yaw/roll in radians.
        Camera(int screenWidth, int screenHeight, glm::vec3 position={0.f,0.f,0.f}, glm::vec3 rotationRadians={0.f,0.f,0.f})
        : SceneObject(position, glm::vec3(1), glm::vec3(1), rotationRadians),
        screenWidth_(screenWidth), screenHeight_(screenHeight) {
            recalculateMatrix();

            projection_ = glm::perspective(
                (float)M_PI_2,                            // Field of view (90 degrees in radians)
                (float)screenWidth / (float)screenHeight, // Aspect ratio (width / height)
                0.01f,                                    // Near clipping plane
                100.0f                                    // Far clipping plane
            );
        }

        void setPosition(const glm::vec3& position) override {
            SceneObject::setPosition(position);
            recalculateMatrix();
        }

        void move(glm::vec3 movementVector) { 
            setPosition(getPosition() + movementVector);
        }

        void setRotation(const glm::vec3& rotationRadians) override {
            SceneObject::setRotation(rotationRadians);
            recalculateMatrix();
        }

        void setLookDirection(const glm::vec3& direction) {
            const float length = glm::length(direction);
            if (length < 0.0001f) {
                return;
            }

            const glm::vec3 normalized = glm::normalize(direction);
            const float pitch = std::asin(glm::clamp(normalized.y, -1.0f, 1.0f));
            const float yaw = std::atan2(normalized.x, -normalized.z);
            setRotation(glm::vec3(pitch, yaw, 0.0f));
        }

        void lookAt(const glm::vec3& target) {
            setLookDirection(target - getPosition());
        }

        glm::mat4 getViewMatrix() const {
            return view_;
        }

        glm::mat4 getProjectionMatrix() const {
            return projection_;
        }

        void rotateViewDirection(const glm::vec2& rotationDelta) {
            glm::vec3 looking = getRotation();
            // Yaw (around Y axis)
            looking.y -= rotationDelta.y;
            // Pitch (around X axis)
            looking.x += rotationDelta.x;
            // Clamp pitch to avoid flipping
            const float pitchLimit = (float)M_PI_2 - 0.01f; // ~89.4 degrees
            if (looking.x > pitchLimit) looking.x = pitchLimit;
            if (looking.x < -pitchLimit) looking.x = -pitchLimit;
            setRotation(looking);
        }

        void move(const glm::vec2& movement) {
            glm::vec3 forwardDir = getForwardDirection();
            glm::vec3 rightDir = glm::cross(forwardDir, glm::vec3(0.f, 1.f, 0.f));
            if (glm::length(rightDir) < 0.0001f) {
                const float yaw = getRotation().y;
                rightDir = glm::vec3(std::cos(yaw), 0.0f, std::sin(yaw));
            } else {
                rightDir = glm::normalize(rightDir);
            }
            glm::vec3 newPos = getPosition() + forwardDir * movement.x + rightDir * movement.y;
            setPosition(newPos);
        }

        // Moves camera in the horizontal plane relative to its view direction: x=forward/backward, y=left/right
        void move2D(const glm::vec2& movement) {
            const float yaw = getRotation().y;
            glm::vec3 forwardDir(std::sin(yaw), 0.0f, -std::cos(yaw));
            glm::vec3 rightDir = glm::normalize(glm::cross(forwardDir, glm::vec3(0.f, 1.f, 0.f)));
            glm::vec3 newPos = getPosition() + forwardDir * movement.x + rightDir * movement.y;
            setPosition(newPos);
        }

        void moveVertical(float amount) {
            glm::vec3 position = getPosition();
            position.y += amount;
            setPosition(position);
        }

        glm::vec3 getForwardDirection() const {
            const glm::vec3 rotation = getRotation();
            float pitch = rotation.x;
            float yaw = rotation.y;
            glm::vec3 direction;
            direction.x = cos(pitch) * sin(yaw);
            direction.y = sin(pitch);
            direction.z = -cos(pitch) * cos(yaw);
            return glm::normalize(direction);
        }

        bool isCamera() const override { return true; }
        void updateProjection(int width, int height) {
            screenWidth_ = width;
            screenHeight_ = height;
            projection_ = glm::perspective(
                (float)M_PI_2,
                (float)screenWidth_ / (float)screenHeight_,
                0.01f,
                100.0f
            );
        }
        void draw(class Shader& shader) override {
            // Camera does not draw anything
            (void)shader; // suppress unused parameter warning
        }

    private:
        void recalculateMatrix() {
            glm::vec3 forward = getForwardDirection();
            view_ = glm::lookAt(
                getPosition(),
                getPosition() + forward,
                glm::vec3(0.f, 1.f, 0.f)              // Up vector (positive Y-axis)
            );
        }

        glm::mat4 view_; // View matrix
        glm::mat4 projection_; // Projection matrix

        int screenWidth_;
        int screenHeight_;

};
