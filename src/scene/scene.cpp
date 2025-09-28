#include "scene.h"
#include "../frustum/frustum.h"
#include <fstream>
#include <glm/gtc/matrix_transform.hpp>

Scene::Scene(const std::string& name) : name_(name), activeCamera_(nullptr) {
    //std::cout << "scene constructor called" << std::endl;
    generateGizmo();
}

void Scene::generateGizmo() {
    if (gizmo_) delete gizmo_;
    //std::cout << "Generating gizmo for scene: " << name_ << std::endl;
    gizmo_ = new Mesh("../assets/xyz_arrow.obj", Material("../assets/xyz_arrow.png"), glm::vec3(0,0,0), glm::vec3(1,1,1));
    //gizmo_->setActive(false); // Start inactive
}

Scene::~Scene() {
    for (SceneObject* object : objects_) {
        delete object;
    }
}

void Scene::setActiveCamera(Camera* camera) {
    activeCamera_ = camera;
}

Camera* Scene::getActiveCamera() const {
    return activeCamera_;
}

void Scene::draw(Shader &shader) {
    shader.setMat4("u_view", activeCamera_->getViewMatrix());
    shader.setMat4("u_projection", activeCamera_->getProjectionMatrix());
    shader.setVec3("viewPos", activeCamera_->getPosition());
    shader.setVec3("globalAmbient", glm::vec3());
    shader.setMat4("lightSpaceMatrix", getLightSpaceMatrix());

    // Set up lights FIRST before drawing any objects
    int i = 0;
    for (LightObject *light : lightObjects_) {
        if (light->isActive()) {
            light->draw(shader, i);
            ++i;
        }
    }
    shader.setInt("numLights", static_cast<int>(i));

    // Frustum culling setup
    Frustum frustum;
    glm::mat4 vp = activeCamera_->getProjectionMatrix() * activeCamera_->getViewMatrix();
    frustum.extract(vp);

    // Now draw all objects with proper lighting information
    //int objectsCulled = 0;
    for (SceneObject* object : objects_) {
        if (object->isDrawable() && object->isActive()) {
            // Frustum culling for meshes
            if (object->isMesh()) {
                Mesh* mesh = static_cast<Mesh*>(object);
                if (!frustum.isBoxVisible(mesh->getWorldAABB())) {
                    //objectsCulled++;
                    continue; // Cull mesh
                }
            }
            object->draw(shader);
        }
    }

    //draw gizmo
    if (gizmo_ && gizmo_->isActive()) {
        gizmo_->draw(shader);
    }

    //std::cout << "Culled " << objectsCulled << " objects this frame.\n";
}

void Scene::drawShadowMap(Shader &shader) {
    // Set up light's view/projection matrices
    shader.setMat4("lightSpaceMatrix", getLightSpaceMatrix());
    //int drawn = 0;
    // Only draw objects that cast shadows (meshes, cubes, planes)
    for (SceneObject* object : objects_) {
        if (!object->isActive() || !object->isDrawable()) continue;
        // Only draw shadow-casting objects (skip lights, cameras, gizmo)
        if (object->isMesh()) {
            if (object == gizmo_) continue;
            Mesh* mesh = static_cast<Mesh*>(object);
            mesh->shadowDraw(shader);
            //drawn++;
        }
    }
    //std::cout << "Drawn " << drawn << " objects to shadow map.\n";
}

void Scene::add(SceneObject* obj) {
    if (obj->isLight()) {
        lightObjects_.push_back(static_cast<LightObject*>(obj));
    }
    if (obj->isCamera()) {
        cameras_.push_back(static_cast<Camera*>(obj));
    }
    objects_.push_back(obj);
}

const std::vector<SceneObject*>& Scene::getCubes() const {
    static std::vector<SceneObject*> cubes;
    cubes.clear();
    for (SceneObject* obj : objects_) {
        if (obj->isCube()) cubes.push_back(obj);
    }
    return cubes;
}

const std::vector<SceneObject*>& Scene::getPlanes() const {
    static std::vector<SceneObject*> planes;
    planes.clear();
    for (SceneObject* obj : objects_) {
        if (obj->isPlane()) planes.push_back(obj);
    }
    return planes;
}

const std::vector<SceneObject*>& Scene::getMeshes() const {
    static std::vector<SceneObject*> meshes;
    meshes.clear();
    for (SceneObject* obj : objects_) {
        if (obj->isMesh()) meshes.push_back(obj);
    }
    return meshes;
}

const std::vector<LightObject*>& Scene::getPointLights() const {
    static std::vector<LightObject*> pointlights;
    pointlights.clear();
    for (LightObject* light : lightObjects_) {
        if (light->isPointLight()) pointlights.push_back(light);
    }
    return pointlights;
}

