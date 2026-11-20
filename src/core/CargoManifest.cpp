#include "CargoManifest.h"
#include <cmath>
#include <limits>

namespace oem {

CargoLine::CargoLine() : cargoId(), quantity(0), unitMassKg(0.0), originLocationId() {}
CargoTransfer::CargoTransfer()
    : transferId(), direction(CARGO_LOAD), vesselId(), locationId(), cargoId(),
      quantity(0), unitMassKg(0.0), simulationTime(0) {}

CargoManifest::CargoManifest(const std::string& vesselId)
    : vesselId_(vesselId), lineIndexes_(), transferIndexes_(), lines_(), history_() {}

Result CargoManifest::load(const CargoTransfer& transfer) {
    return apply(transfer, CARGO_LOAD);
}

Result CargoManifest::unload(const CargoTransfer& transfer) {
    return apply(transfer, CARGO_UNLOAD);
}

Result CargoManifest::apply(const CargoTransfer& transfer, CargoTransferDirection expected) {
    if (transfer.transferId.empty() || transfer.cargoId.empty() ||
        transfer.locationId.empty() || vesselId_.empty() ||
        transfer.vesselId != vesselId_)
        return Result::failure("INVALID_CARGO_TRANSFER", "Transfer, cargo, location and matching vessel IDs are required.");
    if (transfer.direction != expected)
        return Result::failure("TRANSFER_DIRECTION", "Cargo transfer direction does not match the requested operation.");
    if (transfer.quantity <= 0 || !(transfer.unitMassKg > 0.0) ||
        !_finite(transfer.unitMassKg))
        return Result::failure("INVALID_CARGO_AMOUNT", "Quantity and unit mass must be positive and finite.");

    std::map<std::string, std::size_t>::const_iterator prior =
        transferIndexes_.find(transfer.transferId);
    if (prior != transferIndexes_.end()) {
        const CargoTransfer& old = history_[prior->second];
        if (old.direction == transfer.direction && old.vesselId == transfer.vesselId &&
            old.locationId == transfer.locationId && old.cargoId == transfer.cargoId &&
            old.quantity == transfer.quantity && old.unitMassKg == transfer.unitMassKg &&
            old.simulationTime == transfer.simulationTime)
            return Result::success();
        return Result::failure("CARGO_TRANSFER_CONFLICT", "Transfer ID was already used for different cargo facts.");
    }

    std::map<std::string, std::size_t>::iterator lineIt = lineIndexes_.find(transfer.cargoId);
    if (expected == CARGO_UNLOAD) {
        if (lineIt == lineIndexes_.end() ||
            lines_[lineIt->second].quantity < transfer.quantity)
            return Result::failure("INSUFFICIENT_CARGO", "The vessel manifest does not contain the requested quantity.");
        if (std::fabs(lines_[lineIt->second].unitMassKg - transfer.unitMassKg) > 1e-9)
            return Result::failure("CARGO_MASS_MISMATCH", "Unit mass differs from the cargo already in the manifest.");
    } else if (lineIt != lineIndexes_.end()) {
        if (std::fabs(lines_[lineIt->second].unitMassKg - transfer.unitMassKg) > 1e-9)
            return Result::failure("CARGO_MASS_MISMATCH", "Unit mass differs from the cargo already in the manifest.");
        if (lines_[lineIt->second].quantity >
            std::numeric_limits<long long>::max() - transfer.quantity)
            return Result::failure("CARGO_QUANTITY_OVERFLOW", "Cargo quantity would overflow.");
    }

    if (expected == CARGO_LOAD) {
        if (lineIt == lineIndexes_.end()) {
            CargoLine line;
            line.cargoId = transfer.cargoId;
            line.quantity = transfer.quantity;
            line.unitMassKg = transfer.unitMassKg;
            line.originLocationId = transfer.locationId;
            lines_.push_back(line);
            lineIndexes_[transfer.cargoId] = lines_.size() - 1;
        } else {
            lines_[lineIt->second].quantity += transfer.quantity;
        }
    } else {
        lines_[lineIt->second].quantity -= transfer.quantity;
        if (lines_[lineIt->second].quantity == 0) {
            lines_.erase(lines_.begin() + lineIt->second);
            lineIndexes_.clear();
            for (std::size_t i = 0; i < lines_.size(); ++i)
                lineIndexes_[lines_[i].cargoId] = i;
        }
    }

    history_.push_back(transfer);
    transferIndexes_[transfer.transferId] = history_.size() - 1;
    return Result::success();
}

const std::string& CargoManifest::vesselId() const { return vesselId_; }
const std::vector<CargoLine>& CargoManifest::lines() const { return lines_; }
const std::vector<CargoTransfer>& CargoManifest::history() const { return history_; }

long long CargoManifest::quantity(const std::string& cargoId) const {
    std::map<std::string, std::size_t>::const_iterator it = lineIndexes_.find(cargoId);
    return it == lineIndexes_.end() ? 0 : lines_[it->second].quantity;
}

double CargoManifest::totalMassKg() const {
    double total = 0.0;
    for (std::size_t i = 0; i < lines_.size(); ++i)
        total += static_cast<double>(lines_[i].quantity) * lines_[i].unitMassKg;
    return total;
}

bool CargoManifest::hasTransfer(const std::string& transferId) const {
    return transferIndexes_.find(transferId) != transferIndexes_.end();
}

} // namespace oem
