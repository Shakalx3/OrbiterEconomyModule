#include "MissionEconomics.h"
#include <cmath>
#include <limits>

namespace oem {

MissionRouteDistance::MissionRouteDistance()
    : metres(0.0), kind(MISSION_ROUTE_ESTIMATE), valid(false) {}

MissionEconomicsConfig::MissionEconomicsConfig()
    : nominalCruiseSpeedMetresPerSecond(10000.0), deadlineMargin(1.5),
      baseAllowanceSeconds(21600), minimumAllowanceSeconds(21600),
      maximumAllowanceSeconds(31536000), rewardCreditsPerKilometre(1),
      minimumReward(100), maximumReward(500000) {}

MissionQuote::MissionQuote() : reward(0), allowanceSeconds(0), valid(false) {}

MissionQuote quoteMission(Credits cargoValue, long long quantity,
                          const MissionRouteDistance& route,
                          const MissionEconomicsConfig& config) {
    MissionQuote quote;
    if (cargoValue <= 0 || quantity <= 0 || !route.valid ||
        !std::isfinite(route.metres) || route.metres < 0.0 ||
        !std::isfinite(config.nominalCruiseSpeedMetresPerSecond) ||
        config.nominalCruiseSpeedMetresPerSecond <= 0.0 ||
        !std::isfinite(config.deadlineMargin) || config.deadlineMargin < 1.0 ||
        config.baseAllowanceSeconds < 0 ||
        config.minimumAllowanceSeconds < 0 ||
        config.maximumAllowanceSeconds < config.minimumAllowanceSeconds ||
        config.rewardCreditsPerKilometre < 0 ||
        config.minimumReward <= 0 || config.maximumReward < config.minimumReward)
        return quote;

    const long double base = static_cast<long double>(cargoValue) * quantity;
    const long double distanceKm = static_cast<long double>(route.metres) / 1000.0L;
    const long double distanceBonus = distanceKm * config.rewardCreditsPerKilometre;
    long double reward = base + distanceBonus;
    if (!std::isfinite(static_cast<double>(reward))) return quote;
    if (reward < config.minimumReward) reward = config.minimumReward;
    if (reward > config.maximumReward) reward = config.maximumReward;
    quote.reward = static_cast<Credits>(std::ceil(reward));

    const long double travelSeconds =
        static_cast<long double>(route.metres) /
        config.nominalCruiseSpeedMetresPerSecond * config.deadlineMargin;
    if (!std::isfinite(static_cast<double>(travelSeconds))) return MissionQuote();
    long double allowance = static_cast<long double>(config.baseAllowanceSeconds) + travelSeconds;
    if (allowance < config.minimumAllowanceSeconds)
        allowance = config.minimumAllowanceSeconds;
    if (allowance > config.maximumAllowanceSeconds)
        allowance = config.maximumAllowanceSeconds;
    quote.allowanceSeconds = static_cast<SimulationSeconds>(std::ceil(allowance));
    quote.valid = quote.reward > 0 && quote.allowanceSeconds >= 0;
    return quote;
}

} // namespace oem
