#include "../../src/core/MarketPricing.h"
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
    return 0;
}
