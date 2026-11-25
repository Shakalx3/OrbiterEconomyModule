#ifndef OEM_CORE_MARKET_PRICING_H
#define OEM_CORE_MARKET_PRICING_H

#include "Market.h"

namespace oem {

// Stock-driven local pricing. Every location has its own inventory and therefore
// its own price. A purchase reduces supply and raises the next quote; a sale
// increases supply and lowers the next quote.
class MarketPricingBook {
public:
    explicit MarketPricingBook(long long targetStock = 100,
                               int scarcityPremiumBasisPoints = 5000,
                               int surplusDiscountBasisPoints = 2500);
    Result unitPriceForSide(const CargoDefinition& cargo, long long stationStock,
                            MarketSide side, Credits& price) const;
    long long targetStock() const;
private:
    long long targetStock_;
    int scarcityPremiumBasisPoints_;
    int surplusDiscountBasisPoints_;
};

} // namespace oem
#endif
