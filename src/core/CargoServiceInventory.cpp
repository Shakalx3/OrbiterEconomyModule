#include "CargoServiceInventory.h"
#include <cmath>
#include <limits>
namespace oem {
CargoStockLine::CargoStockLine() : cargoId(), quantity(0), unitMassKg(0.0) {}
CargoServiceInventory::CargoServiceInventory(const std::string& id)
    : serviceVesselId_(id), lineIndexes_(), transferIndexes_(), lines_(), history_() {}
Result CargoServiceInventory::setInitialStock(const std::string& cargoId, long long quantity, double unitMassKg) {
    if (serviceVesselId_.empty() || cargoId.empty() || quantity < 0 || !(unitMassKg > 0.0) || !std::isfinite(unitMassKg))
        return Result::failure("INVALID_CARGO_STOCK", "Service ID, cargo ID, non-negative quantity and positive finite unit mass are required.");
    if (!history_.empty())
        return Result::failure("CARGO_STOCK_ALREADY_ACTIVE", "Initial stock cannot be changed after the first transfer.");
    std::map<std::string, std::size_t>::iterator it = lineIndexes_.find(cargoId);
    if (it != lineIndexes_.end()) {
        if (std::fabs(lines_[it->second].unitMassKg - unitMassKg) > 1e-9)
            return Result::failure("CARGO_MASS_MISMATCH", "Configured unit mass differs from the existing stock line.");
        lines_[it->second].quantity = quantity;
        return Result::success();
    }
    if (quantity == 0) return Result::success();
    CargoStockLine line;
    line.cargoId = cargoId; line.quantity = quantity; line.unitMassKg = unitMassKg;
    lines_.push_back(line); lineIndexes_[cargoId] = lines_.size() - 1;
    return Result::success();
}
Result CargoServiceInventory::apply(const CargoTransfer& transfer) {
    if (serviceVesselId_.empty() || transfer.transferId.empty() || transfer.vesselId.empty() ||
        transfer.locationId != serviceVesselId_ || transfer.cargoId.empty())
        return Result::failure("INVALID_SERVICE_TRANSFER", "Transfer must identify this service, a vessel, cargo and transfer ID.");
    if (transfer.direction != CARGO_LOAD && transfer.direction != CARGO_UNLOAD)
        return Result::failure("TRANSFER_DIRECTION", "Unknown cargo transfer direction.");
    if (transfer.quantity <= 0 || !(transfer.unitMassKg > 0.0) || !std::isfinite(transfer.unitMassKg))
        return Result::failure("INVALID_CARGO_AMOUNT", "Quantity and unit mass must be positive and finite.");
    std::map<std::string, std::size_t>::const_iterator prior = transferIndexes_.find(transfer.transferId);
    if (prior != transferIndexes_.end()) {
        const CargoTransfer& old = history_[prior->second];
        if (old.direction == transfer.direction && old.vesselId == transfer.vesselId &&
            old.locationId == transfer.locationId && old.cargoId == transfer.cargoId &&
            old.quantity == transfer.quantity && old.unitMassKg == transfer.unitMassKg &&
            old.simulationTime == transfer.simulationTime) return Result::success();
        return Result::failure("CARGO_TRANSFER_CONFLICT", "Transfer ID was already used for different service inventory facts.");
    }
    std::map<std::string, std::size_t>::iterator it = lineIndexes_.find(transfer.cargoId);
    if (it != lineIndexes_.end() && std::fabs(lines_[it->second].unitMassKg - transfer.unitMassKg) > 1e-9)
        return Result::failure("CARGO_MASS_MISMATCH", "Transfer unit mass differs from the service inventory line.");
    if (transfer.direction == CARGO_LOAD) {
        if (it == lineIndexes_.end() || lines_[it->second].quantity < transfer.quantity)
            return Result::failure("INSUFFICIENT_STATION_STOCK", "The cargo service does not have enough stock for this load.");
        lines_[it->second].quantity -= transfer.quantity;
        if (lines_[it->second].quantity == 0) {
            lines_.erase(lines_.begin() + it->second); lineIndexes_.clear();
            for (std::size_t i = 0; i < lines_.size(); ++i) lineIndexes_[lines_[i].cargoId] = i;
        }
    } else {
        if (it == lineIndexes_.end()) {
            CargoStockLine line; line.cargoId = transfer.cargoId;
            line.quantity = transfer.quantity; line.unitMassKg = transfer.unitMassKg;
            lines_.push_back(line); lineIndexes_[transfer.cargoId] = lines_.size() - 1;
        } else {
            if (lines_[it->second].quantity > std::numeric_limits<long long>::max() - transfer.quantity)
                return Result::failure("CARGO_QUANTITY_OVERFLOW", "Service stock quantity would overflow.");
            lines_[it->second].quantity += transfer.quantity;
        }
    }
    history_.push_back(transfer); transferIndexes_[transfer.transferId] = history_.size() - 1;
    return Result::success();
}
const std::string& CargoServiceInventory::serviceVesselId() const { return serviceVesselId_; }
const std::vector<CargoStockLine>& CargoServiceInventory::lines() const { return lines_; }
const std::vector<CargoTransfer>& CargoServiceInventory::history() const { return history_; }
long long CargoServiceInventory::quantity(const std::string& id) const {
    std::map<std::string, std::size_t>::const_iterator it = lineIndexes_.find(id);
    return it == lineIndexes_.end() ? 0 : lines_[it->second].quantity;
}
bool CargoServiceInventory::hasTransfer(const std::string& id) const {
    return transferIndexes_.find(id) != transferIndexes_.end();
}
}
