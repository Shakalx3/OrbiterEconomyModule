#include "../../src/core/EconomicAgentConfig.h"
#include <cassert>
#include <cstdio>
int main(){
 const std::string path="OEMEconomicAgentConfigTests.cfg";std::remove(path.c_str());
 oem::EconomicAgentBook source;oem::EconomicAgent a;a.agentId="trader-1";a.displayName="Interplanetary Trader";a.role=oem::AGENT_COMPETITOR;a.homeLocationId="Earth";a.operatingLocationIds.push_back("Earth");a.operatingLocationIds.push_back("Mars");a.balance=12000;a.activityInterval=86400;a.lastActivityTime=42;
 oem::AgentCargoPolicy p;p.cargoId="ORE";p.targetQuantity=30;p.maximumBuyPrice=500;p.minimumSellPrice=100;a.cargoPolicies.push_back(p);a.inventory["ORE"]=5;a.cargoOriginLocation["ORE"]="Earth";a.cargoUnitCost["ORE"]=120;
 assert(source.add(a).ok);oem::EconomicAgentConfigStore store;assert(store.save(path,source).ok);
 oem::EconomicAgentBook loaded;assert(store.load(path,loaded).ok);assert(loaded.agents().size()==1);
 const oem::EconomicAgent&b=loaded.agents()[0];assert(b.agentId==a.agentId&&b.balance==12000&&b.activityInterval==86400&&b.lastActivityTime==42);
 assert(b.operatingLocationIds.size()==2&&b.cargoPolicies.size()==1&&b.inventory.find("ORE")->second==5);
 assert(b.cargoOriginLocation.find("ORE")->second=="Earth"&&b.cargoUnitCost.find("ORE")->second==120);
 oem::EconomicAgentBook unchanged; oem::EconomicAgent invalid=a;invalid.agentId="";assert(!unchanged.add(invalid).ok);
 std::remove(path.c_str());return 0;
}
