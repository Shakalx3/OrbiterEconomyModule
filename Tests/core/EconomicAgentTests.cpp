#include "../../src/core/EconomicAgent.h"
#include "../../src/core/EconomyJournal.h"
#include <cassert>
#include <cstdio>
int main(){
 oem::CargoCatalog catalog;oem::CargoDefinition d;d.cargoId="ore";d.displayName="Ore";d.unitMassKg=1;d.buyPrice=10;d.sellPrice=7;assert(catalog.add(d).ok);
 oem::CargoServiceInventory station("MoonBase");assert(station.setInitialStock("ore",50,1).ok);
 std::map<std::string,oem::CargoServiceInventory> locations;locations["MoonBase"]=station;
 oem::EconomicAgent a;a.agentId="lunar-works";a.displayName="Lunar Works";a.role=oem::AGENT_CUSTOMER;a.homeLocationId="MoonBase";a.operatingLocationIds.push_back("MoonBase");a.balance=325;a.activityInterval=100;a.lastActivityTime=0;
 oem::AgentCargoPolicy p;p.cargoId="ore";p.targetQuantity=40;p.maximumBuyPrice=15;a.cargoPolicies.push_back(p);
 oem::EconomicAgentBook book;assert(book.add(a).ok);std::vector<oem::AgentTradeIntent> intents;
 assert(book.planDueTrades(99,catalog,locations,intents).ok&&intents.empty());
 assert(book.planDueTrades(100,catalog,locations,intents).ok&&intents.size()==1);
 assert(intents[0].quantity==25&&intents[0].unitPrice==13&&intents[0].totalPrice==325&&intents[0].side==oem::AGENT_BUYS_FROM_LOCATION);
 std::vector<oem::AgentTradeIntent> again;assert(book.planDueTrades(100,catalog,locations,again).ok&&again[0].operationId==intents[0].operationId);
 const std::string path="OEMEconomicAgentTests.journal";std::remove(path.c_str());oem::EconomyJournalStore journal;
 assert(journal.applyAgentTrade(path,intents[0],catalog,book,locations).ok);
 assert(locations["MoonBase"].quantity("ore")==25);
 assert(book.agents()[0].inventory.find("ore")->second==25&&book.agents()[0].balance==0&&book.agents()[0].lastActivityTime==100);
 assert(journal.applyAgentTrade(path,intents[0],catalog,book,locations).ok);
 assert(locations["MoonBase"].quantity("ore")==25&&book.agents()[0].inventory.find("ore")->second==25);
 std::map<std::string,oem::CargoServiceInventory> recoveredLocations;
 recoveredLocations["MoonBase"]=oem::CargoServiceInventory("MoonBase");
 assert(recoveredLocations["MoonBase"].setInitialStock("ore",50,1).ok);
 oem::EconomicAgentBook recoveredAgents;assert(recoveredAgents.add(a).ok);
 oem::Ledger ledger;std::map<std::string,oem::CargoManifest> manifests;
 assert(journal.load(path,catalog,ledger,recoveredLocations,manifests,0,0,&recoveredAgents).ok);
 assert(recoveredLocations["MoonBase"].quantity("ore")==25);
 assert(recoveredAgents.agents()[0].inventory.find("ore")->second==25);
 assert(recoveredAgents.agents()[0].balance==0&&recoveredAgents.agents()[0].lastActivityTime==100);
 assert(journal.load(path,catalog,ledger,recoveredLocations,manifests,0,0,&recoveredAgents).ok);
 assert(recoveredLocations["MoonBase"].quantity("ore")==25&&recoveredAgents.agents()[0].inventory.find("ore")->second==25);
 std::vector<oem::AgentTradeIntent> notDue;assert(recoveredAgents.planDueTrades(199,catalog,recoveredLocations,notDue).ok&&notDue.empty());
 // A competitor buys from a surplus location, then carries the cargo to a
 // different location only when the destination quote covers its purchase cost.
 oem::CargoDefinition routeCargo;routeCargo.cargoId="route-ore";routeCargo.displayName="Route Ore";routeCargo.unitMassKg=1;routeCargo.buyPrice=10;routeCargo.sellPrice=7;assert(catalog.add(routeCargo).ok);
 oem::CargoServiceInventory surplus("EarthBase");assert(surplus.setInitialStock("route-ore",200,1).ok);
 oem::CargoServiceInventory demand("MarsBase");
 std::map<std::string,oem::CargoServiceInventory> routeLocations;routeLocations["EarthBase"]=surplus;routeLocations["MarsBase"]=demand;
 oem::EconomicAgent trader;trader.agentId="route-trader";trader.displayName="Route Trader";trader.role=oem::AGENT_COMPETITOR;trader.homeLocationId="EarthBase";trader.operatingLocationIds.push_back("EarthBase");trader.operatingLocationIds.push_back("MarsBase");trader.balance=1000;trader.activityInterval=100;
 oem::AgentCargoPolicy routePolicy;routePolicy.cargoId="route-ore";routePolicy.targetQuantity=20;routePolicy.maximumBuyPrice=100;routePolicy.minimumSellPrice=0;trader.cargoPolicies.push_back(routePolicy);
 oem::EconomicAgentBook routeBook;assert(routeBook.add(trader).ok);std::vector<oem::AgentTradeIntent> routeIntents;oem::EconomyJournalStore routeJournal;const std::string routeJournalPath="OEMEconomicAgentRouteTests.journal";std::remove(routeJournalPath.c_str());
 assert(routeBook.planDueTrades(100,catalog,routeLocations,routeIntents).ok&&routeIntents.size()==1);
 assert(routeIntents[0].locationId=="EarthBase"&&routeIntents[0].side==oem::AGENT_BUYS_FROM_LOCATION);
 assert(routeJournal.applyAgentTradeBatch(routeJournalPath,routeIntents,catalog,routeBook,routeLocations).ok);
 assert(routeLocations["EarthBase"].quantity("route-ore")==180&&routeBook.agents()[0].inventory.find("route-ore")->second==20);
 std::map<std::string,std::map<std::string,oem::SimulationSeconds> > routeTimes;routeTimes["EarthBase"]["MarsBase"]=500;
 routeIntents.clear();assert(routeBook.planDueTrades(599,catalog,routeLocations,routeTimes,routeIntents,true).ok&&routeIntents.empty());
 assert(routeBook.planDueTrades(600,catalog,routeLocations,routeTimes,routeIntents,true).ok&&routeIntents.size()==1);
 assert(routeIntents[0].locationId=="MarsBase"&&routeIntents[0].side==oem::AGENT_SELLS_TO_LOCATION);
 assert(routeJournal.applyAgentTradeBatch(routeJournalPath,routeIntents,catalog,routeBook,routeLocations).ok);
 assert(routeLocations["MarsBase"].quantity("route-ore")==20&&routeBook.agents()[0].inventory.find("route-ore")->second==0);
 assert(routeBook.agents()[0].balance==1060);
 std::map<std::string,oem::CargoServiceInventory> recoveredRouteLocations;
 oem::CargoServiceInventory recoveredEarth("EarthBase");assert(recoveredEarth.setInitialStock("route-ore",200,1).ok);
 recoveredRouteLocations["EarthBase"]=recoveredEarth;recoveredRouteLocations["MarsBase"]=oem::CargoServiceInventory("MarsBase");
 oem::EconomicAgentBook recoveredRouteBook;assert(recoveredRouteBook.add(trader).ok);
 oem::Ledger routeLedger;std::map<std::string,oem::CargoManifest> routeManifests;
 assert(routeJournal.load(routeJournalPath,catalog,routeLedger,recoveredRouteLocations,routeManifests,0,0,&recoveredRouteBook).ok);
 assert(recoveredRouteLocations["EarthBase"].quantity("route-ore")==180&&recoveredRouteLocations["MarsBase"].quantity("route-ore")==20);
 assert(recoveredRouteBook.agents()[0].inventory.find("route-ore")->second==0);
 assert(recoveredRouteBook.agents()[0].balance==1060&&recoveredRouteBook.agents()[0].lastActivityTime==600);
 std::remove(routeJournalPath.c_str());
 // Agent trades are part of location history and prevent an opening-stock reseed.
 bool hasHistory=false;assert(journal.hasLocationHistory(path,"MoonBase",hasHistory).ok&&hasHistory);
 assert(journal.hasLocationCargoHistory(path,"MoonBase","ore",hasHistory).ok&&hasHistory);
 std::remove(path.c_str());
 oem::EconomicAgent bad=a;bad.agentId="bad";bad.homeLocationId="Elsewhere";assert(!book.add(bad).ok);
 return 0;
}
