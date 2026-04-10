#include "inventory_system.h"

InventorySystem::InventorySystem(size_t playerSlotCount, size_t lockerSlotCount)
    : lockerSlotCount_(lockerSlotCount),
      playerInventory_(playerSlotCount) {
}

void InventorySystem::initializeLockers(const std::vector<int>& lockerObjectIds) {
    for (int lockerObjectId : lockerObjectIds) {
        ensureLockerInventory(lockerObjectId);
    }
}

bool InventorySystem::seedLockerWithItem(int lockerObjectId, const std::string& itemId, int count) {
    return ensureLockerInventory(lockerObjectId).addItem(itemId, count);
}

bool InventorySystem::openLocker(int lockerObjectId) {
    ensureLockerInventory(lockerObjectId);
    openLockerId_ = lockerObjectId;
    return true;
}

void InventorySystem::closeLocker() {
    openLockerId_ = -1;
}

bool InventorySystem::isLockerOpen() const {
    return openLockerId_ != -1;
}

int InventorySystem::getOpenLockerId() const {
    return openLockerId_;
}

InventoryContainer& InventorySystem::getPlayerInventory() {
    return playerInventory_;
}

const InventoryContainer& InventorySystem::getPlayerInventory() const {
    return playerInventory_;
}

InventoryContainer& InventorySystem::ensureLockerInventory(int lockerObjectId) {
    auto it = lockerInventories_.find(lockerObjectId);
    if (it == lockerInventories_.end()) {
        it = lockerInventories_.emplace(lockerObjectId, InventoryContainer(lockerSlotCount_)).first;
    }

    return it->second;
}

const InventoryContainer* InventorySystem::getLockerInventory(int lockerObjectId) const {
    auto it = lockerInventories_.find(lockerObjectId);
    if (it == lockerInventories_.end()) {
        return nullptr;
    }

    return &it->second;
}

InventoryContainer* InventorySystem::getOpenLockerInventory() {
    if (!isLockerOpen()) {
        return nullptr;
    }

    return &ensureLockerInventory(openLockerId_);
}

const InventoryContainer* InventorySystem::getOpenLockerInventory() const {
    if (!isLockerOpen()) {
        return nullptr;
    }

    return getLockerInventory(openLockerId_);
}

bool InventorySystem::movePlayerSlotToOpenLocker(int slotIndex) {
    InventoryContainer* openLocker = getOpenLockerInventory();
    if (!openLocker) {
        return false;
    }

    return playerInventory_.moveSlotTo(slotIndex, *openLocker);
}

bool InventorySystem::moveOpenLockerSlotToPlayer(int slotIndex) {
    InventoryContainer* openLocker = getOpenLockerInventory();
    if (!openLocker) {
        return false;
    }

    return openLocker->moveSlotTo(slotIndex, playerInventory_);
}

void InventorySystem::selectPlayerSlot(int slotIndex) {
    if (playerInventory_.isValidSlot(slotIndex)) {
        selectedPlayerSlot_ = slotIndex;
    }
}

int InventorySystem::getSelectedPlayerSlot() const {
    return selectedPlayerSlot_;
}
