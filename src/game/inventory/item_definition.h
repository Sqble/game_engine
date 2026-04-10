#pragma once

#include <string>

struct ItemDefinition {
    std::string id;
    std::string name;
    std::string iconLabel;
    int maxStack = 1;
};

const ItemDefinition* GetItemDefinitionById(const std::string& itemId);
