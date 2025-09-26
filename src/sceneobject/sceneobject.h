#pragma once
#include <glm/glm.hpp>
#include <string>
#include <vector>

static int sceneObjectIdCounter = 0;

class SceneObject {
public:
    SceneObject(const glm::vec3& position = glm::vec3(0.0f),
                const glm::vec3& size = glm::vec3(1.0f),
                const glm::vec3& color = glm::vec3(1.0f),
                const glm::vec3& rotation = glm::vec3(0.0f),
                bool isActive = true)
        : position_(position), size_(size), color_(color),
          rotation_(rotation), isActive_(isActive), id_(assignNextId()) {}

    virtual ~SceneObject() = default;

    virtual void draw(class Shader& shader) = 0;

    // Setters
    virtual void setPosition(const glm::vec3& position) { position_ = position; }
    virtual void setSize(const glm::vec3& size) { size_ = size; }
    virtual void setColor(const glm::vec3& color) { color_ = color; }
    virtual void setRotation(const glm::vec3& rotation) { rotation_ = rotation; }
    virtual void setActive(bool active) { isActive_ = active; }
    void setTag(const std::string& tag) { tag_ = tag; }
    void setId(int id) { id_ = id; }
    
    // Type identification
    virtual bool isDrawable() const { return false; }
    virtual bool isMesh() const { return false; }
    virtual bool isPlane() const { return false; }
    virtual bool isCube() const { return false; }
    virtual bool isCamera() const { return false; }
    virtual bool isLight() const { return false; }
    virtual bool isPointLight() const { return false; }
    virtual bool isParent() const { return false; }
    virtual bool isChild() const { return isChild_; }

    // Ray intersection (for engine UI mesh selection, raycasting)
    virtual bool intersectRay(const glm::vec3& rayOrigin, const glm::vec3& rayDir, float& hitDist) const {
        return false;
    }

    // Getters
    glm::vec3 getPosition() const { return position_; }
    glm::vec3 getSize() const { return size_; }
    glm::vec3 getColor() const { return color_; }
    glm::vec3 getRotation() const { return rotation_; }
    bool isActive() const { return isActive_; }
    std::string getTag() const { return tag_; }
    int getId() const { return id_; }

    // Virtual children getter for tree UI
    virtual const std::vector<SceneObject*>& getChildren() const {
        static const std::vector<SceneObject*> empty;
        return empty;
    }

    static int nextId() { return sceneObjectIdCounter + 1; }
    static int assignNextId() { return sceneObjectIdCounter++; }
    static void setIDCounter(int id) { sceneObjectIdCounter = id; }
    bool isChild_ = false;
protected:
    glm::vec3 position_;
    glm::vec3 size_;
    glm::vec3 color_;
    glm::vec3 rotation_;
    bool isActive_;
    std::string tag_ = "";
    int id_ = 0;
    
};


template <typename T>
class ParentObject : public T {
public:
    using T::T; // Inherit constructors

    ParentObject(const std::vector<SceneObject*>& children) : children_(children) {
        for (SceneObject* child : children_) {
            child->isChild_ = true;
        }
    }

    void setPosition(const glm::vec3& position) override {
        glm::vec3 delta = position - this->position_;
        T::setPosition(position);
        for (SceneObject* child : children_) {
            child->setPosition(child->getPosition() + delta);
        }
    }

    void setSize(const glm::vec3& size) override {
        glm::vec3 delta = size - this->size_;
        T::setSize(size);
        for (SceneObject* child : children_) {
            child->setSize(child->getSize() + delta);
        }
    }

    void setRotation(const glm::vec3& rotation) override {
        glm::vec3 delta = rotation - this->rotation_;
        T::setRotation(rotation);
        for (SceneObject* child : children_) {
            child->setRotation(child->getRotation() + delta);
        }
    }

    void setActive(bool active) override {
        if (!active) {
            // Cache current active state of children
            childActiveCache_.clear();
            for (SceneObject* child : children_) {
                childActiveCache_.push_back(child->isActive());
                child->setActive(false);
            }
        } else {
            // Restore only those that were previously active
            for (size_t i = 0; i < children_.size(); ++i) {
                if (i < childActiveCache_.size() && childActiveCache_[i]) {
                    children_[i]->setActive(true);
                }
            }
        }
        T::setActive(active);
    }

    void addChild(SceneObject* child) {
        child->isChild_ = true;
        children_.push_back(child);
    }


    const std::vector<SceneObject*>& getChildren() const override {
        return children_;
    }

    bool isParent() const override { return true; }

private:
    std::vector<SceneObject*> children_;
    std::vector<bool> childActiveCache_;
};