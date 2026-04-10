#include "engine_ui.h"
#include "../objmanager/objmanager.h"
#include "../mesh/mesh.h"
#include "../pointlight/pointlight.h"
#include "../undo/undo.h"

#include "../includes/imgui/imgui.h"
#include "../includes/imgui/imgui_impl_glfw.h"
#include "../includes/imgui/imgui_impl_opengl3.h"
#include "../includes/imgui/imgui_internal.h" 
#include "../includes/json.hpp"

#include <chrono>
#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include <sstream>

static std::string toastMessage;
static std::chrono::steady_clock::time_point toastEndTime;

void ShowToast(const std::string& message, float durationSeconds) {
    toastMessage = message;
    toastEndTime = std::chrono::steady_clock::now() + std::chrono::milliseconds((int)(durationSeconds * 1000));
}

void RenderToast(int screenWidth) {
    if (toastMessage.empty()) return;
    auto now = std::chrono::steady_clock::now();
    if (now > toastEndTime) {
        toastMessage.clear();
        return;
    }

    // Position at top left, full width
    ImVec2 pos = ImVec2(0, 0);
    ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2((float)screenWidth, 40), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.85f); // semi-transparent

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings |
                             ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav |
                             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_AlwaysAutoResize |
                             ImGuiWindowFlags_NoInputs;

    ImGui::Begin("##ToastBar", nullptr, flags);
    ImGui::SetCursorPosY(10); // Center vertically
    ImGui::SetCursorPosX((screenWidth - ImGui::CalcTextSize(toastMessage.c_str()).x) * 0.5f); // Center horizontally
    ImGui::Text("%s", toastMessage.c_str());
    ImGui::End();
}

void RenderInteractionPrompt(int screenWidth, int screenHeight, const std::string& prompt) {
    if (prompt.empty()) {
        return;
    }

    ImGui::SetNextWindowPos(ImVec2(screenWidth * 0.5f, screenHeight - 32.0f), ImGuiCond_Always, ImVec2(0.5f, 1.0f));
    ImGui::SetNextWindowBgAlpha(0.8f);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings |
                             ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav |
                             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_AlwaysAutoResize |
                             ImGuiWindowFlags_NoInputs;

    ImGui::Begin("##InteractionPrompt", nullptr, flags);
    ImGui::TextUnformatted(prompt.c_str());
    ImGui::End();
}

static UndoManager undoManager;

static void RestoreHierarchyFromState(Scene* scene, SceneObject* obj, const json& state) {
    if (!scene || !obj) {
        return;
    }

    const int parentId = state.contains("parent") ? state.value("parent", -1) : state.value("parentId", -1);
    if (parentId != -1) {
        SceneObject* parent = scene->getObjectWithId(parentId);
        if (parent) {
            obj->setParent(parent, true);
        }
    }

    if (state.contains("childIds")) {
        for (const auto& childIdValue : state["childIds"]) {
            SceneObject* child = scene->getObjectWithId(childIdValue.get<int>());
            if (child) {
                child->setParent(obj, true);
            }
        }
    }
}

static void ApplySerializedState(Scene* scene, SceneObject* obj, const json& state, int screenWidth, int screenHeight) {
    if (!scene || !obj) {
        return;
    }

    SceneObject* newState = deserializeObject(state, screenWidth, screenHeight);
    if (!newState) {
        return;
    }

    obj->setPosition(newState->getPosition());
    obj->setRotation(newState->getRotation());
    obj->setSize(newState->getSize());
    obj->setColor(newState->getColor());
    obj->setActive(newState->isActive());

    if (auto light = dynamic_cast<LightObject*>(obj)) {
        if (auto newLight = dynamic_cast<LightObject*>(newState)) {
            light->setIntensity(newLight->getIntensity());
        }
    }

    if (auto spotLight = dynamic_cast<SpotLight*>(obj)) {
        if (auto newSpotLight = dynamic_cast<SpotLight*>(newState)) {
            spotLight->setDirection(newSpotLight->getDirection());
            spotLight->setCutoff(newSpotLight->getCutoff());
        }
    }

    RestoreHierarchyFromState(scene, obj, state);
    delete newState;
}

