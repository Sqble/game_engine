#pragma once
#include "../scene/scene.h"

#include <string>

void ShowEngineUI(Scene* scene, int screenWidth, int screenHeight, SceneObject*& selectedMesh, bool& sceneEditingMode, Camera*& camera);


static void ShowSceneEditingWindow(Scene* scene, int screenWidth, int screenHeight, SceneObject*& selectedMesh, bool sceneEditingMode, Camera*& camera);
static void ShowSceneViewWindow(Scene* scene, SceneObject*& selectedMesh, bool sceneEditingMode);
static void ShowAssetListWindow(Scene* scene);
void RenderToast(int screenWidth);
void RenderInteractionPrompt(int screenWidth, int screenHeight, const std::string& prompt);
void ShowToast(const std::string& message, float durationSeconds = 4.0f);
