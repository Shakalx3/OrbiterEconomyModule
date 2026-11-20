#ifndef OEM_CORE_MARKET_TRADE_H
#define OEM_CORE_MARKET_TRADE_H

#include "Ledger.h"
#include "Market.h"

namespace oem {

class MarketTradeEngine {
public:
    // Applies all three in-memory projections transactionally. Durable journal integration
    // is a separate prerequisite before this operation is exposed by the native UI.
    Result execute(const CargoCatalog& catalog,
                   const MarketQuote& quote,
                   const std::string& operationId,
                   const std::string& buyerAccountId,
                   const std::string& stationAccountId,
                   Ledger& ledger,
                   CargoServiceInventory& inventory,
                   CargoManifest& manifest) const;
};

} // namespace oem

#endif
