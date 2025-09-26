#include "engine_ui.h"
#include "../objmanager/objmanager.h"
#include "../mesh/mesh.h"
#include "../pointlight/pointlight.h"

#include "../includes/imgui/imgui.h"
#include "../includes/imgui/imgui_impl_glfw.h"
#include "../includes/imgui/imgui_impl_opengl3.h"
#include "../includes/imgui/imgui_internal.h" 

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
                             ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_AlwaysAutoResize;

    ImGui::Begin("##ToastBar", nullptr, flags);
    ImGui::SetCursorPosY(10); // Center vertically
    ImGui::SetCursorPosX((screenWidth - ImGui::CalcTextSize(toastMessage.c_str()).x) * 0.5f); // Center horizontally
    ImGui::Text("%s", toastMessage.c_str());
    ImGui::End();
}

void ShowEngineUI(Scene* scene, int screenWidth, int screenHeight, SceneObject*& selectedMesh, bool& sceneEditingMode, Camera*& camera) {
    if (!sceneEditingMode) {
        return;
    }

    static bool dockInit = false;
    ImGuiWindowFlags window_flags =  ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar;
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2((float)screenWidth, (float)screenHeight), ImGuiCond_Always);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("EngineDockSpace", nullptr, window_flags);

    ImGuiID dockspace_id = ImGui::GetID("EngineDockSpace");
    if (!dockInit) {
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

    RenderToast(screenWidth);
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
            static bool editActive;
            static float editIntensity;
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
                } else if (selectedMesh->isPointLight()) {
                    PointLight* light = static_cast<PointLight*>(selectedMesh);
                    editPos = light->getPosition();
                    editColor = light->getColor();
                    editActive = light->isActive();
                    editIntensity = light->getIntensity();
                }
                lastMesh = selectedMesh;
            }

            if (selectedMesh->isMesh()) {
                ImGui::Text("Selected Mesh Properties:");
                bool posChanged = ImGui::InputFloat3("Position", &editPos.x, "%.3f");
                bool scaleChanged = ImGui::InputFloat3("Scale", &editScale.x, "%.3f");
                bool rotChanged = ImGui::InputFloat3("Rotation (deg)", &editRot.x, "%.3f");
                bool colorChanged = ImGui::ColorEdit3("Color", &editColor.x);
                bool activeChanged = ImGui::Checkbox("Active", &editActive);

                Mesh* mesh = static_cast<Mesh*>(selectedMesh);
                if (posChanged && editPos != mesh->getPosition()) {
                    mesh->setPosition(editPos);
                    scene->getGizmo()->setPosition(editPos);
                }
                if (scaleChanged && editScale != mesh->getSize()) {
                    mesh->setSize(editScale);
                }
                if (rotChanged && editRot != mesh->getRotation()) {
                    mesh->setRotation(editRot);
                }
                if (colorChanged && editColor != mesh->getColor()) {
                    mesh->setColor(editColor);
                }
                if (activeChanged && editActive != mesh->isActive()) {
                    mesh->setActive(editActive);
                }
            } else if (selectedMesh->isPointLight()) {
                ImGui::Text("Selected Light Properties:");
                bool posChanged = ImGui::InputFloat3("Position", &editPos.x, "%.3f");
                bool colorChanged = ImGui::ColorEdit3("Color", &editColor.x);
                bool intensityChanged = ImGui::InputFloat("Intensity", &editIntensity, 0.01f, 0.1f, "%.2f");
                bool activeChanged = ImGui::Checkbox("Active", &editActive);

                PointLight* light = static_cast<PointLight*>(selectedMesh);
                if (posChanged && editPos != light->getPosition()) {
                    light->setPosition(editPos);
                    scene->getGizmo()->setPosition(editPos);
                }
                if (colorChanged && editColor != light->getColor()) {
                    light->setColor(editColor);
                }
                if (intensityChanged && editIntensity != light->getIntensity()) { 
                    light->setIntensity(editIntensity);
                }
                if (activeChanged && editActive != light->isActive()) {
                    light->setActive(editActive);
                }
            }

            if (ImGui::Button("Delete Object")) {
                //scene->remove(selectedMesh);
                //delete selectedMesh;
                selectedMesh = nullptr;
                lastMesh = nullptr;
            }
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
                } else if (objToDuplicate->isPointLight()) {
                    PointLight* light = static_cast<PointLight*>(objToDuplicate);
                    PointLight* newLight = new PointLight(
                        light->getPosition() + glm::vec3(1,0,0), // Offset position
                        light->getColor(),
                        light->getIntensity(),
                        light->isActive()
                    );
                    scene->add(newLight);
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
        displayName = obj->isMesh() ? "Mesh" : obj->isPointLight() ? "Light" : obj->isParent() ? "Parent" : "Object";
    }
    char label[128];
    snprintf(label, sizeof(label), "[%s] (%.2f, %.2f, %.2f)##node%d", displayName.c_str(), pos.x, pos.y, pos.z, nodeIdx++);

    bool isSelected = (obj == selectedMesh);
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
    if (isSelected) flags |= ImGuiTreeNodeFlags_Selected;

    bool isParent = obj->isParent();
    bool open = false;
    if (isParent) {
        open = ImGui::TreeNodeEx(label, flags);
    } else {
        ImGui::TreeNodeEx(label, flags | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
    }
    if (ImGui::IsItemClicked()) {
        selectedMesh = obj;
        scene->getGizmo()->setPosition(selectedMesh->getPosition());
    }
    if (isParent && open) {
        // Recursively show children
        const auto* parentObj = obj;
        // Only call getChildren if isParent is true
        if (parentObj->isParent()) {
            const auto& children = static_cast<const ParentObject<SceneObject>*>(parentObj)->getChildren();
            for (auto* child : children) {
                ShowSceneObjectTree(child, selectedMesh, scene, nodeIdx);
            }
        }
        ImGui::TreePop();
    }
}

static void ShowSceneViewWindow(Scene* scene, SceneObject*& selectedMesh, bool sceneEditingMode) {
    if (!sceneEditingMode) return;
    ImGui::Begin("Scene View");
    ImGui::Text("Objects in Scene (Tree):");
    int nodeIdx = 0;
    // Show all top-level meshes
    for (auto mesh : scene->getMeshes()) {
        ShowSceneObjectTree(mesh, selectedMesh, scene, nodeIdx);
    }
    // Show all top-level lights
    for (auto light : scene->getPointLights()) {
        ShowSceneObjectTree(light, selectedMesh, scene, nodeIdx);
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
    }
    ImGui::End();
}