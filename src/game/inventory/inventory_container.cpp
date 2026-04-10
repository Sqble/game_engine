#include "inventory_container.h"

#include "item_definition.h"

#include <algorithm>

bool InventorySlot::isEmpty() const {
    return itemId.empty() || count <= 0;
}

void InventorySlot::clear() {
    itemId.clear();
    count = 0;
}

InventoryContainer::InventoryContainer(size_t slotCount)
    : slots_(slotCount) {
}

size_t InventoryContainer::getSlotCount() const {
    return slots_.size();
}

bool InventoryContainer::isValidSlot(int slotIndex) const {
    return slotIndex >= 0 && static_cast<size_t>(slotIndex) < slots_.size();
}

const std::vector<InventorySlot>& InventoryContainer::getSlots() const {
    return slots_;
}

std::vector<InventorySlot>& InventoryContainer::getSlots() {
    return slots_;
}

bool InventoryContainer::setSlot(int slotIndex, const InventorySlot& slot) {
    if (!isValidSlot(slotIndex)) {
        return false;
    }

    slots_[slotIndex] = slot;
    return true;
}

bool InventoryContainer::addItem(const std::string& itemId, int count) {
    if (itemId.empty() || count <= 0) {
        return false;
    }

    const ItemDefinition* definition = GetItemDefinitionById(itemId);
    const int maxStack = definition ? definition->maxStack : 1;

    std::vector<InventorySlot> updatedSlots = slots_;
    int remainingCount = count;

    while (remainingCount > 0) {
        int slotIndex = -1;
        for (size_t i = 0; i < updatedSlots.size(); ++i) {
            const InventorySlot& slot = updatedSlots[i];
            if (slot.itemId == itemId && slot.count < maxStack) {
                slotIndex = static_cast<int>(i);
                break;
            }
        }

        if (slotIndex == -1) {
            for (size_t i = 0; i < updatedSlots.size(); ++i) {
                if (updatedSlots[i].isEmpty()) {
                    slotIndex = static_cast<int>(i);
                    break;
                }
            }
        }

        if (slotIndex == -1) {
            return false;
        }

        InventorySlot& slot = updatedSlots[slotIndex];
        if (slot.isEmpty()) {
            slot.itemId = itemId;
        }

        const int spaceLeft = maxStack - slot.count;
        const int addedCount = std::min(spaceLeft, remainingCount);
        slot.count += addedCount;
        remainingCount -= addedCount;
    }

    slots_ = std::move(updatedSlots);
    return true;
}

bool InventoryContainer::moveSlotTo(int slotIndex, InventoryContainer& target) {
    if (!isValidSlot(slotIndex) || slots_[slotIndex].isEmpty()) {
        return false;
    }

    const InventorySlot movedSlot = slots_[slotIndex];
    if (!target.addItem(movedSlot.itemId, movedSlot.count)) {
        return false;
    }

    slots_[slotIndex].clear();
    return true;
}
