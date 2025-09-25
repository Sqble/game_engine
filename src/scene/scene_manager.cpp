#include "scene_manager.h"

std::unordered_map<std::string, Scene*> SceneManager::scenes_;
Scene* SceneManager::activeScene_ = nullptr;