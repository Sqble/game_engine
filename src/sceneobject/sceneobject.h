#pragma once

#include <glm/glm.hpp>

#include <algorithm>
#include <string>
#include <vector>

class SceneObject {
public:
    SceneObject(const glm::vec3& position = glm::vec3(0.0f),
                const glm::vec3& size = glm::vec3(1.0f),
                const glm::vec3& color = glm::vec3(1.0f),
                const glm::vec3& rotation = glm::vec3(0.0f),
                bool isActive = true);

    virtual ~SceneObject() = default;

    virtual void draw(class Shader& shader) = 0;

    virtual void setPosition(const glm::vec3& position);
    virtual void setSize(const glm::vec3& size);
    virtual void setColor(const glm::vec3& color) { color_ = color; }
    virtual void setRotation(const glm::vec3& rotation);
    virtual void setActive(bool active) { isActive_ = active; }
    void setTag(const std::string& tag) { tag_ = tag; }
    void setId(int id) { id_ = id; }

    bool setParent(SceneObject* parent, bool keepWorldTransform = true);
    bool addChild(SceneObject* child, bool keepWorldTransform = true);
    void removeChild(SceneObject* child, bool keepWorldTransform = true);
    void detach(bool keepWorldTransform = true);
    void detachChildren(bool keepWorldTransform = true);

    virtual bool isDrawable() const { return false; }
    virtual bool isMesh() const { return false; }
    virtual bool isPlane() const { return false; }
    virtual bool isCube() const { return false; }
    virtual bool isCamera() const { return false; }
    virtual bool isLight() const { return false; }
    virtual bool isPointLight() const { return false; }
    virtual bool isSpotLight() const { return false; }

    bool isParent() const { return !children_.empty(); }
    bool isChild() const { return parent_ != nullptr; }

    virtual bool intersectRay(const glm::vec3& rayOrigin, const glm::vec3& rayDir, float& hitDist) const {
        (void)rayOrigin;
        (void)rayDir;
        (void)hitDist;
        return false;
    }

    glm::vec3 getPosition() const;
    glm::vec3 getSize() const;
    glm::vec3 getColor() const { return color_; }
    glm::vec3 getRotation() const;
    bool isActive() const { return isActive_; }
    bool isEffectivelyActive() const;
    std::string getTag() const { return tag_; }
    int getId() const { return id_; }
    int getParentId() const;

    glm::vec3 getLocalPosition() const { return position_; }
    glm::vec3 getLocalSize() const { return size_; }
    glm::vec3 getLocalRotation() const { return rotation_; }

    SceneObject* getParent() const { return parent_; }
    const std::vector<SceneObject*>& getChildren() const { return children_; }

    static int nextId() { return sceneObjectIdCounter + 1; }
    static int assignNextId();
    static void setIDCounter(int id) { sceneObjectIdCounter = id; }

protected:
    static int sceneObjectIdCounter;

    glm::vec3 position_;
    glm::vec3 size_;
    glm::vec3 color_;
    glm::vec3 rotation_;
    bool isActive_;
    std::string tag_;
    int id_;

private:
    bool wouldCreateCycle(const SceneObject* newParent) const;
    static glm::vec3 divideScale(const glm::vec3& value, const glm::vec3& divisor);

    SceneObject* parent_ = nullptr;
    std::vector<SceneObject*> children_;
};
