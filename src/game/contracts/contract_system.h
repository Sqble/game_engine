#pragma once

#include <array>
#include <cstddef>
#include <string>

struct CargoContract {
    std::string contractId;
    std::string pickupName;
    std::string destinationName;
    std::string cargoLabel;
    int cargoCount = 0;
    int rewardCredits = 0;
    std::string statusText;
};

class ContractSystem {
public:
    ContractSystem();

    bool hasActiveCargoContract() const;
    const CargoContract* getActiveCargoContract() const;

    bool requestCargoContract();
    bool abandonActiveCargoContract();

private:
    std::array<CargoContract, 3> cargoTemplates_;
    CargoContract activeCargoContract_;
    std::size_t nextCargoTemplateIndex_ = 0;
    bool hasActiveCargoContract_ = false;
};
