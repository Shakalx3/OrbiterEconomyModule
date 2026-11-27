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
 // Agent trades are part of location history and prevent an opening-stock reseed.
 bool hasHistory=false;assert(journal.hasLocationHistory(path,"MoonBase",hasHistory).ok&&hasHistory);
 assert(journal.hasLocationCargoHistory(path,"MoonBase","ore",hasHistory).ok&&hasHistory);
 std::remove(path.c_str());
 oem::EconomicAgent bad=a;bad.agentId="bad";bad.homeLocationId="Elsewhere";assert(!book.add(bad).ok);
 return 0;
}
