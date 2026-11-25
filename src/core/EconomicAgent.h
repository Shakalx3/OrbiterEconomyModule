#ifndef OEM_CORE_ECONOMIC_AGENT_H
#define OEM_CORE_ECONOMIC_AGENT_H
#include "Market.h"
#include <map>
#include <string>
#include <vector>
namespace oem {
enum EconomicAgentRole { AGENT_SUPPLIER=1, AGENT_CUSTOMER=2, AGENT_CARRIER=3, AGENT_COMPETITOR=4 };
struct AgentCargoPolicy {
    std::string cargoId;
    long long targetQuantity;
    Credits maximumBuyPrice;
    Credits minimumSellPrice;
    AgentCargoPolicy();
};
struct EconomicAgent {
    std::string agentId;
    std::string displayName;
    EconomicAgentRole role;
    std::string homeLocationId;
    std::vector<std::string> operatingLocationIds;
    std::vector<AgentCargoPolicy> cargoPolicies;
    std::map<std::string,long long> inventory;
    Credits balance;
    bool unlimitedFunds;
    SimulationSeconds activityInterval;
    SimulationSeconds lastActivityTime;
    bool enabled;
    EconomicAgent();
};
enum AgentTradeSide { AGENT_BUYS_FROM_LOCATION=1, AGENT_SELLS_TO_LOCATION=2 };
struct AgentTradeIntent {
    std::string operationId;
    std::string agentId;
    std::string locationId;
    std::string cargoId;
    AgentTradeSide side;
    long long quantity;
    Credits unitPrice;
    Credits totalPrice;
    SimulationSeconds simulationTime;
    AgentTradeIntent();
};
class EconomicAgentBook {
public:
    Result add(const EconomicAgent& agent);
    const std::vector<EconomicAgent>& agents() const;
    // Creates bounded, deterministic trade intents only. It does not mutate stock,
    // balances or manifests; callers must durably commit an intent before applying it.
    Result planDueTrades(SimulationSeconds now, const CargoCatalog& catalog,
        const std::map<std::string,CargoServiceInventory>& locations,
        std::vector<AgentTradeIntent>& intents) const;
private:
    std::map<std::string,std::size_t> indexes_;
    std::vector<EconomicAgent> agents_;
};
}
#endif