void ShowEngineUI(Scene* scene, int screenWidth, int screenHeight, SceneObject*& selectedMesh, bool& sceneEditingMode, Camera*& camera) {

    // Handle Undo/Redo shortcuts (Ctrl+Z, Ctrl+Shift+Z)
    ImGuiIO& io = ImGui::GetIO();
    static bool lastUndoPressed = false;
    static bool lastRedoPressed = false;
    bool undoPressed = io.KeyCtrl && !io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_Z, false);
    bool redoPressed = io.KeyCtrl && io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_Z, false);

    if (undoPressed && !lastUndoPressed && undoManager.canUndo()) {
        UndoManager::Action action = undoManager.popUndo();
        // TODO: Apply undo action to scene (requires implementation)
        //ShowToast("Undo: " + std::to_string(action.type));
        if (action.type == UndoManager::Action::Duplicate) { //to undo duplicate we remove the object
            scene->remove(scene->getObjectWithId(action.objectId));
            ShowToast("Removed duplicated object with ID: " + std::to_string(action.objectId));
        }
        if (action.type == UndoManager::Action::Create) { //to undo create we remove the object
            scene->remove(scene->getObjectWithId(action.objectId));
            ShowToast("Removed created object with ID: " + std::to_string(action.objectId));
        }
        if (action.type == UndoManager::Action::Delete) { //to undo delete we recreate the object
            SceneObject* obj = deserializeObject(action.objectStateBefore, screenWidth, screenHeight);
            if (obj) {
                scene->add(obj);
                RestoreHierarchyFromState(scene, obj, action.objectStateBefore);
                ShowToast("Restored deleted object with ID: " + std::to_string(action.objectId));
            }
        }
        if (action.type == UndoManager::Action::Transform) { //to undo transform we restore the previous state
            SceneObject* obj = scene->getObjectWithId(action.objectId);
            if (obj) {
                ApplySerializedState(scene, obj, action.objectStateBefore, screenWidth, screenHeight);
                ShowToast("Reverted transform of object with ID: " + std::to_string(action.objectId));
            }
        }
    }
    lastUndoPressed = undoPressed;

    if (redoPressed && !lastRedoPressed && undoManager.canRedo()) {
        UndoManager::Action action = undoManager.popRedo();
        // TODO: Apply redo action to scene (requires implementation)
        //ShowToast("Redo: " + std::to_string(action.type));
        if (action.type == UndoManager::Action::Duplicate) { //to redo duplicate we recreate the object
            SceneObject* obj = deserializeObject(action.objectStateAfter, screenWidth, screenHeight);
            if (obj) {
                scene->add(obj);
                RestoreHierarchyFromState(scene, obj, action.objectStateAfter);
                ShowToast("Re-duplicated object with ID: " + std::to_string(action.objectId));
            }
        }
        if (action.type == UndoManager::Action::Create) { //to redo create we recreate the object
            SceneObject* obj = deserializeObject(action.objectStateAfter, screenWidth, screenHeight);
            if (obj) {
                scene->add(obj);
                RestoreHierarchyFromState(scene, obj, action.objectStateAfter);
                ShowToast("Re-created object with ID: " + std::to_string(action.objectId));
            }
        }
        if (action.type == UndoManager::Action::Delete) { //to redo delete we remove the object
            scene->remove(scene->getObjectWithId(action.objectId));
            ShowToast("Re-deleted object with ID: " + std::to_string(action.objectId));
        }
        if (action.type == UndoManager::Action::Transform) { //to redo transform we restore the new state
            SceneObject* obj = scene->getObjectWithId(action.objectId);
            if (obj) {
                ApplySerializedState(scene, obj, action.objectStateAfter, screenWidth, screenHeight);
                ShowToast("Re-applied transform of object with ID: " + std::to_string(action.objectId));
            }
        }
    }
    lastRedoPressed = redoPressed;

    if (!sceneEditingMode) {
        return;
    }

    static bool dockInit = false;
    static int lastWidth = 0, lastHeight = 0;
    static ImGuiID dockspace_id = 0;
    bool needsReset = (screenWidth != lastWidth || screenHeight != lastHeight);
    if (needsReset) {
        dockInit = false;
        lastWidth = screenWidth;
        lastHeight = screenHeight;
    }
    
    ImGuiWindowFlags window_flags =  ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar;
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2((float)screenWidth, (float)screenHeight), ImGuiCond_Always);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    
    // Use unique name for each size to force reset
    std::string dockName = "EngineDockSpace_" + std::to_string(screenWidth) + "x" + std::to_string(screenHeight);
    ImGui::Begin(dockName.c_str(), nullptr, window_flags);

    dockspace_id = ImGui::GetID("EngineDockSpace");
    if (!dockInit || needsReset) {
        dockInit = true;
        ImGui::DockBuilderRemoveNode(dockspace_id);
        ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspace_id, ImVec2((float)screenWidth, (float)screenHeight));

        ImGuiID dock_main_id = dockspace_id;
        ImGuiID dock_left = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.25f, nullptr, &dock_main_id);
        ImGuiID dock_right = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.33f, nullptr, &dock_main_id);
        ImGuiID dock_bottom = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.25f, nullptr, &dock_main_id);

        ImGui::DockBuilderDockWindow("Scene View", dock_left);
        ImGui::DockBuilderDockWindow("Scene Editing", dock_right);
        ImGui::DockBuilderDockWindow("Asset List", dock_bottom);
        ImGui::DockBuilderFinish(dockspace_id);
    }
    ImGui::DockSpace(dockspace_id, ImVec2(0, 0), ImGuiDockNodeFlags_PassthruCentralNode);
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();

    ShowSceneEditingWindow(scene, screenWidth, screenHeight, selectedMesh, sceneEditingMode, camera);
    ShowSceneViewWindow(scene, selectedMesh, sceneEditingMode);
    ShowAssetListWindow(scene);

    ImGui::End(); // End EngineDockSpace
}


