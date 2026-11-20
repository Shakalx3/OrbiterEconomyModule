#ifndef OEM_CORE_MARKET_TRADE_H
#define OEM_CORE_MARKET_TRADE_H

#include "Ledger.h"
#include "Market.h"

namespace oem {

class MarketTradeEngine {
public:
    // Applies the three projections transactionally. EconomyJournalStore provides the durable commit
    // point before publishing these candidate projections in the native bridge.
    Result execute(const CargoCatalog& catalog,
                   const MarketQuote& quote,
                   const std::string& operationId,
                   const std::string& buyerAccountId,
                   Ledger& ledger,
                   CargoServiceInventory& inventory,
                   CargoManifest& manifest) const;
};

} // namespace oem

#endif
