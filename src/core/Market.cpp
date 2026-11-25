#include "Market.h"
#include <cmath>
#include <limits>

namespace oem {

CargoDefinition::CargoDefinition()
    : cargoId(), displayName(), unitMassKg(0.0), buyPrice(0), sellPrice(0) {}

Result CargoCatalog::add(const CargoDefinition& definition) {
    if (definition.cargoId.empty() || definition.displayName.empty())
        return Result::failure("INVALID_CARGO_DEFINITION", "Cargo ID and display name are required.");
    if (!(definition.unitMassKg > 0.0) || !std::isfinite(definition.unitMassKg))
        return Result::failure("INVALID_CARGO_MASS", "Cargo unit mass must be positive and finite.");
    if (definition.buyPrice < 0 || definition.sellPrice < 0)
        return Result::failure("INVALID_CARGO_PRICE", "Cargo prices cannot be negative.");
    if (indexes_.find(definition.cargoId) != indexes_.end())
        return Result::failure("CARGO_ALREADY_DEFINED", "Cargo ID already exists in the catalog.");
    entries_.push_back(definition);
    indexes_[definition.cargoId] = entries_.size() - 1;
    return Result::success();
}

bool CargoCatalog::find(const std::string& cargoId, CargoDefinition& definition) const {
    std::map<std::string, std::size_t>::const_iterator it = indexes_.find(cargoId);
    if (it == indexes_.end()) return false;
    definition = entries_[it->second];
    return true;
}

const std::vector<CargoDefinition>& CargoCatalog::entries() const { return entries_; }

MarketQuote::MarketQuote()
    : quoteId(), stationId(), cargoId(), side(MARKET_BUY_FROM_STATION),
      quantity(0), unitPrice(0), totalPrice(0), stationStockAtQuote(0), simulationTime(0),
      maximumStationStock((std::numeric_limits<long long>::max)()) {}

Result MarketQuoteEngine::createQuote(const CargoCatalog& catalog,
                                      const CargoServiceInventory& inventory,
                                      const std::string& quoteId,
                                      const std::string& cargoId,
                                      MarketSide side,
                                      long long quantity,
                                      SimulationSeconds simulationTime,
                                      MarketQuote& quote,
                                      long long maximumStationStock) const {
    if (quoteId.empty() || inventory.serviceVesselId().empty() || cargoId.empty())
        return Result::failure("INVALID_MARKET_QUOTE", "Quote, station and cargo IDs are required.");
    if (quantity <= 0)
        return Result::failure("INVALID_MARKET_QUANTITY", "Market quantity must be positive.");
    if (side != MARKET_BUY_FROM_STATION && side != MARKET_SELL_TO_STATION)
        return Result::failure("INVALID_MARKET_SIDE", "Unknown market side.");

    CargoDefinition definition;
    if (!catalog.find(cargoId, definition))
        return Result::failure("UNKNOWN_CARGO", "Cargo ID is not present in the catalog.");

    const long long stock = inventory.quantity(cargoId);
    if (side == MARKET_BUY_FROM_STATION && stock < quantity)
        return Result::failure("INSUFFICIENT_STATION_STOCK", "The station does not have enough stock for this quote.");
    if (side == MARKET_SELL_TO_STATION &&
        stock > (std::numeric_limits<long long>::max)() - quantity)
        return Result::failure("CARGO_QUANTITY_OVERFLOW", "Station stock would overflow after this sale.");
    if (maximumStationStock < 0)
        return Result::failure("MARKET_STOCK_CAPACITY", "Station stock capacity cannot be negative.");
    if (side == MARKET_SELL_TO_STATION &&
        (stock >= maximumStationStock || quantity > maximumStationStock - stock))
        return Result::failure("MARKET_STATION_FULL", "The station has insufficient free capacity for this cargo.");

    const Credits unitPrice = side == MARKET_BUY_FROM_STATION
        ? definition.buyPrice : definition.sellPrice;
    if (unitPrice > 0 &&
        quantity > std::numeric_limits<Credits>::max() / unitPrice)
        return Result::failure("MARKET_VALUE_OVERFLOW", "Total quote value would overflow integer credits.");

    MarketQuote candidate;
    candidate.quoteId = quoteId;
    candidate.stationId = inventory.serviceVesselId();
    candidate.cargoId = cargoId;
    candidate.side = side;
    candidate.quantity = quantity;
    candidate.unitPrice = unitPrice;
    candidate.totalPrice = unitPrice * quantity;
    candidate.stationStockAtQuote = stock;
    candidate.simulationTime = simulationTime;
    candidate.maximumStationStock = maximumStationStock;
    quote = candidate;
    return Result::success();
}

} // namespace oem
