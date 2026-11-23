#include "../../src/core/PropellantQuote.h"
#include <cassert>
#include <cmath>
#include <limits>

int main() {
    std::vector<oem::PropellantResourceState> tanks;
    tanks.push_back(oem::PropellantResourceState(20.0, 100.0));
    tanks.push_back(oem::PropellantResourceState(5.0, 25.0));

    oem::PropellantPriceContext context;
    context.locationId = "station:ISS";
    context.locationKind = oem::PROPELLANT_LOCATION_ORBITAL_STATION;
    context.celestialBody = "Earth";

    oem::PropellantQuote quote;
    assert(oem::createPropellantQuote(
        tanks, "REFUEL-001", context, 50.0, 2.5, 1000, quote).ok);
    assert(quote.quoteId == "REFUEL-001");
    assert(quote.context.locationId == "station:ISS");
    assert(quote.context.celestialBody == "Earth");
    assert(std::fabs(quote.requestedMassKg - 50.0) < 1e-9);
    assert(std::fabs(quote.unitPriceCreditsPerKg - 2.5) < 1e-9);
    assert(quote.totalCostCredits == 125);
    assert(quote.balanceAtQuote == 1000);
    assert(quote.fillPlan.targetMassKg.size() == 2);
    assert(std::fabs(quote.fillPlan.plannedMassKg - 50.0) < 1e-9);

    // Fractional credit totals are rounded upward, never truncated.
    assert(oem::createPropellantQuote(
        tanks, "REFUEL-002", context, 0.1, 2.5, 1, quote).ok);
    assert(quote.totalCostCredits == 1);

    const std::string previousId = quote.quoteId;
    assert(!oem::createPropellantQuote(
        tanks, "REFUEL-003", context, 50.0, 2.5, 124, quote).ok);
    assert(quote.quoteId == previousId); // Failed quote does not overwrite output.
    assert(!oem::createPropellantQuote(
        tanks, "REFUEL-004", context, 151.0, 1.0, 1000, quote).ok);
    assert(!oem::createPropellantQuote(
        tanks, "REFUEL-005", context, 1.0, 0.0, 1000, quote).ok);
    assert(!oem::createPropellantQuote(
        tanks, "REFUEL-006", context, 1.0,
        std::numeric_limits<double>::infinity(), 1000, quote).ok);
    assert(!oem::createPropellantQuote(
        tanks, "REFUEL-007", context, 1.0, 1.0, -1, quote).ok);

    oem::PropellantPriceContext missingBody = context;
    missingBody.celestialBody.clear();
    assert(!oem::createPropellantQuote(
        tanks, "REFUEL-008", missingBody, 1.0, 1.0, 1000, quote).ok);

    oem::PropellantPriceContext invalidLocation = context;
    invalidLocation.locationKind = static_cast<oem::PropellantLocationKind>(99);
    assert(!oem::createPropellantQuote(
        tanks, "REFUEL-009", invalidLocation, 1.0, 1.0, 1000, quote).ok);

    std::vector<oem::PropellantResourceState> tiny;
    tiny.push_back(oem::PropellantResourceState(0.0, 1.0));
    assert(!oem::createPropellantQuote(
        tiny, "REFUEL-010", context, 1.0e100, 1.0e100,
        std::numeric_limits<oem::Credits>::max(), quote).ok);
    return 0;
}
