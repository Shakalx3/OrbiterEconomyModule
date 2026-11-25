#include "../../src/core/EconomicAgent.h"
#include <cassert>
int main(){
 oem::CargoCatalog catalog;oem::CargoDefinition d;d.cargoId="ore";d.displayName="Ore";d.unitMassKg=1;d.buyPrice=10;d.sellPrice=7;assert(catalog.add(d).ok);
 oem::CargoServiceInventory station("MoonBase");assert(station.setInitialStock("ore",50,1).ok);
 std::map<std::string,oem::CargoServiceInventory> locations;locations["MoonBase"]=station;
 oem::EconomicAgent a;a.agentId="lunar-works";a.displayName="Lunar Works";a.role=oem::AGENT_CUSTOMER;a.homeLocationId="MoonBase";a.operatingLocationIds.push_back("MoonBase");a.balance=250;a.activityInterval=100;a.lastActivityTime=0;
 oem::AgentCargoPolicy p;p.cargoId="ore";p.targetQuantity=40;p.maximumBuyPrice=12;a.cargoPolicies.push_back(p);
 oem::EconomicAgentBook book;assert(book.add(a).ok);std::vector<oem::AgentTradeIntent> intents;
 assert(book.planDueTrades(99,catalog,locations,intents).ok&&intents.empty());
 assert(book.planDueTrades(100,catalog,locations,intents).ok&&intents.size()==1);
 assert(intents[0].quantity==25&&intents[0].totalPrice==250&&intents[0].side==oem::AGENT_BUYS_FROM_LOCATION);
 std::vector<oem::AgentTradeIntent> again;assert(book.planDueTrades(100,catalog,locations,again).ok&&again[0].operationId==intents[0].operationId);
 oem::EconomicAgent bad=a;bad.agentId="bad";bad.homeLocationId="Elsewhere";assert(!book.add(bad).ok);
 return 0;
}