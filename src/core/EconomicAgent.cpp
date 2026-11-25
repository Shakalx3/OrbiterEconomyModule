#include "EconomicAgent.h"
#include <algorithm>
#include <limits>
namespace oem {
AgentCargoPolicy::AgentCargoPolicy():cargoId(),targetQuantity(0),maximumBuyPrice(0),minimumSellPrice(0){}
EconomicAgent::EconomicAgent():agentId(),displayName(),role(AGENT_SUPPLIER),homeLocationId(),operatingLocationIds(),cargoPolicies(),inventory(),balance(0),unlimitedFunds(false),activityInterval(86400),lastActivityTime(0),enabled(true){}
AgentTradeIntent::AgentTradeIntent():operationId(),agentId(),locationId(),cargoId(),side(AGENT_BUYS_FROM_LOCATION),quantity(0),unitPrice(0),totalPrice(0),simulationTime(0){}
namespace {
unsigned long hashIntent(const std::string& s){unsigned long h=2166136261UL;for(std::size_t i=0;i<s.size();++i){h^=(unsigned char)s[i];h*=16777619UL;}return h;}
bool contains(const std::vector<std::string>& v,const std::string& s){return std::find(v.begin(),v.end(),s)!=v.end();}
}
Result EconomicAgentBook::add(const EconomicAgent& a){
 if(a.agentId.empty()||a.displayName.empty()||a.homeLocationId.empty()||a.operatingLocationIds.empty()||
    a.activityInterval<=0||a.lastActivityTime<0||a.balance<0||
    a.role<AGENT_SUPPLIER||a.role>AGENT_COMPETITOR)
  return Result::failure("INVALID_ECONOMIC_AGENT","Agent identity, role, home, operating locations, non-negative balance and positive activity interval are required.");
 if(indexes_.find(a.agentId)!=indexes_.end())return Result::failure("DUPLICATE_ECONOMIC_AGENT","Agent ID is already registered.");
 if(!contains(a.operatingLocationIds,a.homeLocationId))return Result::failure("AGENT_HOME_NOT_OPERATING","Home location must be in the operating-location list.");
 for(std::size_t i=0;i<a.cargoPolicies.size();++i){const AgentCargoPolicy&p=a.cargoPolicies[i];if(p.cargoId.empty()||p.targetQuantity<=0||p.maximumBuyPrice<0||p.minimumSellPrice<0)return Result::failure("INVALID_AGENT_CARGO_POLICY","Cargo policy values must be valid and bounded.");for(std::size_t j=0;j<i;++j)if(a.cargoPolicies[j].cargoId==p.cargoId)return Result::failure("DUPLICATE_AGENT_CARGO_POLICY","Cargo policies must have unique cargo IDs.");}
 agents_.push_back(a);indexes_[a.agentId]=agents_.size()-1;return Result::success();
}
const std::vector<EconomicAgent>& EconomicAgentBook::agents()const{return agents_;}
Result EconomicAgentBook::planDueTrades(SimulationSeconds now,const CargoCatalog& catalog,const std::map<std::string,CargoServiceInventory>& locations,std::vector<AgentTradeIntent>& out)const{
 if(now<0)return Result::failure("INVALID_AGENT_TIME","Simulation time cannot be negative.");
 std::vector<AgentTradeIntent> planned;
 for(std::size_t i=0;i<agents_.size();++i){const EconomicAgent&a=agents_[i];if(!a.enabled||now<a.lastActivityTime||now-a.lastActivityTime<a.activityInterval)continue;
  for(std::size_t l=0;l<a.operatingLocationIds.size();++l){std::map<std::string,CargoServiceInventory>::const_iterator loc=locations.find(a.operatingLocationIds[l]);if(loc==locations.end())continue;
   for(std::size_t p=0;p<a.cargoPolicies.size();++p){const AgentCargoPolicy&policy=a.cargoPolicies[p];CargoDefinition def;if(!catalog.find(policy.cargoId,def))continue;
    const long long held=a.inventory.count(policy.cargoId)?a.inventory.find(policy.cargoId)->second:0;
    AgentTradeIntent intent;intent.agentId=a.agentId;intent.locationId=loc->first;intent.cargoId=policy.cargoId;intent.simulationTime=now;
    if((a.role==AGENT_CUSTOMER||a.role==AGENT_COMPETITOR)&&held<policy.targetQuantity&&loc->second.quantity(policy.cargoId)>0&&def.buyPrice<=policy.maximumBuyPrice){
      intent.side=AGENT_BUYS_FROM_LOCATION;intent.unitPrice=def.buyPrice;intent.quantity=std::min(policy.targetQuantity-held,loc->second.quantity(policy.cargoId));
      if(!a.unlimitedFunds){if(intent.unitPrice<=0||a.balance<intent.unitPrice)continue;intent.quantity=std::min(intent.quantity,a.balance/intent.unitPrice);}
    }else if((a.role==AGENT_SUPPLIER||a.role==AGENT_COMPETITOR)&&held>0&&def.sellPrice>=policy.minimumSellPrice){
      intent.side=AGENT_SELLS_TO_LOCATION;intent.unitPrice=def.sellPrice;intent.quantity=std::min(held,policy.targetQuantity);
    }else continue;
    if(intent.quantity<=0||intent.unitPrice<0||intent.quantity>std::numeric_limits<Credits>::max()/std::max<Credits>(1,intent.unitPrice))continue;
    intent.totalPrice=intent.quantity*intent.unitPrice;
    intent.operationId="agent-"+a.agentId+"-"+loc->first+"-"+policy.cargoId+"-"+std::to_string((long long)now)+"-"+std::to_string(hashIntent(a.agentId+loc->first+policy.cargoId));
    planned.push_back(intent);break;
   }if(!planned.empty()&&planned.back().agentId==a.agentId)break;
  }
 }
 out.insert(out.end(),planned.begin(),planned.end());return Result::success();
}
}