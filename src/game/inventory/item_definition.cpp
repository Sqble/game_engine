#include "item_definition.h"

const ItemDefinition* GetItemDefinitionById(const std::string& itemId) {
    static const ItemDefinition wrenchDefinition{"wrench", "Wrench", "WR", 1};

    if (itemId == wrenchDefinition.id) {
        return &wrenchDefinition;
    }

    return nullptr;
}
