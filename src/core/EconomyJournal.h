#ifndef OEM_CORE_ECONOMY_JOURNAL_H
#define OEM_CORE_ECONOMY_JOURNAL_H
#include "MarketTrade.h"
#include "DeliveryContract.h"
#include "MissionObjectiveBook.h"
#include "PrimaryResource.h"
#include "ProductionRecipe.h"
#include <map>
#include <string>
namespace oem {
struct MissionFailurePolicy {
    unsigned int penaltyBasisPoints;
    int reputationDelta;
    MissionFailurePolicy() : penaltyBasisPoints(1000), reputationDelta(-5) {}
    bool valid() const { return penaltyBasisPoints <= 10000 && reputationDelta < 0 && reputationDelta >= -100; }
};

class MissionFailurePolicyStore {
public:
    Result loadOrCreate(const std::string& path, MissionFailurePolicy& policy) const;
};

class EconomyJournalStore {
public:
    // Inputs are explicit opening-state projections. Recovery is transactional across all stations.
    Result load(const std::string& path, const CargoCatalog& catalog, Ledger& ledger,
                std::map<std::string, CargoServiceInventory>& inventories,
                std::map<std::string, CargoManifest>& manifests,
                DeliveryContractBook* contracts = 0,
                MissionObjectiveBook* objectives = 0) const;
    // A single durable append commits either a non-financial exchange or a market trade.
    // Durable, replay-safe creation of primary resources at a configured source.
    Result recoverPrimaryProductionTimes(const std::string& path, PrimaryResourceBook& book) const;
    Result recoverProductionRecipeTimes(const std::string& path, ProductionRecipeBook& book) const;
    Result applyProductionRecipe(const std::string& path, const ProductionRecipeIntent& intent,
                std::map<std::string, CargoServiceInventory>& inventories) const;
    Result hasLocationHistory(const std::string& path, const std::string& locationId, bool& hasHistory) const;
    Result hasLocationCargoHistory(const std::string& path, const std::string& locationId,
                                   const std::string& cargoId, bool& hasHistory) const;
    Result applyPrimaryProduction(const std::string& path, const std::string& operationId,
                const std::string& sourceId, const std::string& locationId,
                const std::string& cargoId, long long quantity, double unitMassKg,
                SimulationSeconds simulationTime,
                std::map<std::string, CargoServiceInventory>& inventories) const;
    Result applyExchange(const std::string& path,
                std::map<std::string, CargoServiceInventory>& inventories,
                CargoManifest& manifest, const CargoTransfer& transfer) const;
    Result applyTrade(const std::string& path, const CargoCatalog& catalog,
                const MarketQuote& quote, const std::string& operationId,
                const std::string& buyerAccountId, Ledger& ledger,
                std::map<std::string, CargoServiceInventory>& inventories,
                CargoManifest& manifest) const;
    Result acceptContract(const std::string& path, DeliveryContractBook& contracts,
                          const DeliveryContract& contract) const;
    // Atomically accept a delivery contract, debit origin stock, and record the
    // contracted cargo in the vessel's OEM manifest in one durable journal record.
    Result acceptContractWithCargo(const std::string& path, DeliveryContractBook& contracts,
                          std::map<std::string, CargoServiceInventory>& inventories,
                          CargoManifest& manifest, const DeliveryContract& contract,
                          double unitMassKg) const;
    Result completeContract(const std::string& path, DeliveryContractBook& contracts,
                            const std::string& contractId, const std::string& eventId,
                            SimulationSeconds completedAt,
                            std::map<std::string, CargoServiceInventory>& inventories,
                            CargoManifest& manifest) const;
    Result settleContract(const std::string& path, DeliveryContractBook& contracts,
                          const std::string& contractId, const std::string& companyAccountId,
                          Ledger& ledger) const;
    // Durable, idempotent debit for a completed physical propellant fill.
    Result chargePropellant(const std::string& path, const std::string& operationId,
                            const std::string& accountId, Credits cost,
                            SimulationSeconds simulationTime, Ledger& ledger) const;
    Result expireContract(const std::string& path, DeliveryContractBook& contracts,
                          const std::string& contractId, Ledger& ledger,
                          const std::string& accountId, SimulationSeconds expiredAt,
                          const MissionFailurePolicy& policy = MissionFailurePolicy()) const;
    Result cancelContract(const std::string& path, DeliveryContractBook& contracts,
                          const std::string& contractId) const;
    Result acceptMissionObjective(const std::string& path, MissionObjectiveBook& objectives,
                          const MissionObjective& objective, Credits reward) const;
    // Requires the objective book to have observed this exact matching Orbiter event first.
    Result completeMissionObjective(const std::string& path, MissionObjectiveBook& objectives,
                          const std::string& missionId, const std::string& eventId,
                          SimulationSeconds completedAt) const;
    Result settleMissionObjective(const std::string& path, MissionObjectiveBook& objectives,
                          const std::string& missionId, const std::string& accountId,
                          Ledger& ledger) const;
    Result expireMissionObjective(const std::string& path, MissionObjectiveBook& objectives,
                          const std::string& missionId, Ledger& ledger,
                          const std::string& accountId, SimulationSeconds expiredAt,
                          const MissionFailurePolicy& policy = MissionFailurePolicy()) const;
    Result cancelMissionObjective(const std::string& path, MissionObjectiveBook& objectives,
                          const std::string& missionId) const;
};
}
#endif
