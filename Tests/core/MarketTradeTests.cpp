#include "../../src/core/MarketTrade.h"
#include <cassert>

static oem::CargoDefinition oxygen() {
    oem::CargoDefinition d; d.cargoId="oxygen"; d.displayName="Oxygen";
    d.unitMassKg=10.0; d.buyPrice=125; d.sellPrice=80; return d;
}

int main() {
    oem::CargoCatalog catalog; assert(catalog.add(oxygen()).ok);
    oem::CargoServiceInventory station("ISS");
    assert(station.setInitialStock("oxygen", 10, 10.0).ok);
    oem::CargoManifest ship("GL-01");
    oem::Ledger ledger;
    assert(ledger.createAccount("company", 1000).ok);
    oem::MarketQuoteEngine quotes;
    oem::MarketTradeEngine trades;
    oem::MarketQuote buy;
    assert(quotes.createQuote(catalog, station, "q1", "oxygen",
        oem::MARKET_BUY_FROM_STATION, 4, 200, buy).ok);

    assert(trades.execute(catalog, buy, "trade-1", "company",
        ledger, station, ship).ok);
    oem::Credits balance=0;
    assert(ledger.getBalance("company", balance) && balance==500);
    assert(!ledger.getBalance("ISS-account", balance));
    assert(station.quantity("oxygen")==6 && ship.quantity("oxygen")==4);

    // Exact replay does not debit money or move cargo twice.
    assert(trades.execute(catalog, buy, "trade-1", "company",
        ledger, station, ship).ok);
    assert(ledger.getBalance("company", balance) && balance==500);
    assert(station.quantity("oxygen")==6 && ship.quantity("oxygen")==4);

    oem::MarketQuote sell;
    assert(quotes.createQuote(catalog, station, "q2", "oxygen",
        oem::MARKET_SELL_TO_STATION, 2, 201, sell).ok);
    assert(trades.execute(catalog, sell, "trade-2", "company",
        ledger, station, ship).ok);
    assert(ledger.getBalance("company", balance) && balance==660);
    assert(!ledger.getBalance("ISS-account", balance));
    assert(station.quantity("oxygen")==8 && ship.quantity("oxygen")==2);

    // A primary-resource stock cap applies to market buybacks as well as production.
    oem::CargoServiceInventory cappedStock("ISS");
    assert(cappedStock.setInitialStock("oxygen", 149, 10.0).ok);
    oem::CargoManifest seller("GL-03");
    oem::CargoTransfer cargo;
    cargo.transferId="load-seller"; cargo.direction=oem::CARGO_LOAD;
    cargo.vesselId="GL-03"; cargo.locationId="ISS"; cargo.cargoId="oxygen";
    cargo.quantity=2; cargo.unitMassKg=10.0; cargo.simulationTime=200;
    assert(seller.load(cargo).ok);
    oem::MarketQuote overCapacity;
    assert(!quotes.createQuote(catalog, cappedStock, "q-cap-bad", "oxygen",
        oem::MARKET_SELL_TO_STATION, 2, 201, overCapacity, 150).ok);
    oem::MarketQuote toCapacity;
    assert(quotes.createQuote(catalog, cappedStock, "q-cap-good", "oxygen",
        oem::MARKET_SELL_TO_STATION, 1, 201, toCapacity, 150).ok);
    assert(trades.execute(catalog, toCapacity, "trade-cap-1", "company",
        ledger, cappedStock, seller).ok);
    assert(cappedStock.quantity("oxygen")==150 && seller.quantity("oxygen")==1);
    oem::MarketQuote fullStation;
    assert(!quotes.createQuote(catalog, cappedStock, "q-cap-full", "oxygen",
        oem::MARKET_SELL_TO_STATION, 1, 202, fullStation, 150).ok);

    // Execution rechecks capacity if inventory changed after a quote was issued.
    oem::CargoServiceInventory staleStock("ISS");
    assert(staleStock.setInitialStock("oxygen", 149, 10.0).ok);
    oem::CargoManifest staleSeller("GL-04");
    cargo.transferId="load-stale-seller"; cargo.vesselId="GL-04";
    assert(staleSeller.load(cargo).ok);
    oem::MarketQuote staleQuote;
    assert(quotes.createQuote(catalog, staleStock, "q-cap-stale", "oxygen",
        oem::MARKET_SELL_TO_STATION, 1, 203, staleQuote, 150).ok);
    oem::CargoTransfer production;
    production.transferId="production-between-quote-and-trade";
    production.direction=oem::CARGO_UNLOAD; production.vesselId="OEM-PRODUCER";
    production.locationId="ISS"; production.cargoId="oxygen";
    production.quantity=1; production.unitMassKg=10.0; production.simulationTime=203;
    assert(staleStock.apply(production).ok);
    assert(ledger.getBalance("company", balance));
    const oem::Credits beforeStaleTrade=balance;
    assert(!trades.execute(catalog, staleQuote, "trade-cap-stale", "company",
        ledger, staleStock, staleSeller).ok);
    assert(staleStock.quantity("oxygen")==150 && staleSeller.quantity("oxygen")==2);
    assert(ledger.getBalance("company", balance) && balance==beforeStaleTrade);

    // Insufficient funds leaves all three projections unchanged.
    oem::CargoDefinition costly=oxygen(); costly.cargoId="costly";
    costly.buyPrice=10000; costly.sellPrice=1;
    oem::CargoCatalog second; assert(second.add(costly).ok);
    oem::CargoServiceInventory richStock("ISS");
    assert(richStock.setInitialStock("costly", 1, 10.0).ok);
    oem::CargoManifest emptyShip("GL-02");
    oem::MarketQuote expensive;
    assert(quotes.createQuote(second, richStock, "q3", "costly",
        oem::MARKET_BUY_FROM_STATION, 1, 202, expensive).ok);
    const long long beforeStock=richStock.quantity("costly");
    assert(!trades.execute(second, expensive, "trade-3", "company",
        ledger, richStock, emptyShip).ok);
    assert(richStock.quantity("costly")==beforeStock && emptyShip.quantity("costly")==0);
    assert(ledger.getBalance("company", balance) && balance==740);

    // A malformed quote total is rejected before any projection is changed.
    expensive.unitPrice=1; expensive.totalPrice=2;
    assert(!trades.execute(second, expensive, "trade-4", "company",
        ledger, richStock, emptyShip).ok);
    assert(richStock.quantity("costly")==1 && emptyShip.quantity("costly")==0);
    return 0;
}
