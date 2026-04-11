#include "scene.h"

#include "../frustum/frustum.h"

#include <cmath>
#include <fstream>
#include <glm/gtc/matrix_transform.hpp>
#include <utility>

namespace {
int getSerializedParentId(const json& j) {
    if (j.contains("parent") && !j["parent"].is_null()) {
        return j.value("parent", -1);
    }
    return j.value("parentId", -1);
}

void applyLegacyInteractionDefaults(SceneObject* object, const std::string& tag) {
    if (!object) {
        return;
    }

    if (tag == "computer") {
        object->setInteractable(true);
        object->setInteractionType("computer");
        object->setInteractionPrompt("[E] Use Computer");
    } else if (tag == "locker") {
        object->setInteractable(true);
        object->setInteractionType("locker");
        object->setInteractionPrompt("[E] Open Locker");
    }
}

glm::vec3 chooseUpVector(const glm::vec3& forward) {
    const glm::vec3 normalizedForward = glm::normalize(forward);
    const glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
    if (std::fabs(glm::dot(normalizedForward, worldUp)) > 0.99f) {
        return glm::vec3(0.0f, 0.0f, 1.0f);
    }
    return worldUp;
}

glm::mat4 buildSpotLightSpaceMatrix(const SpotLight* spotLight) {
    if (!spotLight) {
        return glm::mat4(1.0f);
    }

    const glm::vec3 lightPos = spotLight->getPosition();
    const glm::vec3 direction = glm::normalize(spotLight->getDirection());
    const glm::mat4 lightProjection = glm::perspective(
        glm::radians(glm::clamp(spotLight->getCutoff() * 2.0f, 10.0f, 175.0f)),
        1.0f,
        0.05f,
        50.0f
    );
    const glm::mat4 lightView = glm::lookAt(lightPos, lightPos + direction, chooseUpVector(direction));
    return lightProjection * lightView;
}
}

Scene::Scene(const std::string& name) : name_(name), activeCamera_(nullptr) {
    generateGizmo();
}

void Scene::generateGizmo() {
    gizmo_ = std::make_unique<Mesh>(
        "assets/xyz_arrow.obj",
        Material("assets/xyz_arrow.png"),
        glm::vec3(0, 0, 0),
        glm::vec3(1, 1, 1)
    );
}

Scene::~Scene() {
    clearObjects();
}

void Scene::clearObjects() {
    objects_.clear();
    lightObjects_.clear();
    cameras_.clear();
    activeCamera_ = nullptr;
}

void Scene::setActiveCamera(Camera* camera) {
    activeCamera_ = camera;
}

Camera* Scene::getActiveCamera() const {
    return activeCamera_;
}

void Scene::draw(Shader& shader) {
    if (!activeCamera_) {
        return;
    }

    shader.setMat4("u_view", activeCamera_->getViewMatrix());
    shader.setMat4("u_projection", activeCamera_->getProjectionMatrix());
    shader.setVec3("viewPos", activeCamera_->getPosition());
    shader.setVec3("globalAmbient", glm::vec3());
    shader.setMat4("lightSpaceMatrix", getLightSpaceMatrix());

    shader.setVec3("fogColor", glm::vec3(0.4f, 0.55f, 0.3f));
    shader.setFloat("fogDensity", 0.03f);
    shader.setFloat("fogHeight", -3.0f);

    int numPointLights = 0;
    int numSpotLights = 0;
    int shadowedLightType = 0;
    int shadowedLightIndex = -1;
    for (LightObject* light : lightObjects_) {
        if (!light->isEffectivelyActive()) {
            continue;
        }

        if (light->isPointLight()) {
            if (shadowedLightType == 0) {
                shadowedLightType = 1;
                shadowedLightIndex = numPointLights;
            }
            light->draw(shader, numPointLights);
            numPointLights++;
        } else {
            if (shadowedLightType != 2) {
                shadowedLightType = 2;
                shadowedLightIndex = numSpotLights;
            }
            light->draw(shader, numSpotLights);
            numSpotLights++;
        }
    }
    shader.setInt("numPointLights", numPointLights);
    shader.setInt("numSpotLights", numSpotLights);
    shader.setInt("shadowedLightType", shadowedLightType);
    shader.setInt("shadowedLightIndex", shadowedLightIndex);

    Frustum frustum;
    glm::mat4 vp = activeCamera_->getProjectionMatrix() * activeCamera_->getViewMatrix();
    frustum.extract(vp);

    for (const auto& objectPtr : objects_) {
        SceneObject* object = objectPtr.get();
        if (!object->isDrawable() || !object->isEffectivelyActive()) {
            continue;
        }

        if (object->isMesh()) {
            Mesh* mesh = static_cast<Mesh*>(object);
            if (!frustum.isBoxVisible(mesh->getWorldAABB())) {
                continue;
            }
        }

        object->draw(shader);
    }

    if (gizmo_ && gizmo_->isEffectivelyActive()) {
        gizmo_->draw(shader);
    }
}

