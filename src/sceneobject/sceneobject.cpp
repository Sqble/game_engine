#include "sceneobject.h"

#include <cmath>

int SceneObject::sceneObjectIdCounter = 0;

SceneObject::SceneObject(const glm::vec3& position,
                         const glm::vec3& size,
                         const glm::vec3& color,
                         const glm::vec3& rotation,
                         bool isActive)
    : position_(position),
      size_(size),
      color_(color),
      rotation_(rotation),
      isActive_(isActive),
      tag_(""),
      id_(assignNextId()) {}

int SceneObject::assignNextId() {
    return sceneObjectIdCounter++;
}

void SceneObject::setPosition(const glm::vec3& position) {
    if (!parent_) {
        position_ = position;
        return;
    }

    position_ = position - parent_->getPosition();
}

void SceneObject::setSize(const glm::vec3& size) {
    if (!parent_) {
        size_ = size;
        return;
    }

    size_ = divideScale(size, parent_->getSize());
}

void SceneObject::setRotation(const glm::vec3& rotation) {
    if (!parent_) {
        rotation_ = rotation;
        return;
    }

    rotation_ = rotation - parent_->getRotation();
}

bool SceneObject::setParent(SceneObject* parent, bool keepWorldTransform) {
    if (parent == this) {
        return false;
    }

    if (parent && wouldCreateCycle(parent)) {
        return false;
    }

    if (parent_ == parent) {
        return true;
    }

    const glm::vec3 worldPosition = getPosition();
    const glm::vec3 worldRotation = getRotation();
    const glm::vec3 worldSize = getSize();

    if (parent_) {
        auto& siblings = parent_->children_;
        siblings.erase(std::remove(siblings.begin(), siblings.end(), this), siblings.end());
    }

    parent_ = parent;

    if (parent_) {
        auto& children = parent_->children_;
        if (std::find(children.begin(), children.end(), this) == children.end()) {
            children.push_back(this);
        }
    }

    if (keepWorldTransform) {
        setPosition(worldPosition);
        setRotation(worldRotation);
        setSize(worldSize);
    }

    return true;
}

bool SceneObject::addChild(SceneObject* child, bool keepWorldTransform) {
    if (!child) {
        return false;
    }

    return child->setParent(this, keepWorldTransform);
}

void SceneObject::removeChild(SceneObject* child, bool keepWorldTransform) {
    if (!child || child->parent_ != this) {
        return;
    }

    child->setParent(nullptr, keepWorldTransform);
}

void SceneObject::detach(bool keepWorldTransform) {
    setParent(nullptr, keepWorldTransform);
}

void SceneObject::detachChildren(bool keepWorldTransform) {
    auto children = children_;
    for (SceneObject* child : children) {
        removeChild(child, keepWorldTransform);
    }
}

glm::vec3 SceneObject::getPosition() const {
    if (!parent_) {
        return position_;
    }

    return parent_->getPosition() + position_;
}

glm::vec3 SceneObject::getSize() const {
    if (!parent_) {
        return size_;
    }

    return parent_->getSize() * size_;
}

glm::vec3 SceneObject::getRotation() const {
    if (!parent_) {
        return rotation_;
    }

    return parent_->getRotation() + rotation_;
}

bool SceneObject::isEffectivelyActive() const {
    if (!isActive_) {
        return false;
    }

    return !parent_ || parent_->isEffectivelyActive();
}

int SceneObject::getParentId() const {
    return parent_ ? parent_->getId() : -1;
}

bool SceneObject::wouldCreateCycle(const SceneObject* newParent) const {
    const SceneObject* current = newParent;
    while (current) {
        if (current == this) {
            return true;
        }
        current = current->parent_;
    }
    return false;
}

glm::vec3 SceneObject::divideScale(const glm::vec3& value, const glm::vec3& divisor) {
    glm::vec3 result(1.0f);
    const float epsilon = 0.0001f;

    result.x = std::fabs(divisor.x) < epsilon ? value.x : value.x / divisor.x;
    result.y = std::fabs(divisor.y) < epsilon ? value.y : value.y / divisor.y;
    result.z = std::fabs(divisor.z) < epsilon ? value.z : value.z / divisor.z;

    return result;
}
