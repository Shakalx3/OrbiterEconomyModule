#ifndef OEM_CORE_ECONOMY_JOURNAL_H
#define OEM_CORE_ECONOMY_JOURNAL_H
#include "MarketTrade.h"
#include "DeliveryContract.h"
#include <map>
#include <string>
namespace oem {
class EconomyJournalStore {
public:
    // Inputs are explicit opening-state projections. Recovery is transactional across all stations.
    Result load(const std::string& path, const CargoCatalog& catalog, Ledger& ledger,
                std::map<std::string, CargoServiceInventory>& inventories,
                std::map<std::string, CargoManifest>& manifests,
                DeliveryContractBook* contracts = 0) const;
    // A single durable append commits either a non-financial exchange or a market trade.
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
                          const std::string& contractId) const;
    Result cancelContract(const std::string& path, DeliveryContractBook& contracts,
                          const std::string& contractId) const;
};
}
#endif
