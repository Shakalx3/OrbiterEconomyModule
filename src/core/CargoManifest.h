#ifndef OEM_CORE_CARGO_MANIFEST_H
#define OEM_CORE_CARGO_MANIFEST_H

#include "Types.h"
#include <map>
#include <string>
#include <vector>

namespace oem {

struct CargoLine {
    std::string cargoId;
    long long quantity;
    double unitMassKg;
    std::string originLocationId;
    CargoLine();
};

enum CargoTransferDirection {
    CARGO_LOAD = 1,
    CARGO_UNLOAD = 2
};

struct CargoTransfer {
    std::string transferId;
    CargoTransferDirection direction;
    std::string vesselId;
    std::string locationId;
    std::string cargoId;
    long long quantity;
    double unitMassKg;
    SimulationSeconds simulationTime;
    CargoTransfer();
};

class CargoManifest {
public:
    explicit CargoManifest(const std::string& vesselId = std::string());

    Result load(const CargoTransfer& transfer);
    Result unload(const CargoTransfer& transfer);

    const std::string& vesselId() const;
    const std::vector<CargoLine>& lines() const;
    const std::vector<CargoTransfer>& history() const;
    long long quantity(const std::string& cargoId) const;
    double totalMassKg() const;
    bool hasTransfer(const std::string& transferId) const;

private:
    Result apply(const CargoTransfer& transfer, CargoTransferDirection expected);
    std::string vesselId_;
    std::map<std::string, std::size_t> lineIndexes_;
    std::map<std::string, std::size_t> transferIndexes_;
    std::vector<CargoLine> lines_;
    std::vector<CargoTransfer> history_;
};

} // namespace oem

#endif
