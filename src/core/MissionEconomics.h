#ifndef OEM_CORE_MISSION_ECONOMICS_H
#define OEM_CORE_MISSION_ECONOMICS_H

#include "Types.h"

namespace oem {

enum MissionRouteKind {
    MISSION_ROUTE_SURFACE = 1,
    MISSION_ROUTE_ESTIMATE = 2
};

struct MissionRouteDistance {
    double metres;
    MissionRouteKind kind;
    bool valid;
    MissionRouteDistance();
};

struct MissionEconomicsConfig {
    double nominalCruiseSpeedMetresPerSecond;
    double deadlineMargin;
    SimulationSeconds baseAllowanceSeconds;
    SimulationSeconds minimumAllowanceSeconds;
    SimulationSeconds maximumAllowanceSeconds;
    Credits rewardCreditsPerKilometre;
    Credits minimumReward;
    Credits maximumReward;
    MissionEconomicsConfig();
};

struct MissionQuote {
    Credits reward;
    SimulationSeconds allowanceSeconds;
    bool valid;
    MissionQuote();
};

MissionQuote quoteMission(Credits cargoValue, long long quantity,
                          const MissionRouteDistance& route,
                          const MissionEconomicsConfig& config);

} // namespace oem
#endif
