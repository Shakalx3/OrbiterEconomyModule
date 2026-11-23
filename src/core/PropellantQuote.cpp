#include "PropellantQuote.h"
#include <cmath>
#include <limits>

namespace oem {

PropellantPriceContext::PropellantPriceContext()
    : locationId(), locationKind(PROPELLANT_LOCATION_ORBITAL_STATION),
      celestialBody() {}

PropellantQuote::PropellantQuote()
    : quoteId(), context(), requestedMassKg(0.0),
      unitPriceCreditsPerKg(0.0), totalCostCredits(0), balanceAtQuote(0),
      fillPlan() {}

Result createPropellantQuote(
    const std::vector<PropellantResourceState>& resources,
    const std::string& quoteId,
    const PropellantPriceContext& context,
    double requestedMassKg,
    double unitPriceCreditsPerKg,
    Credits availableBalance,
    PropellantQuote& quote) {
    if (quoteId.empty() || context.locationId.empty() ||
        context.celestialBody.empty())
        return Result::failure("PROPELLANT_QUOTE_CONTEXT",
            "A quote requires an ID, canonical economic location, and celestial body.");
    if (context.locationKind != PROPELLANT_LOCATION_ORBITAL_STATION &&
        context.locationKind != PROPELLANT_LOCATION_PLANETARY_SPACEPORT)
        return Result::failure("PROPELLANT_QUOTE_LOCATION",
            "The quote location must be an orbital station or planetary spaceport.");
    if (!std::isfinite(unitPriceCreditsPerKg) ||
        unitPriceCreditsPerKg <= 0.0)
        return Result::failure("PROPELLANT_QUOTE_PRICE",
            "Fuel unit price must be finite and greater than zero.");
    if (availableBalance < 0)
        return Result::failure("PROPELLANT_QUOTE_BALANCE",
            "Available company balance cannot be negative.");

    PropellantFillPlan plan;
    const Result planned = createPropellantFillPlan(
        resources, requestedMassKg, plan);
    if (!planned.ok) return planned;

    const long double rawCost =
        static_cast<long double>(requestedMassKg) *
        static_cast<long double>(unitPriceCreditsPerKg);
    if (!std::isfinite(rawCost) || rawCost <= 0.0L)
        return Result::failure("PROPELLANT_QUOTE_VALUE",
            "Calculated fuel purchase value is invalid.");
    const long double roundedCost = std::ceil(rawCost);
    if (roundedCost >
        static_cast<long double>(std::numeric_limits<Credits>::max()))
        return Result::failure("PROPELLANT_QUOTE_OVERFLOW",
            "Fuel purchase value exceeds the supported credit range.");
    const Credits totalCost = static_cast<Credits>(roundedCost);
    if (totalCost <= 0)
        return Result::failure("PROPELLANT_QUOTE_VALUE",
            "Calculated fuel purchase value must be positive.");
    if (availableBalance < totalCost)
        return Result::failure("PROPELLANT_INSUFFICIENT_FUNDS",
            "The company account does not have enough credits for this refuelling quote.");

    PropellantQuote candidate;
    candidate.quoteId = quoteId;
    candidate.context = context;
    candidate.requestedMassKg = requestedMassKg;
    candidate.unitPriceCreditsPerKg = unitPriceCreditsPerKg;
    candidate.totalCostCredits = totalCost;
    candidate.balanceAtQuote = availableBalance;
    candidate.fillPlan = plan;
    quote = candidate;
    return Result::success();
}

} // namespace oem
