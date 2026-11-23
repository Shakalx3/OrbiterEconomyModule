#ifndef OEM_CORE_DELIVERY_CONTRACT_H
#define OEM_CORE_DELIVERY_CONTRACT_H

#include "CargoManifest.h"
#include "Ledger.h"
#include "SimulationEvent.h"
#include <map>
#include <string>
#include <vector>

namespace oem {

enum ContractStatus {
    CONTRACT_ACCEPTED = 1,
    CONTRACT_COMPLETED = 2,
    CONTRACT_SETTLED = 3,
    CONTRACT_EXPIRED = 4
};

struct DeliveryContract {
    std::string contractId;
    std::string vesselId;
    std::string cargoId;
    long long quantity;
    std::string originLocationId;
    std::string destinationLocationId;
    // Optional celestial-body constraint for planetary destination evidence.
    std::string destinationBodyId;
    Credits reward;
    SimulationSeconds acceptedAt;
    SimulationSeconds deadline;
    ContractStatus status;
    std::string completionEventId;
    SimulationSeconds completedAt;
    DeliveryContract();
};

// Portable, deterministic delivery rules. Completion requires the specified
// vessel to carry enough cargo loaded at the named origin after acceptance,
// then produce a verified landing at the destination base or docking event
// with the exact destination vessel. It never mutates Orbiter physics.
class DeliveryContractBook {
public:
    DeliveryContractBook();

    Result accept(const DeliveryContract& contract);
    Result observe(const SimulationEvent& event, const CargoManifest& manifest);
    Result settle(const std::string& contractId, const std::string& companyAccountId,
                  Ledger& ledger);
    // Journal replay hooks. These restore already-committed facts without re-evaluating
    // Orbiter state or issuing a second payment.
    Result restoreCompleted(const std::string& contractId, const std::string& eventId,
                            SimulationSeconds completedAt);
    Result restoreSettled(const std::string& contractId);
    Result restoreExpired(const std::string& contractId);
    std::vector<std::string> expireDue(SimulationSeconds now);

    const DeliveryContract* find(const std::string& contractId) const;
    const std::vector<DeliveryContract>& contracts() const;

private:
    long long eligibleCargoAt(const DeliveryContract& contract,
                              const CargoManifest& manifest,
                              SimulationSeconds eventTime) const;
    bool isDestinationEvent(const DeliveryContract& contract,
                            const SimulationEvent& event) const;

    std::map<std::string, std::size_t> indexes_;
    std::vector<DeliveryContract> contracts_;
};

} // namespace oem

#endif
