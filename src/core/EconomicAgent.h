#ifndef OEM_CORE_ECONOMIC_AGENT_H
#define OEM_CORE_ECONOMIC_AGENT_H

#include "Types.h"
#include <map>
#include <string>
#include <vector>

namespace oem {

enum EconomicAgentRole {
    ECONOMIC_AGENT_SUPPLIER = 1,
    ECONOMIC_AGENT_CUSTOMER = 2,
    ECONOMIC_AGENT_CARRIER = 3,
    ECONOMIC_AGENT_COMPETITOR = 4
};

// Abstract organizations are economy-only entities. They are never Orbiter vessels.
struct EconomicAgent {
    std::string agentId;
    std::string displayName;
    EconomicAgentRole role;
    std::string homeLocationId;
    std::vector<std::string> operatingLocationIds;
    Credits openingBalance;
    Credits currentBalance;
    SimulationSeconds tradeIntervalSeconds;
    SimulationSeconds nextTradeAt;
    std::map<std::string, long long> inventory;
    std::map<std::string, long long> buyTargets;
    std::map<std::string, long long> sellTargets;
    EconomicAgent();
};

class EconomicAgentRegistry {
public:
    Result add(const EconomicAgent& agent);
    bool find(const std::string& agentId, EconomicAgent& agent) const;
    Result updateState(const EconomicAgent& agent);
    const std::vector<EconomicAgent>& entries() const;
    void clear();
private:
    std::map<std::string, std::size_t> indexes_;
    std::vector<EconomicAgent> entries_;
};

class EconomicAgentConfigStore {
public:
    Result load(const std::string& path, EconomicAgentRegistry& registry) const;
    Result save(const std::string& path, const EconomicAgentRegistry& registry) const;
};

} // namespace oem
#endif
