#include "EconomicAgentTrade.h"
#include <limits>
#include <sstream>

namespace oem { namespace {
bool addCredits(Credits current, Credits amount, Credits& result) {
    const Credits maxValue = static_cast<Credits>(~static_cast<unsigned long long>(0) >> 1);
    if (amount < 0 || current > maxValue - amount) return false;
    result = current + amount;
    return true;
}
bool multiplyCredits(Credits price, long long quantity, Credits& total) {
    if (price < 0 || quantity < 0) return false;
    if (price != 0 && quantity > std::numeric_limits<Credits>::max() / price) return false;
    total = price * quantity;
    return true;
}
std::string operationId(const EconomicAgent& agent, const EconomicLocationStock& location,
                        const std::string& cargo, EconomicAgentTradeSide side,
                        SimulationSeconds cycleTime) {
    std::ostringstream out;
    out << "agent-trade:" << agent.agentId << ":" << location.locationId << ":"
        << cargo << ":" << static_cast<int>(side) << ":" << cycleTime;
    return out.str();
}
} // namespace

Result EconomicAgentTradeEngine::advance(EconomicAgentRegistry& agents,
                                          const CargoCatalog& catalog,
                                          std::vector<EconomicLocationStock>& locations,
                                          SimulationSeconds now,
                                          std::size_t maxCycles,
                                          std::vector<EconomicAgentTrade>& trades) const {
    if (now < 0 || maxCycles == 0)
        return Result::failure("AGENT_TRADE_TIME", "Simulation time must be non-negative and the cycle budget positive.");

    // Work on candidate state. Any validation failure leaves all caller state untouched.
    EconomicAgentRegistry candidateAgents = agents;
    std::vector<EconomicLocationStock> candidateLocations = locations;
    std::vector<EconomicAgentTrade> candidateTrades = trades;

    for (std::size_t aIndex = 0; aIndex < candidateAgents.entries().size(); ++aIndex) {
        EconomicAgent agent = candidateAgents.entries()[aIndex];
        std::size_t cycles = 0;
        while (agent.nextTradeAt <= now && cycles < maxCycles) {
            const SimulationSeconds cycleTime = agent.nextTradeAt;
            if (agent.tradeIntervalSeconds <= 0 ||
                cycleTime > std::numeric_limits<SimulationSeconds>::max() - agent.tradeIntervalSeconds)
                return Result::failure("AGENT_TRADE_SCHEDULE", "Economic agent trade schedule overflowed.");

            for (std::size_t l = 0; l < agent.operatingLocationIds.size(); ++l) {
                std::size_t locationIndex = candidateLocations.size();
                for (std::size_t k = 0; k < candidateLocations.size(); ++k)
                    if (candidateLocations[k].locationId == agent.operatingLocationIds[l]) {
                        locationIndex = k; break;
                    }
                if (locationIndex == candidateLocations.size())
                    continue; // A temporarily unavailable location is not a trade opportunity.
                EconomicLocationStock& location = candidateLocations[locationIndex];
                if (location.locationId.empty())
                    return Result::failure("AGENT_TRADE_LOCATION", "Economic location has an empty ID.");

                for (std::map<std::string, long long>::const_iterator target = agent.buyTargets.begin();
                     target != agent.buyTargets.end(); ++target) {
                    const long long held = agent.inventory[target->first];
                    if (held >= target->second) continue;
                    const long long available = location.quantities[target->first];
                    if (available <= 0) continue;
                    CargoDefinition cargo;
                    if (!catalog.find(target->first, cargo))
                        return Result::failure("AGENT_TRADE_CARGO", "Agent buy target references an unknown cargo type.");
                    const long long wanted = target->second - held;
                    long long quantity = wanted < available ? wanted : available;
                    if (cargo.buyPrice > 0) {
                        const long long affordable = agent.currentBalance / cargo.buyPrice;
                        if (quantity > affordable) quantity = affordable;
                    }
                    if (quantity <= 0) continue;
                    Credits total = 0;
                    if (!multiplyCredits(cargo.buyPrice, quantity, total))
                        return Result::failure("AGENT_TRADE_VALUE", "Agent purchase value overflowed.");
                    EconomicAgentTrade trade;
                    trade.operationId = operationId(agent, location, target->first, AGENT_BUYS_FROM_LOCATION, cycleTime);
                    trade.agentId = agent.agentId; trade.locationId = location.locationId;
                    trade.cargoId = target->first; trade.side = AGENT_BUYS_FROM_LOCATION;
                    trade.quantity = quantity; trade.unitPrice = cargo.buyPrice;
                    trade.totalPrice = total; trade.simulationTime = cycleTime;
                    agent.inventory[target->first] += quantity;
                    location.quantities[target->first] -= quantity;
                    agent.currentBalance -= total;
                    candidateTrades.push_back(trade);
                }

                for (std::map<std::string, long long>::const_iterator target = agent.sellTargets.begin();
                     target != agent.sellTargets.end(); ++target) {
                    const long long held = agent.inventory[target->first];
                    if (held <= target->second) continue; // Preserve the configured reserve.
                    CargoDefinition cargo;
                    if (!catalog.find(target->first, cargo))
                        return Result::failure("AGENT_TRADE_CARGO", "Agent sell target references an unknown cargo type.");
                    const long long quantity = held - target->second;
                    const long long stationStock = location.quantities[target->first];
                    if (stationStock > std::numeric_limits<long long>::max() - quantity)
                        return Result::failure("AGENT_TRADE_STOCK", "Economic location stock would overflow.");
                    Credits total = 0, newBalance = 0;
                    if (!multiplyCredits(cargo.sellPrice, quantity, total) ||
                        !addCredits(agent.currentBalance, total, newBalance))
                        return Result::failure("AGENT_TRADE_VALUE", "Agent sale value or balance overflowed.");
                    EconomicAgentTrade trade;
                    trade.operationId = operationId(agent, location, target->first, AGENT_SELLS_TO_LOCATION, cycleTime);
                    trade.agentId = agent.agentId; trade.locationId = location.locationId;
                    trade.cargoId = target->first; trade.side = AGENT_SELLS_TO_LOCATION;
                    trade.quantity = quantity; trade.unitPrice = cargo.sellPrice;
                    trade.totalPrice = total; trade.simulationTime = cycleTime;
                    agent.inventory[target->first] -= quantity;
                    location.quantities[target->first] += quantity;
                    agent.currentBalance = newBalance;
                    candidateTrades.push_back(trade);
                }
            }
            agent.nextTradeAt += agent.tradeIntervalSeconds;
            ++cycles;
        }
        Result updated = candidateAgents.updateState(agent);
        if (!updated.ok) return updated;
    }

    agents = candidateAgents;
    locations.swap(candidateLocations);
    trades.swap(candidateTrades);
    return Result::success();
}

} // namespace oem
