#pragma once

#include "../camera/camera.h"
#include "../cube/cube.h"
#include "../shader/shader.h"
#include "../plane/plane.h"
#include "../mesh/mesh.h"
#include "../pointlight/pointlight.h"
#include "../sceneobject/sceneobject.h"
#include "../sceneobject/lightobject.h"

#include <vector>
#include <string>
#include <memory>
#include "../includes/json.hpp"

using json = nlohmann::json;

const float ambientLight = 0.0f;

class Scene {
public:
    Scene(const std::string& name = "");
    ~Scene();

    void setName(const std::string& name) { name_ = name; }
    const std::string& getName() const { return name_; }

    void add(SceneObject* object);

    /* will need to do in the future
        Spotlight
    */

    const std::vector<SceneObject*>& getCubes() const;
    const std::vector<SceneObject*>& getPlanes() const;
    const std::vector<SceneObject*>& getMeshes() const;
    const std::vector<LightObject*>& getPointLights() const;
    const std::vector<LightObject*>& getLights() const { return lightObjects_; }
    const std::vector<Camera*>& getCameras() const { return cameras_; }


    void setActiveCamera(Camera* camera);
    Camera* getActiveCamera() const;

    void draw(Shader &shader);
    void drawShadowMap(Shader &shader);

    bool saveToFile(const std::string& filename) const;
    bool loadFromFile(const std::string& filename, int screenWidth, int screenHeight);
    Mesh* getGizmo() const { return gizmo_.get(); }
    void generateGizmo();
    void remove(SceneObject* object);

    std::vector<SceneObject*> getObjectsWithTag(const std::string& tag) const;
    SceneObject* getFirstObjectWithTag(const std::string& tag) const;
    Camera* getFirstCameraWithTag(const std::string& tag) const;
    SceneObject* getObjectWithId(int id) const;

    // Computes the light space matrix for the first active point light (for shadow mapping)
    glm::mat4 getLightSpaceMatrix() const;


private:
    std::string name_;
    Camera* activeCamera_;
    std::vector<std::unique_ptr<SceneObject>> objects_;
    std::vector<LightObject*> lightObjects_;
    std::vector<Camera*> cameras_;
    std::unique_ptr<Mesh> gizmo_;

    void clearObjects();
};

json serializeObject(const SceneObject* obj);
SceneObject* deserializeObject(const json& j, int screenWidth, int screenHeight);
