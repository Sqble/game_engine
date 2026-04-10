#pragma once

#include <cstddef>
#include <string>
#include <vector>

struct InventorySlot {
    std::string itemId;
    int count = 0;

    bool isEmpty() const;
    void clear();
};

class InventoryContainer {
public:
    explicit InventoryContainer(size_t slotCount = 0);

    size_t getSlotCount() const;
    bool isValidSlot(int slotIndex) const;

    const std::vector<InventorySlot>& getSlots() const;
    std::vector<InventorySlot>& getSlots();

    bool setSlot(int slotIndex, const InventorySlot& slot);
    bool addItem(const std::string& itemId, int count = 1);
    bool moveSlotTo(int slotIndex, InventoryContainer& target);

private:
    std::vector<InventorySlot> slots_;
};
