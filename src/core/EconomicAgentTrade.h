#ifndef OEM_CORE_ECONOMIC_AGENT_TRADE_H
#define OEM_CORE_ECONOMIC_AGENT_TRADE_H

#include "EconomicAgent.h"
#include "Market.h"
#include <map>
#include <set>
#include <string>
#include <vector>

namespace oem {

// Abstract stock projection for an economic location. This is not a vessel
// manifest and never represents physical cargo loaded aboard an Orbiter ship.
struct EconomicLocationStock {
    std::string locationId;
    std::map<std::string, long long> quantities;
};

enum EconomicAgentTradeSide {
    AGENT_BUYS_FROM_LOCATION = 1,
    AGENT_SELLS_TO_LOCATION = 2
};

struct EconomicAgentTrade {
    std::string operationId;
    std::string agentId;
    std::string locationId;
    std::string cargoId;
    EconomicAgentTradeSide side;
    long long quantity;
    Credits unitPrice;
    Credits totalPrice;
    SimulationSeconds simulationTime;
};

class EconomicAgentTradeEngine {
public:
    // Processes due simulation-time cycles in deterministic registry/location/
    // cargo order. maxCycles bounds catch-up after large time acceleration.
    Result advance(EconomicAgentRegistry& agents,
                   const CargoCatalog& catalog,
                   std::vector<EconomicLocationStock>& locations,
                   SimulationSeconds simulationTime,
                   std::size_t maxCycles,
                   std::vector<EconomicAgentTrade>& trades) const;
};

} // namespace oem
#endif
