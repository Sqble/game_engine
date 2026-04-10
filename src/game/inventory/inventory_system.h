#pragma once

#include "inventory_container.h"

#include <unordered_map>
#include <vector>

class InventorySystem {
public:
    InventorySystem(size_t playerSlotCount = 8, size_t lockerSlotCount = 6);

    void initializeLockers(const std::vector<int>& lockerObjectIds);
    bool seedLockerWithItem(int lockerObjectId, const std::string& itemId, int count = 1);

    bool openLocker(int lockerObjectId);
    void closeLocker();
    bool isLockerOpen() const;
    int getOpenLockerId() const;

    InventoryContainer& getPlayerInventory();
    const InventoryContainer& getPlayerInventory() const;

    InventoryContainer& ensureLockerInventory(int lockerObjectId);
    const InventoryContainer* getLockerInventory(int lockerObjectId) const;
    InventoryContainer* getOpenLockerInventory();
    const InventoryContainer* getOpenLockerInventory() const;

    bool movePlayerSlotToOpenLocker(int slotIndex);
    bool moveOpenLockerSlotToPlayer(int slotIndex);

    void selectPlayerSlot(int slotIndex);
    int getSelectedPlayerSlot() const;

private:
    size_t lockerSlotCount_;
    InventoryContainer playerInventory_;
    std::unordered_map<int, InventoryContainer> lockerInventories_;
    int openLockerId_ = -1;
    int selectedPlayerSlot_ = 0;
};