void Scene::drawShadowMap(Shader& shader) {
    shader.setMat4("lightSpaceMatrix", getLightSpaceMatrix());

    for (const auto& objectPtr : objects_) {
        SceneObject* object = objectPtr.get();
        if (!object->isDrawable() || !object->isEffectivelyActive()) {
            continue;
        }

        if (object->isMesh()) {
            Mesh* mesh = static_cast<Mesh*>(object);
            mesh->shadowDraw(shader);
        }
    }
}

void Scene::add(SceneObject* obj) {
    if (!obj) {
        return;
    }

    if (obj->isLight()) {
        lightObjects_.push_back(static_cast<LightObject*>(obj));
    }
    if (obj->isCamera()) {
        cameras_.push_back(static_cast<Camera*>(obj));
        if (!activeCamera_) {
            activeCamera_ = static_cast<Camera*>(obj);
        }
    }

    objects_.push_back(std::unique_ptr<SceneObject>(obj));
}

const std::vector<SceneObject*>& Scene::getCubes() const {
    static std::vector<SceneObject*> cubes;
    cubes.clear();
    for (const auto& objectPtr : objects_) {
        SceneObject* obj = objectPtr.get();
        if (obj->isCube()) {
            cubes.push_back(obj);
        }
    }
    return cubes;
}

const std::vector<SceneObject*>& Scene::getPlanes() const {
    static std::vector<SceneObject*> planes;
    planes.clear();
    for (const auto& objectPtr : objects_) {
        SceneObject* obj = objectPtr.get();
        if (obj->isPlane()) {
            planes.push_back(obj);
        }
    }
    return planes;
}

const std::vector<SceneObject*>& Scene::getMeshes() const {
    static std::vector<SceneObject*> meshes;
    meshes.clear();
    for (const auto& objectPtr : objects_) {
        SceneObject* obj = objectPtr.get();
        if (obj->isMesh()) {
            meshes.push_back(obj);
        }
    }
    return meshes;
}

const std::vector<LightObject*>& Scene::getPointLights() const {
    static std::vector<LightObject*> pointlights;
    pointlights.clear();
    for (LightObject* light : lightObjects_) {
        if (light->isPointLight()) {
            pointlights.push_back(light);
        }
    }
    return pointlights;
}

json serializeObject(const SceneObject* obj) {
    json j;
    j["type"] = obj->isCube() ? "cube" :
                obj->isPlane() ? "plane" :
                obj->isMesh() ? "mesh" :
                obj->isCamera() ? "camera" :
                obj->isSpotLight() ? "spotlight" :
                obj->isLight() ? "light" : "unknown";
    j["position"] = { obj->getPosition().x, obj->getPosition().y, obj->getPosition().z };
    j["size"] = { obj->getSize().x, obj->getSize().y, obj->getSize().z };
    j["color"] = { obj->getColor().r, obj->getColor().g, obj->getColor().b };
    j["rotation"] = { obj->getRotation().x, obj->getRotation().y, obj->getRotation().z };
    j["localPosition"] = { obj->getLocalPosition().x, obj->getLocalPosition().y, obj->getLocalPosition().z };
    j["localSize"] = { obj->getLocalSize().x, obj->getLocalSize().y, obj->getLocalSize().z };
    j["localRotation"] = { obj->getLocalRotation().x, obj->getLocalRotation().y, obj->getLocalRotation().z };
    j["active"] = obj->isActive();
    j["tag"] = obj->getTag();
    j["id"] = obj->getId();
    j["interactable"] = obj->isInteractable();
    j["interactionType"] = obj->getInteractionType();
    j["interactionPrompt"] = obj->getInteractionPrompt();
    j["parentId"] = obj->getParentId();
    j["parent"] = obj->getParentId();
    j["isParent"] = obj->isParent();
    j["childIds"] = json::array();
    for (SceneObject* child : obj->getChildren()) {
        j["childIds"].push_back(child->getId());
    }

    if (obj->isMesh()) {
        const Mesh* mesh = static_cast<const Mesh*>(obj);
        if (mesh->material.hasAlbedoMap()) {
            j["texture"] = mesh->material.getAlbedoPath();
        }
        if (mesh->material.hasRoughnessMap()) {
            j["roughnessMap"] = mesh->material.getRoughPath();
        }
        if (mesh->material.hasMetalnessMap()) {
            j["metalnessMap"] = mesh->material.getMetalPath();
        }
        if (mesh->material.hasNormalMap()) {
            j["normalMap"] = mesh->material.getNormalPath();
        }
        if (!mesh->objPath.empty()) {
            j["objPath"] = mesh->objPath;
        }
    }

    if (obj->isLight()) {
        const LightObject* light = static_cast<const LightObject*>(obj);
        j["intensity"] = light->getIntensity();
        if (obj->isSpotLight()) {
            const SpotLight* spotLight = static_cast<const SpotLight*>(obj);
            j["direction"] = {
                spotLight->getDirection().x,
                spotLight->getDirection().y,
                spotLight->getDirection().z,
            };
            j["cutoff"] = spotLight->getCutoff();
        }
    }

    return j;
}

