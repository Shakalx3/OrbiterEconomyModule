#include "MarketTrade.h"
#include <limits>

namespace oem {

Result MarketTradeEngine::execute(const CargoCatalog& catalog,
                                  const MarketQuote& quote,
                                  const std::string& operationId,
                                  const std::string& buyerAccountId,
                                  Ledger& ledger,
                                  CargoServiceInventory& inventory,
                                  CargoManifest& manifest) const {
    if (operationId.empty() || quote.quoteId.empty() || buyerAccountId.empty())
        return Result::failure("INVALID_MARKET_TRADE", "Operation, quote and buyer account IDs are required.");
    if (quote.stationId != inventory.serviceVesselId() ||
        manifest.vesselId().empty() || quote.cargoId.empty() || quote.quantity <= 0)
        return Result::failure("MARKET_TRADE_CONTEXT", "Quote station, cargo, quantity or vessel context is invalid.");
    if (quote.side != MARKET_BUY_FROM_STATION && quote.side != MARKET_SELL_TO_STATION)
        return Result::failure("INVALID_MARKET_SIDE", "Unknown market side.");

    CargoDefinition definition;
    if (!catalog.find(quote.cargoId, definition))
        return Result::failure("UNKNOWN_CARGO", "Cargo ID is not present in the catalog.");
    const Credits expectedUnitPrice = quote.side == MARKET_BUY_FROM_STATION
        ? definition.buyPrice : definition.sellPrice;
    if (quote.unitPrice != expectedUnitPrice ||
        (expectedUnitPrice > 0 &&
         quote.quantity > std::numeric_limits<Credits>::max() / expectedUnitPrice) ||
        quote.totalPrice != expectedUnitPrice * quote.quantity)
        return Result::failure("MARKET_QUOTE_INVALID", "Quote price does not match the catalog or its total is invalid.");

    CargoTransfer transfer;
    transfer.transferId = operationId;
    transfer.direction = quote.side == MARKET_BUY_FROM_STATION ? CARGO_LOAD : CARGO_UNLOAD;
    transfer.vesselId = manifest.vesselId();
    transfer.locationId = inventory.serviceVesselId();
    transfer.cargoId = quote.cargoId;
    transfer.quantity = quote.quantity;
    transfer.unitMassKg = definition.unitMassKg;
    transfer.simulationTime = quote.simulationTime;

    Ledger candidateLedger = ledger;
    CargoServiceInventory candidateInventory = inventory;
    CargoManifest candidateManifest = manifest;

    Result cargoResult = candidateInventory.apply(transfer);
    if (!cargoResult.ok) return cargoResult;
    if (quote.side == MARKET_SELL_TO_STATION &&
        candidateInventory.quantity(quote.cargoId) > quote.maximumStationStock)
        return Result::failure("MARKET_STATION_FULL", "The station stock capacity has been reached since this quote was created.");
    cargoResult = transfer.direction == CARGO_LOAD
        ? candidateManifest.load(transfer) : candidateManifest.unload(transfer);
    if (!cargoResult.ok) return cargoResult;

    const std::string reason = quote.side == MARKET_BUY_FROM_STATION
        ? "MARKET_BUY:" + quote.cargoId : "MARKET_SELL:" + quote.cargoId;
    const Credits delta = quote.side == MARKET_BUY_FROM_STATION ? -quote.totalPrice : quote.totalPrice;
    Result moneyResult = candidateLedger.applyExternal(operationId, buyerAccountId, delta, quote.simulationTime, reason);
    if (!moneyResult.ok) return moneyResult;

    ledger = candidateLedger;
    inventory = candidateInventory;
    manifest = candidateManifest;
    return Result::success();
}

} // namespace oem