// Helper function to serialize a SceneObject
json serializeObject(const SceneObject* obj) {
    json j;
    j["type"] = obj->isCube() ? "cube" :
                obj->isPlane() ? "plane" :
                obj->isMesh() ? "mesh" :
                obj->isCamera() ? "camera" :
                obj->isLight() ? "light" : "unknown";
    j["position"] = { obj->getPosition().x, obj->getPosition().y, obj->getPosition().z };
    j["size"] = { obj->getSize().x, obj->getSize().y, obj->getSize().z };
    j["color"] = { obj->getColor().r, obj->getColor().g, obj->getColor().b };
    j["rotation"] = { obj->getRotation().x, obj->getRotation().y, obj->getRotation().z };
    j["active"] = obj->isActive();
    j["tag"] = obj->getTag();
    j["id"] = obj->getId();
    j["parentId"] = obj->getParentId();
    j["isParent"] = obj->isParent();

    if (obj->isMesh()) {
        const Mesh* mesh = static_cast<const Mesh*>(obj);
        if (mesh->material.hasAlbedoMap())
            j["texture"] = mesh->material.getAlbedoPath();
        if (mesh->material.hasRoughnessMap())
            j["roughnessMap"] = mesh->material.getRoughPath();
        if (mesh->material.hasMetalnessMap())
            j["metalnessMap"] = mesh->material.getMetalPath();
        if (mesh->material.hasNormalMap())
            j["normalMap"] = mesh->material.getNormalPath();
        if (!mesh->objPath.empty())
            j["objPath"] = mesh->objPath;
    }

    if (obj->isLight()) {
        const LightObject* light = static_cast<const LightObject*>(obj);
        j["intensity"] = light->getIntensity();
    }

    return j;
}

bool Scene::saveToFile(const std::string& filename) const {
    json jscene;
    jscene["name"] = name_;
    jscene["objects"] = json::array();
    for (const SceneObject* obj : objects_) {
        if (obj->isMesh()) {
            const Mesh* mesh = static_cast<const Mesh*>(obj);
            if (mesh->material.getAlbedoPath() == "../assets/xyz_arrow.obj") continue; // Skip gizmo
        }
        jscene["objects"].push_back(serializeObject(obj));
    }
    // Optionally save active camera index
    int camIdx = -1;
    for (size_t i = 0; i < cameras_.size(); ++i) {
        if (cameras_[i] == activeCamera_) {
            camIdx = static_cast<int>(i);
            break;
        }
    }
    jscene["activeCamera"] = camIdx;

    std::ofstream out(filename);
    if (!out) return false;
    out << jscene.dump(4);
    return true;
}

// Helper function to deserialize a SceneObject (basic, extend as needed)
SceneObject* deserializeObject(const json& j, int screenWidth, int screenHeight) {
    glm::vec3 position(j["position"][0], j["position"][1], j["position"][2]);
    glm::vec3 size(j["size"][0], j["size"][1], j["size"][2]);
    glm::vec3 color(j["color"][0], j["color"][1], j["color"][2]);
    glm::vec3 rotation(j["rotation"][0], j["rotation"][1], j["rotation"][2]);
    bool active = j.value("active", true);
    std::string tag = j.value("tag", "");
    int id = j.value("id", 0);
    if (id > sceneObjectIdCounter) {
        SceneObject::setIDCounter(id + 1);
    }
    int parentId = j.value("parentId", -1);
    bool isParent = j.value("isParent", false);

    std::string type = j.value("type", "unknown");
    if (type == "cube") {
        Cube *r = new ParentObject<Cube>(position, size, color, rotation, active);
        r->setTag(tag);
        r->setParentId(parentId);
        if (isParent) r->willBeParent();
        if (id != 0) r->setId(id);
        return r;
    } else if (type == "plane") {
        Plane *r = new ParentObject<Plane>(position, size, color, rotation, active);
        r->setTag(tag);
        r->setParentId(parentId);
        if (isParent) r->willBeParent();
        if (id != 0) r->setId(id);
        return r;
    } else if (type == "mesh") {
        std::string objPath = j.value("objPath", "");
        std::string texturePath = j.value("texture", "");
        std::string roughPath = j.value("roughnessMap", "");
        std::string metalPath = j.value("metalnessMap", "");
        std::string normalPath = j.value("normalMap", "");
        Material mat = Material(texturePath, roughPath, metalPath, normalPath);
        /*
        if (!roughPath.empty()) mat.setRoughnessMap(roughPath);
        if (!metalPath.empty()) mat.setMetalnessMap(metalPath);
        if (!normalPath.empty()) mat.setNormalMap(normalPath);
        */
        Mesh* mesh = new ParentObject<Mesh>(objPath, mat, position, size, color, rotation, active);
        mesh->setTag(tag);
        mesh->setParentId(parentId);
        if (isParent) mesh->willBeParent();
        if (id != 0) mesh->setId(id);
        return mesh;
    } else if (type == "camera") {
        Camera* cam = new ParentObject<Camera>(screenWidth, screenHeight, position, rotation);
        cam->setTag(tag);
        cam->setParentId(parentId);
        if (isParent) cam->willBeParent();
        if (id != 0) cam->setId(id);
        return cam;
    } else if (type == "light") {
        int intensity = j.value("intensity", 1.0f);
        PointLight* light = new ParentObject<PointLight>(position, color, intensity, active);
        light->setTag(tag);
        light->setParentId(parentId);
        if (isParent) light->willBeParent();
        if (id != 0) light->setId(id);
        return light;
    }
    return nullptr;
}

