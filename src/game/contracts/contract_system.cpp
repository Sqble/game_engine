#include "contract_system.h"

ContractSystem::ContractSystem()
    : cargoTemplates_{
          CargoContract{"CT-101", "Home Dock", "Morrow Anchorage", "sealed repair crate", 2, 180, "Awaiting departure"},
          CargoContract{"CT-102", "Home Dock", "Brine Station", "medical supply case", 1, 140, "Awaiting departure"},
          CargoContract{"CT-103", "Home Dock", "Relay K-17", "reactor filter drum", 3, 240, "Awaiting departure"},
      } {
}

bool ContractSystem::hasActiveCargoContract() const {
    return hasActiveCargoContract_;
}

const CargoContract* ContractSystem::getActiveCargoContract() const {
    return hasActiveCargoContract_ ? &activeCargoContract_ : nullptr;
}

bool ContractSystem::requestCargoContract() {
    if (hasActiveCargoContract_) {
        return false;
    }

    activeCargoContract_ = cargoTemplates_[nextCargoTemplateIndex_];
    hasActiveCargoContract_ = true;
    nextCargoTemplateIndex_ = (nextCargoTemplateIndex_ + 1) % cargoTemplates_.size();
    return true;
}

bool ContractSystem::abandonActiveCargoContract() {
    if (!hasActiveCargoContract_) {
        return false;
    }

    hasActiveCargoContract_ = false;
    activeCargoContract_ = CargoContract{};
    return true;
}
