#include "EconomicAgent.h"
#include "MarketPricing.h"
#include <algorithm>
#include <limits>
#include <sstream>
namespace oem {
AgentCargoPolicy::AgentCargoPolicy():cargoId(),targetQuantity(0),maximumBuyPrice(0),minimumSellPrice(0){}
EconomicAgent::EconomicAgent():agentId(),displayName(),role(AGENT_SUPPLIER),homeLocationId(),operatingLocationIds(),cargoPolicies(),inventory(),cargoOriginLocation(),cargoUnitCost(),balance(0),unlimitedFunds(false),activityInterval(86400),lastActivityTime(0),enabled(true){}
AgentTradeIntent::AgentTradeIntent():operationId(),agentId(),locationId(),cargoId(),side(AGENT_BUYS_FROM_LOCATION),quantity(0),unitPrice(0),totalPrice(0),simulationTime(0){}
namespace {
unsigned int hashIntent(const std::string& s){unsigned int h=2166136261u;for(std::size_t i=0;i<s.size();++i){h^=(unsigned char)s[i];h*=16777619u;}return h;}
bool contains(const std::vector<std::string>& v,const std::string& s){return std::find(v.begin(),v.end(),s)!=v.end();}
std::string intentSignature(const AgentTradeIntent& i){std::ostringstream o;o<<i.agentId<<'|'<<i.locationId<<'|'<<i.cargoId<<'|'<<(int)i.side<<'|'<<i.quantity<<'|'<<i.unitPrice<<'|'<<i.totalPrice<<'|'<<i.simulationTime;return o.str();}
}
Result EconomicAgentBook::add(const EconomicAgent& a){
 if(a.agentId.empty()||a.displayName.empty()||a.homeLocationId.empty()||a.operatingLocationIds.empty()||
    a.activityInterval<=0||a.lastActivityTime<0||a.balance<0||
    a.role<AGENT_SUPPLIER||a.role>AGENT_COMPETITOR)
  return Result::failure("INVALID_ECONOMIC_AGENT","Agent identity, role, home, operating locations, non-negative balance and positive activity interval are required.");
 if(indexes_.find(a.agentId)!=indexes_.end())return Result::failure("DUPLICATE_ECONOMIC_AGENT","Agent ID is already registered.");
 if(!contains(a.operatingLocationIds,a.homeLocationId))return Result::failure("AGENT_HOME_NOT_OPERATING","Home location must be in the operating-location list.");
 for(std::size_t i=0;i<a.operatingLocationIds.size();++i){if(a.operatingLocationIds[i].empty())return Result::failure("INVALID_ECONOMIC_AGENT","Operating location IDs cannot be empty.");for(std::size_t j=0;j<i;++j)if(a.operatingLocationIds[j]==a.operatingLocationIds[i])return Result::failure("DUPLICATE_AGENT_LOCATION","Operating locations must be unique.");}
 for(std::map<std::string,long long>::const_iterator i=a.inventory.begin();i!=a.inventory.end();++i)if(i->first.empty()||i->second<0)return Result::failure("INVALID_AGENT_INVENTORY","Agent inventory cargo IDs and quantities must be valid.");
 for(std::map<std::string,std::string>::const_iterator i=a.cargoOriginLocation.begin();i!=a.cargoOriginLocation.end();++i)if(a.inventory.find(i->first)==a.inventory.end()||i->second.empty()||!contains(a.operatingLocationIds,i->second))return Result::failure("INVALID_AGENT_CARGO_ORIGIN","Cargo origin must reference an inventory item and an operating location.");
 for(std::map<std::string,Credits>::const_iterator i=a.cargoUnitCost.begin();i!=a.cargoUnitCost.end();++i)if(a.inventory.find(i->first)==a.inventory.end()||i->second<0)return Result::failure("INVALID_AGENT_CARGO_COST","Cargo purchase cost must reference an inventory item and be non-negative.");
 for(std::size_t i=0;i<a.cargoPolicies.size();++i){const AgentCargoPolicy&p=a.cargoPolicies[i];if(p.cargoId.empty()||p.targetQuantity<=0||p.maximumBuyPrice<0||p.minimumSellPrice<0)return Result::failure("INVALID_AGENT_CARGO_POLICY","Cargo policy values must be valid and bounded.");for(std::size_t j=0;j<i;++j)if(a.cargoPolicies[j].cargoId==p.cargoId)return Result::failure("DUPLICATE_AGENT_CARGO_POLICY","Cargo policies must have unique cargo IDs.");}
 agents_.push_back(a);indexes_[a.agentId]=agents_.size()-1;return Result::success();
}
const std::vector<EconomicAgent>& EconomicAgentBook::agents()const{return agents_;}
Result EconomicAgentBook::planDueTrades(SimulationSeconds now,const CargoCatalog& catalog,const std::map<std::string,CargoServiceInventory>& locations,std::vector<AgentTradeIntent>& out)const{
 return planDueTrades(now,catalog,locations,std::map<std::string,std::map<std::string,SimulationSeconds> >(),out);
}
Result EconomicAgentBook::planDueTrades(SimulationSeconds now,const CargoCatalog& catalog,const std::map<std::string,CargoServiceInventory>& locations,const std::map<std::string,std::map<std::string,SimulationSeconds> >& routeTravelTimes,std::vector<AgentTradeIntent>& out)const{
 if(now<0)return Result::failure("INVALID_AGENT_TIME","Simulation time cannot be negative.");
 std::vector<AgentTradeIntent> planned;MarketPricingBook pricing;
 for(std::size_t i=0;i<agents_.size();++i){
  const EconomicAgent&a=agents_[i];
  if(!a.enabled||now<a.lastActivityTime)continue;
  bool selected=false;
  for(std::size_t p=0;p<a.cargoPolicies.size()&&!selected;++p){
   const AgentCargoPolicy&policy=a.cargoPolicies[p];CargoDefinition def;
   if(!catalog.find(policy.cargoId,def))continue;
   const std::map<std::string,long long>::const_iterator heldIt=a.inventory.find(policy.cargoId);
   const long long held=heldIt==a.inventory.end()?0:heldIt->second;
   AgentTradeIntent intent;intent.agentId=a.agentId;intent.cargoId=policy.cargoId;intent.simulationTime=now;
   Credits price=0;std::string chosenLocation;long long chosenQuantity=0;
   const bool canBuy=(a.role==AGENT_CUSTOMER||a.role==AGENT_COMPETITOR||a.role==AGENT_CARRIER);
   const bool canSell=(a.role==AGENT_SUPPLIER||a.role==AGENT_COMPETITOR||a.role==AGENT_CARRIER);
   if(held==0&&now-a.lastActivityTime<a.activityInterval)continue;
   if(held>0&&canSell){
    const std::map<std::string,std::string>::const_iterator originIt=a.cargoOriginLocation.find(policy.cargoId);
    const std::map<std::string,Credits>::const_iterator costIt=a.cargoUnitCost.find(policy.cargoId);
    const std::string origin=originIt==a.cargoOriginLocation.end()?std::string():originIt->second;
    const Credits minimum=std::max(policy.minimumSellPrice,costIt==a.cargoUnitCost.end()?Credits(0):costIt->second);
    Credits bestPrice=-1;
    for(std::size_t l=0;l<a.operatingLocationIds.size();++l){
     const std::string&locationId=a.operatingLocationIds[l];
     if(locationId==origin)continue;
     SimulationSeconds routeTime=a.activityInterval;
     if(!routeTravelTimes.empty()){
      std::map<std::string,std::map<std::string,SimulationSeconds> >::const_iterator from=routeTravelTimes.find(origin);
      if(from==routeTravelTimes.end())continue;
      std::map<std::string,SimulationSeconds>::const_iterator route=from->second.find(locationId);
      if(route==from->second.end()||route->second<0)continue;
      routeTime=std::max(routeTime,route->second);
     }
     if(now-a.lastActivityTime<routeTime)continue;
     std::map<std::string,CargoServiceInventory>::const_iterator loc=locations.find(locationId);
     if(loc==locations.end())continue;
     Credits quote=0;
     if(!pricing.unitPriceForSide(def,loc->second.quantity(policy.cargoId),MARKET_SELL_TO_STATION,quote).ok||quote<minimum)continue;
     if(quote>bestPrice){bestPrice=quote;chosenLocation=locationId;chosenQuantity=std::min(held,policy.targetQuantity);}
    }
    if(!chosenLocation.empty()){
     intent.locationId=chosenLocation;intent.side=AGENT_SELLS_TO_LOCATION;intent.unitPrice=bestPrice;intent.quantity=chosenQuantity;price=bestPrice;
    }
   }
   if(intent.quantity<=0&&held==0&&canBuy){
    Credits bestPrice=(std::numeric_limits<Credits>::max)();
    for(std::size_t l=0;l<a.operatingLocationIds.size();++l){
     const std::string&locationId=a.operatingLocationIds[l];
     std::map<std::string,CargoServiceInventory>::const_iterator loc=locations.find(locationId);
     if(loc==locations.end()||loc->second.quantity(policy.cargoId)<=0)continue;
     Credits quote=0;
     if(!pricing.unitPriceForSide(def,loc->second.quantity(policy.cargoId),MARKET_BUY_FROM_STATION,quote).ok||quote>policy.maximumBuyPrice||quote>=bestPrice)continue;
     long long quantity=std::min(policy.targetQuantity,loc->second.quantity(policy.cargoId));
     if(!a.unlimitedFunds){if(quote<=0||a.balance<quote)continue;quantity=std::min(quantity,a.balance/quote);}
     if(quantity<=0)continue;
     bestPrice=quote;chosenLocation=locationId;chosenQuantity=quantity;
    }
    if(!chosenLocation.empty()){
     intent.locationId=chosenLocation;intent.side=AGENT_BUYS_FROM_LOCATION;intent.unitPrice=bestPrice;intent.quantity=chosenQuantity;price=bestPrice;
    }
   }
   if(intent.quantity<=0)continue;
   if(intent.quantity>(std::numeric_limits<Credits>::max)()/ (intent.unitPrice>1?intent.unitPrice:1))continue;
   intent.totalPrice=intent.quantity*intent.unitPrice;
   intent.operationId="agent-"+a.agentId+"-"+intent.locationId+"-"+policy.cargoId+"-"+(intent.side==AGENT_BUYS_FROM_LOCATION?"buy-":"sell-")+std::to_string((long long)now)+"-"+std::to_string(hashIntent(a.agentId+intent.locationId+policy.cargoId));
   planned.push_back(intent);selected=true;
  }
 }
 out.insert(out.end(),planned.begin(),planned.end());return Result::success();
}
Result EconomicAgentBook::applyTrade(const AgentTradeIntent& intent,const CargoCatalog& catalog,std::map<std::string,CargoServiceInventory>& locations){
 if(intent.operationId.empty()||intent.agentId.empty()||intent.locationId.empty()||intent.cargoId.empty()||intent.quantity<=0||intent.unitPrice<0||intent.totalPrice<0||intent.simulationTime<0||(intent.side!=AGENT_BUYS_FROM_LOCATION&&intent.side!=AGENT_SELLS_TO_LOCATION))
  return Result::failure("AGENT_TRADE_INVALID","Agent trade identity, side, quantity, price and simulation time must be valid.");
 if(intent.quantity>std::numeric_limits<Credits>::max()/std::max<Credits>(1,intent.unitPrice)||intent.totalPrice!=intent.quantity*intent.unitPrice)
  return Result::failure("AGENT_TRADE_TOTAL","Agent trade total does not match quantity and unit price.");
 const std::string signature=intentSignature(intent);std::map<std::string,std::string>::const_iterator prior=appliedOperations_.find(intent.operationId);
 if(prior!=appliedOperations_.end())return prior->second==signature?Result::success():Result::failure("AGENT_TRADE_CONFLICT","Agent trade operation ID was reused with different terms.");
 std::map<std::string,std::size_t>::const_iterator ai=indexes_.find(intent.agentId);if(ai==indexes_.end())return Result::failure("AGENT_NOT_FOUND","Agent trade references an unknown economic agent.");
 std::map<std::string,CargoServiceInventory>::iterator li=locations.find(intent.locationId);if(li==locations.end())return Result::failure("ECONOMY_UNKNOWN_STATION","Agent trade references a location without inventory.");
 EconomicAgent nextAgent=agents_[ai->second];if(!nextAgent.enabled||!contains(nextAgent.operatingLocationIds,intent.locationId))return Result::failure("AGENT_LOCATION","Agent is disabled or cannot operate at this location.");
 if(intent.simulationTime<nextAgent.lastActivityTime||intent.simulationTime-nextAgent.lastActivityTime<nextAgent.activityInterval)return Result::failure("AGENT_NOT_DUE","Agent activity interval has not elapsed.");
 const AgentCargoPolicy* policy=0;for(std::size_t i=0;i<nextAgent.cargoPolicies.size();++i)if(nextAgent.cargoPolicies[i].cargoId==intent.cargoId){policy=&nextAgent.cargoPolicies[i];break;}
 if(!policy)return Result::failure("AGENT_CARGO_POLICY","Agent has no policy for this cargo.");
 CargoDefinition def;if(!catalog.find(intent.cargoId,def))return Result::failure("AGENT_CARGO_UNKNOWN","Agent trade cargo is absent from the catalogue.");
 const long long held=nextAgent.inventory.count(intent.cargoId)?nextAgent.inventory.find(intent.cargoId)->second:0;
 const long long stock=li->second.quantity(intent.cargoId);Credits expectedPrice=0;MarketPricingBook pricing;
 const MarketSide marketSide=intent.side==AGENT_BUYS_FROM_LOCATION?MARKET_BUY_FROM_STATION:MARKET_SELL_TO_STATION;
 Result priced=pricing.unitPriceForSide(def,stock,marketSide,expectedPrice);if(!priced.ok)return priced;
 if(expectedPrice!=intent.unitPrice)return Result::failure("AGENT_TRADE_PRICE","Agent trade price no longer matches the location's current market quote.");
 CargoServiceInventory nextLocation(li->second);CargoTransfer transfer;transfer.transferId="agent:"+intent.operationId;transfer.vesselId="OEM-AGENT:"+intent.agentId;transfer.locationId=intent.locationId;transfer.cargoId=intent.cargoId;transfer.quantity=intent.quantity;transfer.unitMassKg=def.unitMassKg;transfer.simulationTime=intent.simulationTime;
 if(intent.side==AGENT_BUYS_FROM_LOCATION){
  if(nextAgent.role!=AGENT_CUSTOMER&&nextAgent.role!=AGENT_COMPETITOR)return Result::failure("AGENT_ROLE","This agent role cannot buy cargo.");
  if(held>=policy->targetQuantity||intent.quantity>policy->targetQuantity-held||intent.quantity>stock||intent.unitPrice>policy->maximumBuyPrice)return Result::failure("AGENT_BUY_LIMIT","Agent buy exceeds its demand, price or available stock.");
  if(!nextAgent.unlimitedFunds&&nextAgent.balance<intent.totalPrice)return Result::failure("AGENT_FUNDS","Agent has insufficient funds for this purchase.");
  if(intent.quantity>std::numeric_limits<long long>::max()-held)return Result::failure("AGENT_INVENTORY_OVERFLOW","Agent inventory quantity would overflow.");
  transfer.direction=CARGO_LOAD;Result applied=nextLocation.apply(transfer);if(!applied.ok)return applied;
  nextAgent.inventory[intent.cargoId]=held+intent.quantity;nextAgent.cargoOriginLocation[intent.cargoId]=intent.locationId;nextAgent.cargoUnitCost[intent.cargoId]=intent.unitPrice;if(!nextAgent.unlimitedFunds)nextAgent.balance-=intent.totalPrice;
 }else{
  if(nextAgent.role!=AGENT_SUPPLIER&&nextAgent.role!=AGENT_COMPETITOR&&nextAgent.role!=AGENT_CARRIER)return Result::failure("AGENT_ROLE","This agent role cannot sell cargo.");
  if(intent.quantity>held||intent.quantity>policy->targetQuantity||intent.unitPrice<std::max(policy->minimumSellPrice,nextAgent.cargoUnitCost.count(intent.cargoId)?nextAgent.cargoUnitCost.find(intent.cargoId)->second:Credits(0))||(nextAgent.cargoOriginLocation.count(intent.cargoId)&&nextAgent.cargoOriginLocation.find(intent.cargoId)->second==intent.locationId))return Result::failure("AGENT_SELL_LIMIT","Agent sale exceeds its inventory or minimum price policy.");
  if(intent.totalPrice>std::numeric_limits<Credits>::max()-nextAgent.balance)return Result::failure("AGENT_BALANCE_OVERFLOW","Agent balance would overflow.");
  transfer.direction=CARGO_UNLOAD;Result applied=nextLocation.apply(transfer);if(!applied.ok)return applied;
  nextAgent.inventory[intent.cargoId]=held-intent.quantity;if(held==intent.quantity){nextAgent.cargoOriginLocation.erase(intent.cargoId);nextAgent.cargoUnitCost.erase(intent.cargoId);}nextAgent.balance+=intent.totalPrice;
 }
 nextAgent.lastActivityTime=intent.simulationTime;agents_[ai->second]=nextAgent;li->second=nextLocation;appliedOperations_[intent.operationId]=signature;return Result::success();
}
}
