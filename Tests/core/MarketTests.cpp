#include "../../src/core/Market.h"
#include <cassert>
#include <limits>

static oem::CargoDefinition item(const char* id, const char* name,
                                 double mass, oem::Credits buy, oem::Credits sell) {
    oem::CargoDefinition d;
    d.cargoId = id; d.displayName = name; d.unitMassKg = mass;
    d.buyPrice = buy; d.sellPrice = sell;
    return d;
}

int main() {
    oem::CargoCatalog catalog;
    assert(catalog.add(item("oxygen", "Oxygen", 10.0, 125, 80)).ok);
    assert(!catalog.add(item("oxygen", "Duplicate", 10.0, 1, 1)).ok);
    assert(!catalog.add(item("bad", "Bad mass", 0.0, 1, 1)).ok);
    assert(!catalog.add(item("bad-price", "Bad price", 1.0, -1, 1)).ok);

    oem::CargoServiceInventory station("ISS");
    assert(station.setInitialStock("oxygen", 12, 10.0).ok);
    oem::MarketQuoteEngine engine;
    oem::MarketQuote quote;
    assert(engine.createQuote(catalog, station, "q-buy-1", "oxygen",
        oem::MARKET_BUY_FROM_STATION, 5, 100, quote).ok);
    assert(quote.stationId == "ISS" && quote.cargoId == "oxygen");
    assert(quote.quantity == 5 && quote.unitPrice == 125 && quote.totalPrice == 625);
    assert(quote.stationStockAtQuote == 12 && quote.simulationTime == 100);

    assert(engine.createQuote(catalog, station, "q-sell-1", "oxygen",
        oem::MARKET_SELL_TO_STATION, 3, 101, quote).ok);
    assert(quote.unitPrice == 80 && quote.totalPrice == 240);
    assert(quote.side == oem::MARKET_SELL_TO_STATION);

    assert(!engine.createQuote(catalog, station, "q-short", "oxygen",
        oem::MARKET_BUY_FROM_STATION, 13, 102, quote).ok);
    assert(!engine.createQuote(catalog, station, "q-unknown", "argon",
        oem::MARKET_BUY_FROM_STATION, 1, 102, quote).ok);
    assert(!engine.createQuote(catalog, station, "q-zero", "oxygen",
        oem::MARKET_BUY_FROM_STATION, 0, 102, quote).ok);

    oem::CargoCatalog expensive;
    assert(expensive.add(item("rare", "Rare material", 1.0,
        std::numeric_limits<oem::Credits>::max(), 0)).ok);
    assert(!engine.createQuote(expensive, station, "q-overflow", "rare",
        oem::MARKET_BUY_FROM_STATION, 2, 103, quote).ok);

    oem::CargoServiceInventory nearLimit("LIMIT");
    assert(nearLimit.setInitialStock("oxygen",
        std::numeric_limits<long long>::max(), 10.0).ok);
    assert(!engine.createQuote(catalog, nearLimit, "q-stock-overflow", "oxygen",
        oem::MARKET_SELL_TO_STATION, 1, 104, quote).ok);
    return 0;
}
