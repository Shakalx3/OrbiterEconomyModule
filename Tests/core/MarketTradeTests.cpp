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

    assert(trades.execute(catalog, buy, "trade-1", "company", "ISS-account",
        ledger, station, ship).ok);
    oem::Credits balance=0;
    assert(ledger.getBalance("company", balance) && balance==500);
    assert(!ledger.getBalance("ISS-account", balance));
    assert(station.quantity("oxygen")==6 && ship.quantity("oxygen")==4);

    // Exact replay does not debit money or move cargo twice.
    assert(trades.execute(catalog, buy, "trade-1", "company", "ISS-account",
        ledger, station, ship).ok);
    assert(ledger.getBalance("company", balance) && balance==500);
    assert(station.quantity("oxygen")==6 && ship.quantity("oxygen")==4);

    oem::MarketQuote sell;
    assert(quotes.createQuote(catalog, station, "q2", "oxygen",
        oem::MARKET_SELL_TO_STATION, 2, 201, sell).ok);
    assert(trades.execute(catalog, sell, "trade-2", "company", "ISS-account",
        ledger, station, ship).ok);
    assert(ledger.getBalance("company", balance) && balance==660);
    assert(!ledger.getBalance("ISS-account", balance));
    assert(station.quantity("oxygen")==8 && ship.quantity("oxygen")==2);

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
    assert(!trades.execute(second, expensive, "trade-3", "company", "ISS-account",
        ledger, richStock, emptyShip).ok);
    assert(richStock.quantity("costly")==beforeStock && emptyShip.quantity("costly")==0);
    assert(ledger.getBalance("company", balance) && balance==660);

    // A forged/stale price is rejected before any projection is changed.
    expensive.unitPrice=1; expensive.totalPrice=1;
    assert(!trades.execute(second, expensive, "trade-4", "company", "ISS-account",
        ledger, richStock, emptyShip).ok);
    assert(richStock.quantity("costly")==1 && emptyShip.quantity("costly")==0);
    return 0;
}
