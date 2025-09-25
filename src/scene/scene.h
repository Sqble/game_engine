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
#include "../includes/json.hpp"

const float ambientLight = 0.1f;

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
    const std::vector<Camera*>& getCameras() const { return cameras_; }


    void setActiveCamera(Camera* camera);
    Camera* getActiveCamera() const;
    void draw(Shader &shader);
    bool saveToFile(const std::string& filename) const;
    bool loadFromFile(const std::string& filename, int screenWidth, int screenHeight);
    Mesh* getGizmo() const { return gizmo_; }
    void generateGizmo();
    void remove(SceneObject* object);


private:
    std::string name_;
    Camera* activeCamera_;
    std::vector<SceneObject*> objects_;
    std::vector<LightObject*> lightObjects_;
    std::vector<Camera*> cameras_;
    Mesh* gizmo_ = nullptr;
};