bool Scene::saveToFile(const std::string& filename) const {
    json jscene;
    jscene["schemaVersion"] = 2;
    jscene["name"] = name_;
    jscene["objects"] = json::array();
    for (const auto& objectPtr : objects_) {
        jscene["objects"].push_back(serializeObject(objectPtr.get()));
    }

    int camIdx = -1;
    for (size_t i = 0; i < cameras_.size(); ++i) {
        if (cameras_[i] == activeCamera_) {
            camIdx = static_cast<int>(i);
            break;
        }
    }
    jscene["activeCamera"] = camIdx;

    std::ofstream out(filename);
    if (!out) {
        return false;
    }
    out << jscene.dump(4);
    return true;
}

SceneObject* deserializeObject(const json& j, int screenWidth, int screenHeight) {
    glm::vec3 position(j["position"][0], j["position"][1], j["position"][2]);
    glm::vec3 size(j["size"][0], j["size"][1], j["size"][2]);
    glm::vec3 color(j["color"][0], j["color"][1], j["color"][2]);
    glm::vec3 rotation(j["rotation"][0], j["rotation"][1], j["rotation"][2]);
    bool active = j.value("active", true);
    std::string tag = j.value("tag", "");
    int id = j.value("id", 0);
    if (id >= SceneObject::nextId()) {
        SceneObject::setIDCounter(id + 1);
    }

    std::string type = j.value("type", "unknown");
    SceneObject* object = nullptr;

    if (type == "cube") {
        object = new Cube(position, size, color, rotation, active);
    } else if (type == "plane") {
        object = new Plane(position, glm::vec2(size.x, size.z), rotation, color, active);
    } else if (type == "mesh") {
        std::string objPath = j.value("objPath", "");
        std::string texturePath = j.value("texture", "");
        std::string roughPath = j.value("roughnessMap", "");
        std::string metalPath = j.value("metalnessMap", "");
        std::string normalPath = j.value("normalMap", "");
        Material mat(texturePath, roughPath, metalPath, normalPath);
        object = new Mesh(objPath, mat, position, size, color, rotation, active);
    } else if (type == "camera") {
        object = new Camera(screenWidth, screenHeight, position, rotation);
        object->setActive(active);
    } else if (type == "spotlight") {
        glm::vec3 direction(0.0f, -1.0f, 0.0f);
        if (j.contains("direction")) {
            direction = glm::vec3(j["direction"][0], j["direction"][1], j["direction"][2]);
        }
        float cutoff = j.value("cutoff", 12.5f);
        float intensity = j.value("intensity", 1.0f);
        object = new SpotLight(position, direction, color, intensity, cutoff, active);
    } else if (type == "light") {
        float intensity = j.value("intensity", 1.0f);
        object = new PointLight(position, color, intensity, active);
    }

    if (!object) {
        return nullptr;
    }

    object->setTag(tag);
    const bool hasInteractionFields = j.contains("interactable") || j.contains("interactionType") || j.contains("interactionPrompt");
    if (hasInteractionFields) {
        const std::string interactionType = j.value("interactionType", "");
        const std::string interactionPrompt = j.value("interactionPrompt", "");
        const bool interactable = j.value("interactable", !interactionType.empty() || !interactionPrompt.empty());
        object->setInteractable(interactable);
        object->setInteractionType(interactionType);
        object->setInteractionPrompt(interactionPrompt);
    } else {
        applyLegacyInteractionDefaults(object, tag);
    }

    if (id != 0) {
        object->setId(id);
    }

    return object;
}

