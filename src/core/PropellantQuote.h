#ifndef OEM_CORE_PROPELLANT_QUOTE_H
#define OEM_CORE_PROPELLANT_QUOTE_H

#include "PropellantPlan.h"
#include "Types.h"
#include <string>

namespace oem {

enum PropellantLocationKind {
    PROPELLANT_LOCATION_ORBITAL_STATION = 1,
    PROPELLANT_LOCATION_PLANETARY_SPACEPORT = 2
};

// Pricing context is copied into the quote so a later transaction can retain
// the exact location/body and unit rate shown to the player.
struct PropellantPriceContext {
    std::string locationId;
    PropellantLocationKind locationKind;
    std::string celestialBody;
    PropellantPriceContext();
};

struct PropellantQuote {
    std::string quoteId;
    PropellantPriceContext context;
    double requestedMassKg;
    double unitPriceCreditsPerKg;
    Credits totalCostCredits;
    Credits balanceAtQuote;
    PropellantFillPlan fillPlan;
    PropellantQuote();
};

// Creates a validated, immutable-by-convention purchase quote. Fuel supply is
// unlimited at eligible locations: this checks tank capacity and company funds,
// never station fuel stock. Fractional credit totals are rounded up to one
// whole credit so the ledger cannot undercharge the quoted rate.
Result createPropellantQuote(
    const std::vector<PropellantResourceState>& resources,
    const std::string& quoteId,
    const PropellantPriceContext& context,
    double requestedMassKg,
    double unitPriceCreditsPerKg,
    Credits availableBalance,
    PropellantQuote& quote);

} // namespace oem

#endif
