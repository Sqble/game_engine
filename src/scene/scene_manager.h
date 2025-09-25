#pragma once
#include "scene.h"
#include <unordered_map>
#include <string>

class SceneManager {
public:
    static Scene* createScene(const std::string& name, bool setActive = true) {
        auto it = scenes_.find(name);
        if (it == scenes_.end()) {
            Scene* scene = new Scene(name);
            scenes_[name] = scene;
            if (setActive) {
                activeScene_ = scene; 
            }
            return scene;
        }
        return it->second;
    }

    static Scene* getScene(const std::string& name) {
        auto it = scenes_.find(name);
        if (it != scenes_.end()) {
            return it->second;
        }
        return nullptr;
    }

    static void setActiveScene(const std::string& name) {
        auto it = scenes_.find(name);
        if (it != scenes_.end()) {
            activeScene_ = it->second;
        }
    }

    static Scene* getActiveScene() {
        return activeScene_;
    }

    static void clear() {
        for (auto& pair : scenes_) {
            delete pair.second;
        }
        scenes_.clear();
        activeScene_ = nullptr;
    }

private:
    static std::unordered_map<std::string, Scene*> scenes_;
    static Scene* activeScene_;
};