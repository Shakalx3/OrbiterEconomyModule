#ifndef OEM_CORE_CARGO_SERVICE_INVENTORY_H
#define OEM_CORE_CARGO_SERVICE_INVENTORY_H
#include "CargoManifest.h"
#include <map>
#include <string>
#include <vector>
namespace oem {
struct CargoStockLine {
    std::string cargoId;
    long long quantity;
    double unitMassKg;
    CargoStockLine();
};
class CargoServiceInventory {
public:
    explicit CargoServiceInventory(const std::string& serviceVesselId = std::string());
    Result setInitialStock(const std::string& cargoId, long long quantity, double unitMassKg);
    Result apply(const CargoTransfer& transfer);
    const std::string& serviceVesselId() const;
    const std::vector<CargoStockLine>& lines() const;
    const std::vector<CargoTransfer>& history() const;
    long long quantity(const std::string& cargoId) const;
    bool hasTransfer(const std::string& transferId) const;
private:
    std::string serviceVesselId_;
    std::map<std::string, std::size_t> lineIndexes_;
    std::map<std::string, std::size_t> transferIndexes_;
    std::vector<CargoStockLine> lines_;
    std::vector<CargoTransfer> history_;
};
}
#endif