bool Scene::loadFromFile(const std::string& filename, int screenWidth, int screenHeight) {
    std::ifstream in(filename);
    if (!in) {
        return false;
    }

    json jscene;
    in >> jscene;

    if (jscene.contains("name")) {
        name_ = jscene["name"].get<std::string>();
    }

    clearObjects();

    std::vector<std::pair<SceneObject*, int>> pendingParents;
    for (const auto& jObj : jscene["objects"]) {
        SceneObject* obj = deserializeObject(jObj, screenWidth, screenHeight);
        if (!obj) {
            continue;
        }

        add(obj);
        pendingParents.push_back({obj, getSerializedParentId(jObj)});
    }

    for (const auto& [child, parentId] : pendingParents) {
        if (parentId == -1) {
            continue;
        }

        SceneObject* parent = getObjectWithId(parentId);
        if (parent) {
            child->setParent(parent, true);
        }
    }

    int camIdx = jscene.value("activeCamera", -1);
    if (camIdx >= 0 && camIdx < static_cast<int>(cameras_.size())) {
        activeCamera_ = cameras_[camIdx];
    } else if (!cameras_.empty()) {
        activeCamera_ = cameras_[0];
    }

    generateGizmo();

    return true;
}

void Scene::remove(SceneObject* object) {
    if (!object) {
        return;
    }

    if (object->getParent()) {
        object->detach(true);
    }
    object->detachChildren(true);

    if (auto light = dynamic_cast<LightObject*>(object)) {
        lightObjects_.erase(std::remove(lightObjects_.begin(), lightObjects_.end(), light), lightObjects_.end());
    }

    if (auto cam = dynamic_cast<Camera*>(object)) {
        cameras_.erase(std::remove(cameras_.begin(), cameras_.end(), cam), cameras_.end());
        if (activeCamera_ == cam) {
            activeCamera_ = cameras_.empty() ? nullptr : cameras_.front();
        }
    }

    objects_.erase(
        std::remove_if(
            objects_.begin(),
            objects_.end(),
            [object](const std::unique_ptr<SceneObject>& candidate) {
                return candidate.get() == object;
            }
        ),
        objects_.end()
    );
}

std::vector<SceneObject*> Scene::getObjectsWithTag(const std::string& tag) const {
    std::vector<SceneObject*> objects;
    for (const auto& objectPtr : objects_) {
        SceneObject* obj = objectPtr.get();
        if (obj->getTag() == tag) {
            objects.push_back(obj);
        }
    }
    return objects;
}

SceneObject* Scene::getFirstObjectWithTag(const std::string& tag) const {
    for (const auto& objectPtr : objects_) {
        SceneObject* obj = objectPtr.get();
        if (obj->getTag() == tag) {
            return obj;
        }
    }
    return nullptr;
}

Camera* Scene::getFirstCameraWithTag(const std::string& tag) const {
    for (Camera* cam : cameras_) {
        if (cam->getTag() == tag) {
            return cam;
        }
    }
    return nullptr;
}

SceneObject* Scene::getObjectWithId(int id) const {
    for (const auto& objectPtr : objects_) {
        SceneObject* obj = objectPtr.get();
        if (obj->getId() == id) {
            return obj;
        }
    }
    return nullptr;
}

glm::mat4 Scene::getLightSpaceMatrix() const {
    for (LightObject* light : lightObjects_) {
        if (light->isEffectivelyActive() && light->isSpotLight()) {
            return buildSpotLightSpaceMatrix(static_cast<const SpotLight*>(light));
        }
    }

    for (LightObject* light : lightObjects_) {
        if (!light->isEffectivelyActive() || !light->isPointLight()) {
            continue;
        }

        const glm::vec3 lightPos = light->getPosition();
        const glm::vec3 target = glm::vec3(0.0f, 0.0f, 0.0f);
        const glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

        const glm::mat4 lightProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.05f, 25.0f);
        const glm::mat4 lightView = glm::lookAt(lightPos, target, up);
        return lightProjection * lightView;
    }

    return glm::mat4(1.0f);
}
