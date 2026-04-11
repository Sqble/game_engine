#pragma once
#include "../game/contracts/contract_system.h"
#include "../game/inventory/inventory_container.h"
#include "../scene/scene.h"

#include <string>

enum class LockerUIActionType {
    None,
    Close,
    MovePlayerSlotToLocker,
    MoveLockerSlotToPlayer,
};

struct LockerUIAction {
    LockerUIActionType type = LockerUIActionType::None;
    int slotIndex = -1;
};

enum class TerminalUIActionType {
    None,
    Close,
    RequestCargoContract,
    AbandonCargoContract,
};

struct TerminalUIAction {
    TerminalUIActionType type = TerminalUIActionType::None;
};

void ShowEngineUI(Scene* scene, int screenWidth, int screenHeight, SceneObject*& selectedMesh, bool& sceneEditingMode, Camera*& camera);


static void ShowSceneEditingWindow(Scene* scene, int screenWidth, int screenHeight, SceneObject*& selectedMesh, bool sceneEditingMode, Camera*& camera);
static void ShowSceneViewWindow(Scene* scene, SceneObject*& selectedMesh, bool sceneEditingMode);
static void ShowAssetListWindow(Scene* scene);
void RenderToast(int screenWidth);
void RenderInteractionPrompt(int screenWidth, int screenHeight, const std::string& prompt);
LockerUIAction RenderInventoryBar(int screenWidth,
                                  int screenHeight,
                                  const InventoryContainer& inventory,
                                  int selectedSlot,
                                  bool allowTransfersToLocker);
LockerUIAction RenderLockerUI(int screenWidth,
                              int screenHeight,
                              const InventoryContainer& lockerInventory);
TerminalUIAction RenderTerminalUI(int screenWidth,
                                  int screenHeight,
                                  float submarineHealth,
                                  const CargoContract* activeContract);
void ShowToast(const std::string& message, float durationSeconds = 4.0f);
