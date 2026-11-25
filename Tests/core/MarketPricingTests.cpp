#include "../../src/core/MarketPricing.h"
#include "../../src/core/MarketTrade.h"
#include <cassert>

static oem::CargoDefinition makeCargo() {
    oem::CargoDefinition d;
    d.cargoId="oxygen"; d.displayName="Oxygen";
    d.unitMassKg=1.0; d.buyPrice=100; d.sellPrice=60;
    return d;
}

int main() {
    const oem::CargoDefinition cargo=makeCargo();
    oem::MarketPricingBook pricing(100, 5000, 2500);
    oem::Credits buy=0, sell=0;

    assert(pricing.unitPriceForSide(cargo, 0, oem::MARKET_BUY_FROM_STATION, buy).ok);
    assert(pricing.unitPriceForSide(cargo, 0, oem::MARKET_SELL_TO_STATION, sell).ok);
    assert(buy==150 && sell==90);

    assert(pricing.unitPriceForSide(cargo, 100, oem::MARKET_BUY_FROM_STATION, buy).ok);
    assert(pricing.unitPriceForSide(cargo, 100, oem::MARKET_SELL_TO_STATION, sell).ok);
    assert(buy==100 && sell==60);

    assert(pricing.unitPriceForSide(cargo, 200, oem::MARKET_BUY_FROM_STATION, buy).ok);
    assert(pricing.unitPriceForSide(cargo, 200, oem::MARKET_SELL_TO_STATION, sell).ok);
    assert(buy==75 && sell==45);

    // Scarcity and surplus effects are bounded, and separate locations can quote
    // different prices for the same commodity because their stocks differ.
    assert(pricing.unitPriceForSide(cargo, 1000, oem::MARKET_BUY_FROM_STATION, buy).ok);
    assert(buy==75);
    assert(pricing.unitPriceForSide(cargo, 25, oem::MARKET_BUY_FROM_STATION, buy).ok);
    assert(buy==138);

    oem::CargoDefinition freeCargo=cargo;
    freeCargo.buyPrice=0; freeCargo.sellPrice=0;
    assert(pricing.unitPriceForSide(freeCargo, 0, oem::MARKET_BUY_FROM_STATION, buy).ok);
    assert(buy==0);

    oem::MarketPricingBook invalid(0, 5000, 2500);
    assert(!invalid.unitPriceForSide(cargo, 10, oem::MARKET_BUY_FROM_STATION, buy).ok);
    assert(!pricing.unitPriceForSide(cargo, -1, oem::MARKET_BUY_FROM_STATION, buy).ok);

    // A committed quote keeps the price calculated at the moment of trade.
    // The next quote sees the changed station stock and therefore a new price.
    oem::CargoCatalog catalog;
    assert(catalog.add(cargo).ok);
    oem::CargoServiceInventory station("ISS");
    assert(station.setInitialStock("oxygen", 50, 1.0).ok);
    oem::CargoManifest ship("GL-01");
    oem::Ledger ledger;
    assert(ledger.createAccount("company", 100000).ok);
    oem::MarketQuoteEngine quotes;
    oem::MarketTradeEngine trades;
    oem::MarketQuote first;
    assert(quotes.createQuote(catalog, station, "q1", "oxygen",
        oem::MARKET_BUY_FROM_STATION, 10, 100, first).ok);
    assert(pricing.unitPriceForSide(cargo, station.quantity("oxygen"),
        oem::MARKET_BUY_FROM_STATION, buy).ok);
    first.unitPrice=buy; first.totalPrice=buy*first.quantity;
    const oem::Credits firstUnitPrice=first.unitPrice;
    assert(trades.execute(catalog, first, "trade-1", "company",
        ledger, station, ship).ok);
    assert(station.quantity("oxygen")==40 && ship.quantity("oxygen")==10);
    assert(pricing.unitPriceForSide(cargo, station.quantity("oxygen"),
        oem::MARKET_BUY_FROM_STATION, buy).ok);
    assert(buy>firstUnitPrice);

    oem::MarketQuote sale;
    assert(quotes.createQuote(catalog, station, "q2", "oxygen",
        oem::MARKET_SELL_TO_STATION, 5, 101, sale).ok);
    assert(pricing.unitPriceForSide(cargo, station.quantity("oxygen"),
        oem::MARKET_SELL_TO_STATION, sell).ok);
    sale.unitPrice=sell; sale.totalPrice=sell*sale.quantity;
    assert(trades.execute(catalog, sale, "trade-2", "company",
        ledger, station, ship).ok);
    assert(station.quantity("oxygen")==45 && ship.quantity("oxygen")==5);
    return 0;
}
