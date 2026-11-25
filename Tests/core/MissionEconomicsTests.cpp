#include "MissionEconomics.h"
#include <cassert>
#include <cmath>

int main() {
    oem::MissionEconomicsConfig config;
    oem::MissionRouteDistance shortRoute;
    shortRoute.valid=true; shortRoute.kind=oem::MISSION_ROUTE_SURFACE; shortRoute.metres=1000.0;
    oem::MissionRouteDistance longRoute=shortRoute; longRoute.metres=1000000000.0;
    const oem::MissionQuote shortQuote=oem::quoteMission(100,1,shortRoute,config);
    const oem::MissionQuote longQuote=oem::quoteMission(100,1,longRoute,config);
    assert(shortQuote.valid && longQuote.valid);
    assert(longQuote.reward>shortQuote.reward);
    assert(longQuote.allowanceSeconds>shortQuote.allowanceSeconds);
    assert(longQuote.reward<=config.maximumReward);
    assert(longQuote.allowanceSeconds<=config.maximumAllowanceSeconds);

    oem::MissionRouteDistance invalid=shortRoute; invalid.valid=false;
    assert(!oem::quoteMission(100,1,invalid,config).valid);
    assert(!oem::quoteMission(100,0,shortRoute,config).valid);
    assert(!oem::quoteMission(0,1,shortRoute,config).valid);

    config.nominalCruiseSpeedMetresPerSecond=0.0;
    assert(!oem::quoteMission(100,1,shortRoute,config).valid);
    return 0;
}