// --- Scene Editing Window ---
static void ShowSceneEditingWindow(Scene* scene, int screenWidth, int screenHeight, SceneObject*& selectedMesh, bool sceneEditingMode, Camera*& camera) {
    ImGui::Begin("Scene Editing");
    static char sceneFileName[256] = "scene.json";
    ImGui::InputText("Scene File", sceneFileName, IM_ARRAYSIZE(sceneFileName));

    if (ImGui::Button("Save Scene")) {
        if (scene->saveToFile(sceneFileName)) {
            //ImGui::TextColored(ImVec4(0,1,0,1), "Scene saved!");
            ShowToast("Scene saved!");
        } else {
            //ImGui::TextColored(ImVec4(1,0,0,1), "Failed to save scene!");
            ShowToast("Failed to save scene!");
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Load Scene")) {
        if (scene->loadFromFile(sceneFileName, screenWidth, screenHeight)) {
            ShowToast("Scene loaded!");
            camera = scene->getActiveCamera();
            //std::cout << "scene loaded name: " << sceneFileName << std::endl;

        } else {
            ShowToast("Failed to load scene!");
        }
    }

    if (sceneEditingMode) {
        ImGui::Text("Click a model to select it");
        if (selectedMesh) {
            static glm::vec3 editPos;
            static glm::vec3 editScale;
            static glm::vec3 editRot;
            static glm::vec3 editColor;
            static glm::vec3 editDirection;
            static bool editActive;
            static float editIntensity;
            static float editCutoff;
            static SceneObject* lastMesh = nullptr;

            // If a new mesh is selected, update edit values to match its properties
            if (selectedMesh != lastMesh) {
                if (selectedMesh->isMesh()) {
                    Mesh* selectedMeshCast = dynamic_cast<Mesh*>(selectedMesh);
                    if (selectedMeshCast) {
                        editPos = selectedMeshCast->getPosition();
                        editScale = selectedMeshCast->getSize();
                        editRot = selectedMeshCast->getRotation();
                        editColor = selectedMeshCast->getColor();
                        editActive = selectedMeshCast->isActive();
                    }
                } else if (selectedMesh->isLight()) {
                    PointLight* light = static_cast<PointLight*>(selectedMesh);
                    editPos = light->getPosition();
                    editColor = light->getColor();
                    editActive = light->isActive();
                    editIntensity = light->getIntensity();
                    if (selectedMesh->isSpotLight()) {
                        SpotLight* spotLight = static_cast<SpotLight*>(selectedMesh);
                        editDirection = spotLight->getDirection();
                        editCutoff = spotLight->getCutoff();
                    }
                } else if (selectedMesh->isCamera()) {
                    Camera* selectedCamera = static_cast<Camera*>(selectedMesh);
                    editPos = selectedCamera->getPosition();
                    editRot = selectedCamera->getRotation();
                    editActive = selectedCamera->isActive();
                }
                lastMesh = selectedMesh;
            }

            // --- Undo for Mesh ---
            if (selectedMesh->isMesh()) {
                ImGui::Text("Selected Mesh Properties:");
                Mesh* mesh = static_cast<Mesh*>(selectedMesh);
                json beforeState, afterState;
                int objId = mesh->getId() ? mesh->getId() : 0;
                // Position
                bool posChanged = ImGui::InputFloat3("Position", &editPos.x, "%.3f");
                if (posChanged && editPos != mesh->getPosition()) {
                    beforeState = serializeObject(mesh);
                    mesh->setPosition(editPos);
                    scene->getGizmo()->setPosition(editPos);
                    afterState = serializeObject(mesh);
                    undoManager.pushAction({UndoManager::Action::Transform, beforeState, afterState, objId});
                }
                // Scale
                bool scaleChanged = ImGui::InputFloat3("Scale", &editScale.x, "%.3f");
                if (scaleChanged && editScale != mesh->getSize()) {
                    beforeState = serializeObject(mesh);
                    mesh->setSize(editScale);
                    afterState = serializeObject(mesh);
                    undoManager.pushAction({UndoManager::Action::Transform, beforeState, afterState, objId});
                }
                // Rotation
                bool rotChanged = ImGui::InputFloat3("Rotation (deg)", &editRot.x, "%.3f");
                if (rotChanged && editRot != mesh->getRotation()) {
                    beforeState = serializeObject(mesh);
                    mesh->setRotation(editRot);
                    afterState = serializeObject(mesh);
                    undoManager.pushAction({UndoManager::Action::Transform, beforeState, afterState, objId});
                }
                // Color
                bool colorChanged = ImGui::ColorEdit3("Color", &editColor.x);
                if (colorChanged && editColor != mesh->getColor()) {
                    beforeState = serializeObject(mesh);
                    mesh->setColor(editColor);
                    afterState = serializeObject(mesh);
                    undoManager.pushAction({UndoManager::Action::Transform, beforeState, afterState, objId});
                }
                // Active
                bool activeChanged = ImGui::Checkbox("Active", &editActive);
                if (activeChanged && editActive != mesh->isActive()) {
                    beforeState = serializeObject(mesh);
                    mesh->setActive(editActive);
                    afterState = serializeObject(mesh);
                    undoManager.pushAction({UndoManager::Action::Transform, beforeState, afterState, objId});
                }
            // --- Undo for PointLight ---
            } else if (selectedMesh->isLight()) {
                ImGui::Text(selectedMesh->isSpotLight() ? "Selected Spotlight Properties:" : "Selected Light Properties:");
                PointLight* light = static_cast<PointLight*>(selectedMesh);
                json beforeState, afterState;
                int objId = light->getId() ? light->getId() : 0;
                bool posChanged = ImGui::InputFloat3("Position", &editPos.x, "%.3f");
                if (posChanged && editPos != light->getPosition()) {
                    beforeState = serializeObject(light);
                    light->setPosition(editPos);
                    scene->getGizmo()->setPosition(editPos);
                    afterState = serializeObject(light);
                    undoManager.pushAction({UndoManager::Action::Transform, beforeState, afterState, objId});
                }
                bool colorChanged = ImGui::ColorEdit3("Color", &editColor.x);
                if (colorChanged && editColor != light->getColor()) {
                    beforeState = serializeObject(light);
                    light->setColor(editColor);
                    afterState = serializeObject(light);
                    undoManager.pushAction({UndoManager::Action::Transform, beforeState, afterState, objId});
                }
                bool intensityChanged = ImGui::InputFloat("Intensity", &editIntensity, 0.01f, 0.1f, "%.2f");
                if (intensityChanged && editIntensity != light->getIntensity()) {
                    beforeState = serializeObject(light);
                    light->setIntensity(editIntensity);
                    afterState = serializeObject(light);
                    undoManager.pushAction({UndoManager::Action::Transform, beforeState, afterState, objId});
                }
                bool activeChanged = ImGui::Checkbox("Active", &editActive);
                if (activeChanged && editActive != light->isActive()) {
                    beforeState = serializeObject(light);
                    light->setActive(editActive);
                    afterState = serializeObject(light);
                    undoManager.pushAction({UndoManager::Action::Transform, beforeState, afterState, objId});
                }
                if (selectedMesh->isSpotLight()) {
                    SpotLight* spotLight = static_cast<SpotLight*>(selectedMesh);
                    bool dirChanged = ImGui::InputFloat3("Direction", &editDirection.x, "%.3f");
                    if (dirChanged && editDirection != spotLight->getDirection()) {
                        beforeState = serializeObject(spotLight);
                        spotLight->setDirection(editDirection);
                        editDirection = spotLight->getDirection();
                        afterState = serializeObject(spotLight);
                        undoManager.pushAction({UndoManager::Action::Transform, beforeState, afterState, objId});
                    }

                    bool cutoffChanged = ImGui::InputFloat("Cutoff (deg)", &editCutoff, 0.5f, 2.0f, "%.2f");
                    if (cutoffChanged && editCutoff != spotLight->getCutoff()) {
                        beforeState = serializeObject(spotLight);
                        spotLight->setCutoff(editCutoff);
                        afterState = serializeObject(spotLight);
                        undoManager.pushAction({UndoManager::Action::Transform, beforeState, afterState, objId});
                    }
                }
            } else if (selectedMesh->isCamera()) {
                ImGui::Text("Selected Camera Properties:");
                Camera* selectedCamera = static_cast<Camera*>(selectedMesh);
                json beforeState, afterState;
                int objId = selectedCamera->getId() ? selectedCamera->getId() : 0;

                if (scene->getActiveCamera() == selectedCamera) {
                    ImGui::Text("Scene Active Camera");
                } else if (ImGui::Button("Make Active Camera")) {
                    scene->setActiveCamera(selectedCamera);
                    camera = selectedCamera;
                }

                bool posChanged = ImGui::InputFloat3("Position", &editPos.x, "%.3f");
                if (posChanged && editPos != selectedCamera->getPosition()) {
                    beforeState = serializeObject(selectedCamera);
                    selectedCamera->setPosition(editPos);
                    scene->getGizmo()->setPosition(editPos);
                    afterState = serializeObject(selectedCamera);
                    undoManager.pushAction({UndoManager::Action::Transform, beforeState, afterState, objId});
                }

                bool rotChanged = ImGui::InputFloat3("Rotation (rad)", &editRot.x, "%.3f");
                if (rotChanged && editRot != selectedCamera->getRotation()) {
                    beforeState = serializeObject(selectedCamera);
                    selectedCamera->setRotation(editRot);
                    afterState = serializeObject(selectedCamera);
                    undoManager.pushAction({UndoManager::Action::Transform, beforeState, afterState, objId});
                }

                bool activeChanged = ImGui::Checkbox("Active", &editActive);
                if (activeChanged && editActive != selectedCamera->isActive()) {
                    beforeState = serializeObject(selectedCamera);
                    selectedCamera->setActive(editActive);
                    afterState = serializeObject(selectedCamera);
                    undoManager.pushAction({UndoManager::Action::Transform, beforeState, afterState, objId});
                }
            }

            // --- Undo for Delete ---
            if (ImGui::Button("Delete Object")) {
                if (selectedMesh) {
                    json beforeState = serializeObject(selectedMesh);
                    int objId = selectedMesh->getId() ? selectedMesh->getId() : 0;

                    scene->remove(selectedMesh);
                    undoManager.pushAction({UndoManager::Action::Delete, beforeState, "", objId});
                    selectedMesh = nullptr;
                    lastMesh = nullptr;


                }
            }
            // --- Undo for Duplicate ---
            if (ImGui::Button("Duplicate Object") && scene->getGizmo()) {
                SceneObject* objToDuplicate = selectedMesh;
                if (objToDuplicate->isMesh()) {
                    Mesh* mesh = static_cast<Mesh*>(objToDuplicate);
                    Mesh* newMesh = new Mesh(
                        mesh->objPath,
                        mesh->material,
                        mesh->getPosition() + glm::vec3(1,0,0), // Offset position
                        mesh->getSize(),
                        mesh->getColor(),
                        mesh->getRotation(),
                        mesh->isActive()
                    );
                    newMesh->setColor(mesh->getColor());
                    scene->add(newMesh);
                    if (objToDuplicate->getParent()) {
                        newMesh->setParent(objToDuplicate->getParent(), true);
                    }
                    // Push create action
                    undoManager.pushAction({UndoManager::Action::Duplicate, "", serializeObject(newMesh), newMesh->getId() ? newMesh->getId() : 0});
                } else if (objToDuplicate->isLight()) {
                    PointLight* light = static_cast<PointLight*>(objToDuplicate);
                    LightObject* newLight = nullptr;
                    if (objToDuplicate->isSpotLight()) {
                        SpotLight* spotLight = static_cast<SpotLight*>(objToDuplicate);
                        newLight = new SpotLight(
                            spotLight->getPosition() + glm::vec3(1,0,0),
                            spotLight->getDirection(),
                            spotLight->getColor(),
                            spotLight->getIntensity(),
                            spotLight->getCutoff(),
                            spotLight->isActive()
                        );
                    } else {
                        newLight = new PointLight(
                            light->getPosition() + glm::vec3(1,0,0),
                            light->getColor(),
                            light->getIntensity(),
                            light->isActive()
                        );
                    }
                    scene->add(newLight);
                    if (objToDuplicate->getParent()) {
                        newLight->setParent(objToDuplicate->getParent(), true);
                    }
                    undoManager.pushAction({UndoManager::Action::Duplicate, "", serializeObject(newLight), newLight->getId() ? newLight->getId() : 0});
                } else if (objToDuplicate->isCamera()) {
                    Camera* sourceCamera = static_cast<Camera*>(objToDuplicate);
                    Camera* newCamera = new Camera(
                        screenWidth,
                        screenHeight,
                        sourceCamera->getPosition() + glm::vec3(1,0,0),
                        sourceCamera->getRotation()
                    );
                    newCamera->setActive(sourceCamera->isActive());
                    scene->add(newCamera);
                    if (objToDuplicate->getParent()) {
                        newCamera->setParent(objToDuplicate->getParent(), true);
                    }
                    undoManager.pushAction({UndoManager::Action::Duplicate, "", serializeObject(newCamera), newCamera->getId() ? newCamera->getId() : 0});
                }
            }
        } else {
            ImGui::Text("No model selected.");
        }
    }
    ImGui::End();
}

// --- Scene View Window ---

// Helper: recursively display a SceneObject and its children as a tree
static void ShowSceneObjectTree(SceneObject* obj, SceneObject*& selectedMesh, Scene* scene, int& nodeIdx) {
    glm::vec3 pos = obj->getPosition();
    std::string displayName;
    std::string tag = obj->getTag();
    if (!tag.empty()) {
        displayName = tag;
    } else {
        displayName = obj->isMesh() ? "Mesh" :
                     obj->isCamera() ? "Camera" :
                     obj->isSpotLight() ? "SpotLight" :
                     obj->isLight() ? "Light" :
                     obj->isCube() ? "Cube" :
                     obj->isPlane() ? "Plane" : "Object";
    }
    char label[128];
    snprintf(label, sizeof(label), "[%s] (%.2f, %.2f, %.2f)##node%d", displayName.c_str(), pos.x, pos.y, pos.z, nodeIdx++);

    bool isSelected = (obj == selectedMesh);
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
    if (isSelected) flags |= ImGuiTreeNodeFlags_Selected;

    bool isParent = !obj->getChildren().empty();
    bool open = false;
    if (isParent) {
        open = ImGui::TreeNodeEx(label, flags);
    } else {
        ImGui::TreeNodeEx(label, flags | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
    }

    // Begin drag source
    if (ImGui::BeginDragDropSource()) {
        ImGui::SetDragDropPayload("SCENE_OBJECT_PTR", &obj, sizeof(SceneObject*));
        ImGui::Text("Move %s", label);
        ImGui::EndDragDropSource();
    }

    // Accept drop target
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCENE_OBJECT_PTR")) {
            SceneObject* droppedObj = *(SceneObject**)payload->Data;
            if (droppedObj != obj) {
                obj->addChild(droppedObj, true);
            }
        }
        ImGui::EndDragDropTarget();
    }

    if (ImGui::IsItemClicked()) {
        selectedMesh = obj;
        scene->getGizmo()->setPosition(selectedMesh->getPosition());
    }
    if (isParent && open) {
        const auto& children = obj->getChildren();
        for (auto* child : children) {
            ShowSceneObjectTree(child, selectedMesh, scene, nodeIdx);
        }
        ImGui::TreePop();
    }
}

