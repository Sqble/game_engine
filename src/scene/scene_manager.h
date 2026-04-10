#pragma once
#include "scene.h"
#include <memory>
#include <unordered_map>
#include <string>

class SceneManager {
public:
    static Scene* createScene(const std::string& name, bool setActive = true) {
        auto it = scenes_.find(name);
        if (it == scenes_.end()) {
            auto scene = std::make_unique<Scene>(name);
            Scene* scenePtr = scene.get();
            scenes_[name] = std::move(scene);
            if (setActive) {
                activeScene_ = scenePtr;
            }
            return scenePtr;
        }
        return it->second.get();
    }

    static Scene* getScene(const std::string& name) {
        auto it = scenes_.find(name);
        if (it != scenes_.end()) {
            return it->second.get();
        }
        return nullptr;
    }

    static void setActiveScene(const std::string& name) {
        auto it = scenes_.find(name);
        if (it != scenes_.end()) {
            activeScene_ = it->second.get();
        }
    }

    static Scene* getActiveScene() {
        return activeScene_;
    }

    static void clear() {
        scenes_.clear();
        activeScene_ = nullptr;
    }

private:
    static std::unordered_map<std::string, std::unique_ptr<Scene>> scenes_;
    static Scene* activeScene_;
};
