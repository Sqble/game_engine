#include "item_definition.h"

const ItemDefinition* GetItemDefinitionById(const std::string& itemId) {
    static const ItemDefinition wrenchDefinition{"wrench", "Wrench", "WR", 1};
    static const ItemDefinition scrapDefinition{"scrap", "Scrap", "SC", 5};

    if (itemId == wrenchDefinition.id) {
        return &wrenchDefinition;
    }
    if (itemId == scrapDefinition.id) {
        return &scrapDefinition;
    }

    return nullptr;
}
