#ifndef OEM_CORE_MARKET_H
#define OEM_CORE_MARKET_H

#include "CargoServiceInventory.h"
#include "Types.h"
#include <map>
#include <limits>
#include <string>
#include <vector>

namespace oem {

struct CargoDefinition {
    std::string cargoId;
    std::string displayName;
    double unitMassKg;
    Credits buyPrice;
    Credits sellPrice;
    CargoDefinition();
};

class CargoCatalog {
public:
    Result add(const CargoDefinition& definition);
    bool find(const std::string& cargoId, CargoDefinition& definition) const;
    const std::vector<CargoDefinition>& entries() const;
private:
    std::map<std::string, std::size_t> indexes_;
    std::vector<CargoDefinition> entries_;
};

enum MarketSide {
    MARKET_BUY_FROM_STATION = 1,
    MARKET_SELL_TO_STATION = 2
};

struct MarketQuote {
    std::string quoteId;
    std::string stationId;
    std::string cargoId;
    MarketSide side;
    long long quantity;
    Credits unitPrice;
    Credits totalPrice;
    long long stationStockAtQuote;
    SimulationSeconds simulationTime;
    long long maximumStationStock;
    MarketQuote();
};

class MarketQuoteEngine {
public:
    Result createQuote(const CargoCatalog& catalog,
                       const CargoServiceInventory& inventory,
                       const std::string& quoteId,
                       const std::string& cargoId,
                       MarketSide side,
                       long long quantity,
                       SimulationSeconds simulationTime,
                       MarketQuote& quote,
                       long long maximumStationStock = (std::numeric_limits<long long>::max)()) const;
};

} // namespace oem

#endif
