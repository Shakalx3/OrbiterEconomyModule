#ifndef OEM_CORE_MARKET_PRICING_H
#define OEM_CORE_MARKET_PRICING_H

#include "Market.h"
#include <cmath>
#include <limits>

namespace oem {

// Header-defined so both the portable CMake library and the manually enumerated
// Orbiter Win32 bridge source list use the exact same pricing implementation.
class MarketPricingBook {
public:
    explicit MarketPricingBook(long long targetStock = 100,
                               int scarcityPremiumBasisPoints = 5000,
                               int surplusDiscountBasisPoints = 2500)
        : targetStock_(targetStock),
          scarcityPremiumBasisPoints_(scarcityPremiumBasisPoints),
          surplusDiscountBasisPoints_(surplusDiscountBasisPoints) {}

    Result unitPriceForSide(const CargoDefinition& cargo, long long stationStock,
                            MarketSide side, Credits& price) const {
        if (cargo.cargoId.empty() || stationStock < 0 || targetStock_ <= 0 ||
            scarcityPremiumBasisPoints_ < 0 || scarcityPremiumBasisPoints_ > 10000 ||
            surplusDiscountBasisPoints_ < 0 || surplusDiscountBasisPoints_ > 10000 ||
            (side != MARKET_BUY_FROM_STATION && side != MARKET_SELL_TO_STATION))
            return Result::failure("MARKET_PRICE_INPUT",
                "Cargo, stock, side and pricing policy values must be valid.");

        const Credits base = side == MARKET_BUY_FROM_STATION
            ? cargo.buyPrice : cargo.sellPrice;
        if (base < 0)
            return Result::failure("MARKET_PRICE_BASE", "Base cargo price cannot be negative.");
        if (base == 0) {
            price = 0;
            return Result::success();
        }

        long double multiplier = 1.0L;
        if (stationStock < targetStock_) {
            const long double scarcity =
                (long double)(targetStock_ - stationStock) / (long double)targetStock_;
            multiplier += ((long double)scarcityPremiumBasisPoints_ / 10000.0L) * scarcity;
        } else if (stationStock > targetStock_) {
            const long double surplus =
                (long double)(stationStock - targetStock_) / (long double)targetStock_;
            const long double bounded = surplus > 1.0L ? 1.0L : surplus;
            multiplier -= ((long double)surplusDiscountBasisPoints_ / 10000.0L) * bounded;
        }

        const long double value = (long double)base * multiplier;
        if (!std::isfinite((double)value) ||
            value > (long double)(std::numeric_limits<Credits>::max)())
            return Result::failure("MARKET_PRICE_OVERFLOW",
                "Dynamic unit price exceeds the credit range.");

        Credits rounded = (Credits)std::floor(value + 0.5L);
        if (rounded < 1) rounded = 1;
        price = rounded;
        return Result::success();
    }

    long long targetStock() const { return targetStock_; }

private:
    long long targetStock_;
    int scarcityPremiumBasisPoints_;
    int surplusDiscountBasisPoints_;
};

} // namespace oem
#endif
