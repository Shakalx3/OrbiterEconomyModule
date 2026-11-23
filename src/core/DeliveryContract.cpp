#include "DeliveryContract.h"
#include <limits>

namespace oem {

DeliveryContract::DeliveryContract()
    : contractId(), vesselId(), cargoId(), quantity(0), originLocationId(),
      destinationLocationId(), destinationBodyId(), missionType(), missionTitle(), missionBriefing(), reward(0), acceptedAt(0), deadline(0),
      status(CONTRACT_ACCEPTED), completionEventId(), completedAt(0) {}

DeliveryContractBook::DeliveryContractBook() : indexes_(), contracts_() {}

Result DeliveryContractBook::accept(const DeliveryContract& c) {
    if (c.contractId.empty() || c.vesselId.empty() || c.cargoId.empty() ||
        c.originLocationId.empty() || c.destinationLocationId.empty())
        return Result::failure("CONTRACT_IDENTITY", "Contract, vessel, cargo, origin and destination IDs are required.");
    if (c.originLocationId == c.destinationLocationId)
        return Result::failure("CONTRACT_ROUTE", "Contract origin and destination must differ.");
    if (c.quantity <= 0 || c.reward <= 0)
        return Result::failure("CONTRACT_AMOUNT", "Contract quantity and reward must be positive.");
    if (c.acceptedAt < 0 || (c.deadline > 0 && c.deadline < c.acceptedAt))
        return Result::failure("CONTRACT_TIME", "Contract deadline cannot precede acceptance.");
    std::map<std::string, std::size_t>::const_iterator prior = indexes_.find(c.contractId);
    if (prior != indexes_.end()) {
        const DeliveryContract& existing = contracts_[prior->second];
        const bool same = existing.vesselId == c.vesselId &&
            existing.cargoId == c.cargoId && existing.quantity == c.quantity &&
            existing.originLocationId == c.originLocationId &&
            existing.destinationLocationId == c.destinationLocationId &&
            existing.destinationBodyId == c.destinationBodyId &&
            existing.missionType == c.missionType &&
            existing.missionTitle == c.missionTitle &&
            existing.missionBriefing == c.missionBriefing &&
            existing.reward == c.reward && existing.acceptedAt == c.acceptedAt &&
            existing.deadline == c.deadline;
        return same ? Result::success() :
            Result::failure("CONTRACT_CONFLICT", "Contract ID already belongs to different terms.");
    }

    DeliveryContract stored = c;
    stored.status = CONTRACT_ACCEPTED;
    stored.completionEventId.clear();
    stored.completedAt = 0;
    contracts_.push_back(stored);
    indexes_[stored.contractId] = contracts_.size() - 1;
    return Result::success();
}

long long DeliveryContractBook::eligibleCargoAt(const DeliveryContract& c,
                                                 const CargoManifest& manifest,
                                                 SimulationSeconds eventTime) const {
    if (manifest.vesselId() != c.vesselId) return 0;
    long long eligible = 0;
    const std::vector<CargoTransfer>& history = manifest.history();
    for (std::size_t i = 0; i < history.size(); ++i) {
        const CargoTransfer& t = history[i];
        if (t.simulationTime > eventTime || t.cargoId != c.cargoId) continue;
        if (t.direction == CARGO_LOAD && t.locationId == c.originLocationId &&
            t.simulationTime >= c.acceptedAt) {
            if (eligible > std::numeric_limits<long long>::max() - t.quantity)
                return std::numeric_limits<long long>::max();
            eligible += t.quantity;
        } else if (t.direction == CARGO_UNLOAD && eligible > 0) {
            eligible = t.quantity >= eligible ? 0 : eligible - t.quantity;
        }
    }
    return eligible;
}

bool DeliveryContractBook::isDestinationEvent(const DeliveryContract& c,
                                               const SimulationEvent& e) const {
    if (e.type == SIM_EVENT_LANDING)
        return e.locationKnown && !e.baseId.empty() &&
               e.baseId == c.destinationLocationId &&
               (c.destinationBodyId.empty() || e.bodyId == c.destinationBodyId);
    if (e.type == SIM_EVENT_DOCK)
        return !e.relatedVesselId.empty() &&
               e.relatedVesselId == c.destinationLocationId;
    return false;
}

Result DeliveryContractBook::observe(const SimulationEvent& e,
                                     const CargoManifest& manifest) {
    if (e.eventId.empty() || e.vesselId.empty())
        return Result::failure("CONTRACT_EVENT_IDENTITY", "Verified event ID and vessel ID are required.");
    bool completedAny = false;
    for (std::size_t i = 0; i < contracts_.size(); ++i) {
        DeliveryContract& c = contracts_[i];
        if (c.status != CONTRACT_ACCEPTED || c.vesselId != e.vesselId) continue;
        if (e.simulationTime < c.acceptedAt || !isDestinationEvent(c, e)) continue;
        if (c.deadline > 0 && e.simulationTime > c.deadline) {
            c.status = CONTRACT_EXPIRED;
            continue;
        }
        if (eligibleCargoAt(c, manifest, e.simulationTime) < c.quantity) continue;
        c.status = CONTRACT_COMPLETED;
        c.completionEventId = e.eventId;
        c.completedAt = e.simulationTime;
        completedAny = true;
    }
    return completedAny ? Result::success() :
        Result::failure("CONTRACT_NO_MATCH", "Event does not complete an accepted delivery contract.");
}

Result DeliveryContractBook::settle(const std::string& id,
                                    const std::string& companyAccountId,
                                    Ledger& ledger) {
    std::map<std::string, std::size_t>::const_iterator it = indexes_.find(id);
    if (it == indexes_.end())
        return Result::failure("CONTRACT_NOT_FOUND", "Contract does not exist.");
    DeliveryContract& c = contracts_[it->second];
    if (c.status == CONTRACT_SETTLED) return Result::success();
    if (c.status != CONTRACT_COMPLETED)
        return Result::failure("CONTRACT_NOT_COMPLETE", "Only a completed contract can be settled.");
    const std::string operationId = "contract-payout:" + c.contractId;
    Result paid = ledger.applyExternal(operationId, companyAccountId, c.reward,
                                        c.completedAt, "Delivery contract " + c.contractId);
    if (!paid.ok) return paid;
    c.status = CONTRACT_SETTLED;
    return Result::success();
}

Result DeliveryContractBook::restoreCompleted(const std::string& id,
                                              const std::string& eventId,
                                              SimulationSeconds completedAt) {
    std::map<std::string, std::size_t>::iterator it = indexes_.find(id);
    if (it == indexes_.end())
        return Result::failure("CONTRACT_NOT_FOUND", "Contract does not exist.");
    if (eventId.empty() || completedAt < 0)
        return Result::failure("CONTRACT_COMPLETION_RECORD", "Completion event and non-negative time are required.");
    DeliveryContract& c = contracts_[it->second];
    if (c.status == CONTRACT_COMPLETED || c.status == CONTRACT_SETTLED) {
        if (c.completionEventId == eventId && c.completedAt == completedAt)
            return Result::success();
        return Result::failure("CONTRACT_COMPLETION_CONFLICT", "Contract already has different completion evidence.");
    }
    if (c.status != CONTRACT_ACCEPTED || completedAt < c.acceptedAt ||
        (c.deadline > 0 && completedAt > c.deadline))
        return Result::failure("CONTRACT_COMPLETION_INVALID", "Completion record is inconsistent with contract state or deadline.");
    c.status = CONTRACT_COMPLETED;
    c.completionEventId = eventId;
    c.completedAt = completedAt;
    return Result::success();
}

Result DeliveryContractBook::restoreSettled(const std::string& id) {
    std::map<std::string, std::size_t>::iterator it = indexes_.find(id);
    if (it == indexes_.end())
        return Result::failure("CONTRACT_NOT_FOUND", "Contract does not exist.");
    DeliveryContract& c = contracts_[it->second];
    if (c.status == CONTRACT_SETTLED) return Result::success();
    if (c.status != CONTRACT_COMPLETED)
        return Result::failure("CONTRACT_NOT_COMPLETE", "Only a completed contract can be restored as settled.");
    c.status = CONTRACT_SETTLED;
    return Result::success();
}

Result DeliveryContractBook::cancel(const std::string& id) {
    std::map<std::string, std::size_t>::iterator it = indexes_.find(id);
    if (it == indexes_.end()) return Result::failure("CONTRACT_NOT_FOUND", "Contract does not exist.");
    DeliveryContract& c = contracts_[it->second];
    if (c.status == CONTRACT_CANCELLED) return Result::success();
    if (c.status != CONTRACT_ACCEPTED)
        return Result::failure("CONTRACT_STATE", "Only an active accepted contract can be cancelled.");
    c.status = CONTRACT_CANCELLED;
    return Result::success();
}

Result DeliveryContractBook::restoreCancelled(const std::string& id) {
    return cancel(id);
}

Result DeliveryContractBook::restoreExpired(const std::string& id) {
    std::map<std::string, std::size_t>::iterator it = indexes_.find(id);
    if (it == indexes_.end())
        return Result::failure("CONTRACT_NOT_FOUND", "Contract does not exist.");
    DeliveryContract& c = contracts_[it->second];
    if (c.status == CONTRACT_EXPIRED) return Result::success();
    if (c.status != CONTRACT_ACCEPTED)
        return Result::failure("CONTRACT_STATE", "Only an accepted contract can expire.");
    c.status = CONTRACT_EXPIRED;
    return Result::success();
}

std::vector<std::string> DeliveryContractBook::expireDue(SimulationSeconds now) {
    std::vector<std::string> expired;
    if (now < 0) return expired;
    for (std::size_t i = 0; i < contracts_.size(); ++i) {
        DeliveryContract& c = contracts_[i];
        if (c.status == CONTRACT_ACCEPTED && c.deadline > 0 && now > c.deadline) {
            c.status = CONTRACT_EXPIRED;
            expired.push_back(c.contractId);
        }
    }
    return expired;
}

const DeliveryContract* DeliveryContractBook::find(const std::string& id) const {
    std::map<std::string, std::size_t>::const_iterator it = indexes_.find(id);
    return it == indexes_.end() ? 0 : &contracts_[it->second];
}
const std::vector<DeliveryContract>& DeliveryContractBook::contracts() const {
    return contracts_;
}

} // namespace oem
