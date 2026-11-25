#include "MarketPricing.h"
#include <cmath>
#include <limits>

namespace oem {

MarketPricingBook::MarketPricingBook(long long targetStock,
                                     int scarcityPremiumBasisPoints,
                                     int surplusDiscountBasisPoints)
    : targetStock_(targetStock),
      scarcityPremiumBasisPoints_(scarcityPremiumBasisPoints),
      surplusDiscountBasisPoints_(surplusDiscountBasisPoints) {}

namespace {
Result calculate(const CargoDefinition& cargo, long long stationStock,
                 long long targetStock, int scarcityBps, int surplusBps,
                 MarketSide side, Credits& price) {
    if (cargo.cargoId.empty() || stationStock < 0 || targetStock <= 0 ||
        scarcityBps < 0 || scarcityBps > 10000 ||
        surplusBps < 0 || surplusBps > 10000 ||
        (side != MARKET_BUY_FROM_STATION && side != MARKET_SELL_TO_STATION))
        return Result::failure("MARKET_PRICE_INPUT", "Cargo, stock, side and pricing policy values must be valid.");
    const Credits base = side == MARKET_BUY_FROM_STATION ? cargo.buyPrice : cargo.sellPrice;
    if (base < 0) return Result::failure("MARKET_PRICE_BASE", "Base cargo price cannot be negative.");
    if (base == 0) { price = 0; return Result::success(); }

    long double multiplier = 1.0L;
    if (stationStock < targetStock) {
        const long double scarcity = (long double)(targetStock - stationStock) /
                                     (long double)targetStock;
        multiplier += ((long double)scarcityBps / 10000.0L) * scarcity;
    } else if (stationStock > targetStock) {
        const long double surplus = (long double)(stationStock - targetStock) /
                                    (long double)targetStock;
        const long double bounded = surplus > 1.0L ? 1.0L : surplus;
        multiplier -= ((long double)surplusBps / 10000.0L) * bounded;
    }
    const long double value = (long double)base * multiplier;
    if (!std::isfinite((double)value) || value > (long double)std::numeric_limits<Credits>::max())
        return Result::failure("MARKET_PRICE_OVERFLOW", "Dynamic unit price exceeds the credit range.");
    Credits rounded = (Credits)std::floor(value + 0.5L);
    if (rounded < 1) rounded = 1;
    price = rounded;
    return Result::success();
}
}

Result MarketPricingBook::unitPriceForSide(const CargoDefinition& cargo,
                                            long long stationStock,
                                            MarketSide side,
                                            Credits& price) const {
    return calculate(cargo, stationStock, targetStock_,
                     scarcityPremiumBasisPoints_, surplusDiscountBasisPoints_,
                     side, price);
}

long long MarketPricingBook::targetStock() const { return targetStock_; }

} // namespace oem
