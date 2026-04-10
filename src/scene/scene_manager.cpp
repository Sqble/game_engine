#include "scene_manager.h"

std::unordered_map<std::string, std::unique_ptr<Scene>> SceneManager::scenes_;
Scene* SceneManager::activeScene_ = nullptr;
