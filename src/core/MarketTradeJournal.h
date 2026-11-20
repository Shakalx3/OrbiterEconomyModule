#ifndef OEM_CORE_MARKET_TRADE_JOURNAL_H
#define OEM_CORE_MARKET_TRADE_JOURNAL_H
#include "MarketTrade.h"
#include <string>
namespace oem {
class MarketTradeJournalStore {
public:
    // Caller supplies configured opening account balances, station stock and empty/current projections.
    // A successful load replays the journal into candidates and publishes all projections together.
    Result load(const std::string& path, const CargoCatalog& catalog,
                Ledger& ledger, CargoServiceInventory& inventory,
                CargoManifest& manifest) const;
    // Durable append is the commit point. In-memory projections change only after fsync/FlushFileBuffers.
    Result apply(const std::string& path, const CargoCatalog& catalog,
                 const MarketQuote& quote, const std::string& operationId,
                 const std::string& buyerAccountId, const std::string& stationAccountId,
                 Ledger& ledger, CargoServiceInventory& inventory,
                 CargoManifest& manifest) const;
};
}
#endif