static void ShowSceneViewWindow(Scene* scene, SceneObject*& selectedMesh, bool sceneEditingMode) {
    if (!sceneEditingMode) return;
    ImGui::Begin("Scene View");
    ImGui::Text("Objects in Scene (Tree):");
    int nodeIdx = 0;
    for (auto mesh : scene->getMeshes()) {
        if (mesh->isChild()) continue;
        ShowSceneObjectTree(mesh, selectedMesh, scene, nodeIdx);
    }
    for (auto light : scene->getLights()) {
        if (light->isChild()) continue;
        ShowSceneObjectTree(light, selectedMesh, scene, nodeIdx);
    }
    for (auto camera : scene->getCameras()) {
        if (camera->isChild()) continue;
        ShowSceneObjectTree(camera, selectedMesh, scene, nodeIdx);
    }
    for (auto cube : scene->getCubes()) {
        if (cube->isChild()) continue;
        ShowSceneObjectTree(cube, selectedMesh, scene, nodeIdx);
    }
    for (auto plane : scene->getPlanes()) {
        if (plane->isChild()) continue;
        ShowSceneObjectTree(plane, selectedMesh, scene, nodeIdx);
    }
    ImGui::End();
}

// Helper to split path into folders
static std::vector<std::string> SplitPath(const std::string& path) {
    std::vector<std::string> parts;
    std::stringstream ss(path);
    std::string item;
    while (std::getline(ss, item, '/')) {
        if (!item.empty() && item != "..") parts.push_back(item);
    }
    return parts;
}

