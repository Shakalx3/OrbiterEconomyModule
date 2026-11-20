#ifndef OEM_CORE_MARKET_TRADE_JOURNAL_H
#define OEM_CORE_MARKET_TRADE_JOURNAL_H
#include "MarketTrade.h"
#include <map>
#include <string>
namespace oem {
class MarketTradeJournalStore {
public:
    // One journal is shared by a station. Caller supplies explicit opening accounts and stock.
    // Recovery reconstructs all vessel manifests touched by the station's trade history.
    Result load(const std::string& path, const CargoCatalog& catalog,
                Ledger& ledger, CargoServiceInventory& inventory,
                std::map<std::string, CargoManifest>& manifests) const;
    // Durable append is the commit point for account, station-stock and one vessel manifest.
    Result apply(const std::string& path, const CargoCatalog& catalog,
                 const MarketQuote& quote, const std::string& operationId,
                 const std::string& buyerAccountId, const std::string& stationAccountId,
                 Ledger& ledger, CargoServiceInventory& inventory,
                 CargoManifest& manifest) const;
};
}
#endif