bool Scene::loadFromFile(const std::string& filename, int screenWidth, int screenHeight) {

    std::ifstream in(filename);
    if (!in) return false;

    json jscene;
    in >> jscene;

    if (jscene.contains("name")) {
        name_ = jscene["name"].get<std::string>();
    }

    // Clean up existing objects
    //for (SceneObject* obj : objects_) delete obj;

    objects_.clear();
    lightObjects_.clear();
    cameras_.clear();
    activeCamera_ = nullptr;

    // Load objects
    for (const auto& jObj : jscene["objects"]) {
        SceneObject* obj = deserializeObject(jObj, screenWidth, screenHeight);
        if (obj) add(obj);
    }

    // setup parent-child relationship
    for (SceneObject* obj : objects_) {
        if (obj->isParent()) {
            //std::cout << "parent found with id: " << obj->getId() << std::endl;
            int parentId = obj->getId();
            for (SceneObject* child : objects_) {
                if (child->getParentId() == parentId) {
                    //std::cout << "  adding child with id: " << child->getId() << std::endl;
                    obj->addChild(child);
                }
            }
        }
    }

    // Restore active camera
    int camIdx = jscene.value("activeCamera", -1);
    if (camIdx >= 0 && camIdx < static_cast<int>(cameras_.size())) {
        activeCamera_ = cameras_[camIdx];
    }

    // Regenerate gizmo
    generateGizmo();

    return true;
}

void Scene::remove(SceneObject* object) {
    // Remove from objects_
    objects_.erase(std::remove(objects_.begin(), objects_.end(), object), objects_.end());

    // Remove from lightObjects_ if it's a LightObject
    if (auto light = dynamic_cast<LightObject*>(object)) {
        lightObjects_.erase(std::remove(lightObjects_.begin(), lightObjects_.end(), light), lightObjects_.end());
    }

    // Remove from cameras_ if it's a Camera
    if (auto cam = dynamic_cast<Camera*>(object)) {
        cameras_.erase(std::remove(cameras_.begin(), cameras_.end(), cam), cameras_.end());
        // Optionally: handle activeCamera_ if needed
        if (activeCamera_ == cam) activeCamera_ = nullptr;
    }
}

std::vector<SceneObject*> Scene::getObjectsWithTag(const std::string& tag) const {
        std::vector<SceneObject*> objects;
        for (SceneObject* obj : objects_) {
            if (obj->getTag() == tag) {
                objects.push_back(obj);
            }
        }
        return objects;
}

SceneObject* Scene::getFirstObjectWithTag(const std::string& tag) const {
    for (SceneObject* obj : objects_) {
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
    for (SceneObject* obj : objects_) {
        if (obj->getId() == id) {
            return obj;
        }
    }
    return nullptr;
}

glm::mat4 Scene::getLightSpaceMatrix() const {
    // Use the first active point light
    const auto& lights = getPointLights();
    if (lights.empty()) {
        // Fallback: return identity
        return glm::mat4(1.0f);
    }
    const LightObject* light = lights[0];
    glm::vec3 lightPos = light->getPosition();
    glm::vec3 target = glm::vec3(0.0f, 0.0f, 1.0f); // Center of scene (could be improved)
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

    // For shadow mapping, use an orthographic projection (directional) or perspective (spot/point)
    // We'll use perspective since we have point lights
    float near_plane = 0.01f, far_plane = 12.0f;
    float fov = glm::radians(90.0f); // 90 degree field of view
    float aspect = 1.0f; // square shadow map
    //std::cout << "[Shadow Debug] Light position: (" << lightPos.x << ", " << lightPos.y << ", " << lightPos.z << ")\n";
    //std::cout << "[Shadow Debug] Perspective FOV: 90 deg, Near/Far: " << near_plane << ", " << far_plane << std::endl;

    glm::mat4 lightProjection = glm::perspective(fov, aspect, near_plane, far_plane);
    glm::mat4 lightView = glm::lookAt(lightPos, target, up);
    return lightProjection * lightView;
}