static void ShowAssetListWindow(Scene* scene) {
    static int selectedAssetIdx = -1;
    static std::string currentFolder = "assets"; // Track current folder path

    ImGui::Begin("Asset List");
    ImGui::Text("Loaded OBJ Models:");

    auto objPaths = OBJManager::getLoadedOBJPaths();

    // Build folder structure
    struct Node {
        std::map<std::string, Node> folders;
        std::vector<int> files; // indices in objPaths
        std::string path; // full path for navigation
        Node* parent = nullptr;
    };
    Node root;
    root.path = "assets";

    // Build tree
    std::map<std::string, Node*> pathToNode;
    pathToNode[root.path] = &root;
    for (int i = 0; i < objPaths.size(); ++i) {
        auto parts = SplitPath(objPaths[i]);
        if (parts.empty()) continue;
        Node* node = &root;
        std::string currentPath = "assets";
        for (size_t j = 1; j < parts.size() - 1; ++j) {
            currentPath += "/" + parts[j];
            if (node->folders.find(parts[j]) == node->folders.end()) {
                node->folders[parts[j]].parent = node;
                node->folders[parts[j]].path = currentPath;
            }
            node = &node->folders[parts[j]];
            pathToNode[currentPath] = node;
        }
        node->files.push_back(i);
    }

    // Find current node
    Node* currentNode = pathToNode.count(currentFolder) ? pathToNode[currentFolder] : &root;

    float boxWidth = 180.0f;
    float boxHeight = 60.0f;
    float spacing = 12.0f;
    int boxesPerRow = std::max(1, (int)(ImGui::GetContentRegionAvail().x / (boxWidth + spacing)));
    int itemIdx = 0;

    // Show ../ box if not in root
    if (currentNode != &root) {
        if (itemIdx % boxesPerRow != 0) ImGui::SameLine();
        ImGui::PushID("upfolder");
        ImGui::BeginChild("UpFolderBox", ImVec2(boxWidth, boxHeight), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse );
        if (ImGui::Selectable("../", false, 0, ImVec2(boxWidth - 10, 30))) {
            currentFolder = currentNode->parent ? currentNode->parent->path : "assets";
        }
        ImGui::Text("Go up");
        ImGui::EndChild();
        ImGui::PopID();
        itemIdx++;
    }

    // Show folders in current node
    for (auto& kv : currentNode->folders) {
        if (itemIdx % boxesPerRow != 0) ImGui::SameLine();
        ImGui::PushID(kv.second.path.c_str());
        ImGui::BeginChild("FolderBox", ImVec2(boxWidth, boxHeight), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        std::string label = kv.first + "/";
        if (ImGui::Selectable(label.c_str(), false, 0, ImVec2(boxWidth - 10, 30))) {
            currentFolder = kv.second.path;
        }
        ImGui::Text("Folder");
        ImGui::EndChild();
        ImGui::PopID();
        itemIdx++;
    }

    // Show files in current node
    for (int idx = 0; idx < currentNode->files.size(); ++idx) {
        int i = currentNode->files[idx];
        if (itemIdx % boxesPerRow != 0) ImGui::SameLine();
        ImGui::PushID(i);
        ImGui::BeginChild("AssetBox", ImVec2(boxWidth, boxHeight), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        bool isSelected = (selectedAssetIdx == i);
        std::string fileName = SplitPath(objPaths[i]).back();
        if (ImGui::Selectable(fileName.c_str(), isSelected, 0, ImVec2(boxWidth - 10, 30))) {
            selectedAssetIdx = i;
        }
        ImGui::Text("Index: %d", i);
        ImGui::EndChild();
        ImGui::PopID();
        itemIdx++;
    }

    if (ImGui::Button("Create New") && selectedAssetIdx >= 0 && selectedAssetIdx < objPaths.size()) {
        std::string objPath = objPaths[selectedAssetIdx];
        OBJData objData = OBJManager::getOBJ(objPath);

        std::string texPath = objData.texturePath.empty()
            ? (objPath.substr(0, objPath.find_last_of('.')) + ".png")
            : objData.texturePath;
        std::string roughPath = objData.roughPath;
        std::string metalPath = objData.mtlPath;
        std::string normalPath = objData.normalPath;

        Mesh* newMesh = new Mesh(objPath, Material(texPath, roughPath, metalPath, normalPath), glm::vec3(1,1,1), glm::vec3(1,1,1));

        if (newMesh)
            scene->add(newMesh);

        // --- Undo for Create New ---
        json afterState = serializeObject(newMesh);
        int objId = newMesh->getId();
        undoManager.pushAction({UndoManager::Action::Create, json(), afterState, objId});
    }
    ImGui::End();
}
