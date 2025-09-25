#pragma once
#include <glm/glm.hpp>
#include <string>

class SceneObject {
public:
    SceneObject(const glm::vec3& position = glm::vec3(0.0f),
                const glm::vec3& size = glm::vec3(1.0f),
                const glm::vec3& color = glm::vec3(1.0f), // r, g, b
                const glm::vec3& rotation = glm::vec3(0.0f), // pitch, yaw, roll
                bool isActive = true)
        : position_(position), size_(size), color_(color),
          rotation_(rotation), isActive_(isActive) {}

    virtual ~SceneObject() = default;

    virtual void draw(class Shader& shader) = 0;

    virtual void setPosition(const glm::vec3& position) { position_ = position; }
    glm::vec3 getPosition() const { return position_; }

    virtual void setSize(const glm::vec3& size) { size_ = size; }
    glm::vec3 getSize() const { return size_; }

    virtual void setColor(const glm::vec3& color) { color_ = color; }
    glm::vec3 getColor() const { return color_; }

    virtual void setRotation(const glm::vec3& rotation) { rotation_ = rotation; }
    glm::vec3 getRotation() const { return rotation_; }

    virtual void setActive(bool active) { isActive_ = active; }
    bool isActive() const { return isActive_; }

    virtual bool isDrawable() const { return false; }
    virtual bool isMesh() const { return false; }
    virtual bool isPlane() const { return false; }
    virtual bool isCube() const { return false; }
    virtual bool isCamera() const { return false; }
    virtual bool isLight() const { return false; }
    virtual bool isPointLight() const { return false; }

    virtual bool intersectRay(const glm::vec3& rayOrigin, const glm::vec3& rayDir, float& hitDist) const {
        return false; // Default: no ray intersection support (only mesh can raycast)
    }

    void setTag(const std::string& tag) { tag_ = tag; }
    std::string getTag() const { return tag_; }

protected:
    glm::vec3 position_;
    glm::vec3 size_;
    glm::vec3 color_;
    glm::vec3 rotation_;
    bool isActive_;
    std::string tag_ = "";
};

