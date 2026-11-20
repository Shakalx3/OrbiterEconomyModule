#ifndef OEM_CORE_ECONOMY_JOURNAL_H
#define OEM_CORE_ECONOMY_JOURNAL_H
#include "MarketTrade.h"
#include <map>
#include <string>
namespace oem {
class EconomyJournalStore {
public:
    // Inputs are explicit opening-state projections. Recovery is transactional across all stations.
    Result load(const std::string& path, const CargoCatalog& catalog, Ledger& ledger,
                std::map<std::string, CargoServiceInventory>& inventories,
                std::map<std::string, CargoManifest>& manifests) const;
    // A single durable append commits either a non-financial exchange or a market trade.
    Result applyExchange(const std::string& path,
                std::map<std::string, CargoServiceInventory>& inventories,
                CargoManifest& manifest, const CargoTransfer& transfer) const;
    Result applyTrade(const std::string& path, const CargoCatalog& catalog,
                const MarketQuote& quote, const std::string& operationId,
                const std::string& buyerAccountId, Ledger& ledger,
                std::map<std::string, CargoServiceInventory>& inventories,
                CargoManifest& manifest) const;
};
}
#endif
