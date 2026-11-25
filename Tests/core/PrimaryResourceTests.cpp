#include "../../src/core/PrimaryResource.h"
#include "../../src/core/EconomyJournal.h"
#include "../../src/core/PrimaryResourceConfig.h"
#include <cstdio>
#include <cassert>
int main(){
 oem::CargoServiceInventory stock("MoonBase");
 oem::StarterStockProfile profile;profile.profileId="lunar-mining";oem::StarterStockLine ore;ore.cargoId="ore";ore.quantity=120;ore.unitMassKg=1.5;profile.lines.push_back(ore);
 assert(oem::seedStarterStock(profile,stock).ok);assert(stock.quantity("ore")==120);
 assert(!oem::seedStarterStock(profile,stock).ok);
 oem::PrimaryResourceSource source;source.sourceId="lunar-ore-vein";source.locationId="MoonBase";source.cargoId="ore";source.unitMassKg=1.5;source.unitsPerCycle=20;source.cycleSeconds=86400;source.maximumStock=150;
 oem::PrimaryResourceBook book;assert(book.add(source).ok);std::map<std::string,oem::CargoServiceInventory> locations;locations["MoonBase"]=stock;
 std::vector<oem::PrimaryResourceIntent> intents;assert(book.planDue(86399,locations,intents).ok&&intents.empty());
 assert(book.planDue(86400,locations,intents).ok&&intents.size()==1&&intents[0].quantity==20);
 assert(book.markCommitted(source.sourceId,86400).ok);std::vector<oem::PrimaryResourceIntent> again;assert(book.planDue(86400,locations,again).ok&&again.empty());

 // Production is committed durably and replayed exactly once from the global journal.
 const std::string journalPath="OEMPrimaryResourceTests.journal";std::remove(journalPath.c_str());
 oem::EconomyJournalStore journal;
 assert(journal.applyPrimaryProduction(journalPath,intents[0].operationId,source.sourceId,
     source.locationId,source.cargoId,intents[0].quantity,intents[0].unitMassKg,
     intents[0].simulationTime,locations).ok);
 assert(locations["MoonBase"].quantity("ore")==140);
 assert(journal.applyPrimaryProduction(journalPath,intents[0].operationId,source.sourceId,
     source.locationId,source.cargoId,intents[0].quantity,intents[0].unitMassKg,
     intents[0].simulationTime,locations).ok);
 assert(locations["MoonBase"].quantity("ore")==140);
 std::map<std::string,oem::CargoServiceInventory> recovered;
 recovered["MoonBase"]=oem::CargoServiceInventory("MoonBase");
 assert(recovered["MoonBase"].setInitialStock("ore",120,1.5).ok);
 oem::CargoCatalog catalog;oem::Ledger ledger;std::map<std::string,oem::CargoManifest> manifests;
 assert(journal.load(journalPath,catalog,ledger,recovered,manifests).ok);
 assert(recovered["MoonBase"].quantity("ore")==140);
 assert(journal.load(journalPath,catalog,ledger,recovered,manifests).ok);
 assert(recovered["MoonBase"].quantity("ore")==140);
 std::remove(journalPath.c_str());

 // A crash after journal commit but before saving the source config must not
 // allow the same production cycle to be scheduled again after restart.
 oem::PrimaryResourceBook staleBook;assert(staleBook.add(source).ok);
 assert(staleBook.sources()[0].lastProductionTime==0);
 assert(journal.recoverPrimaryProductionTimes(journalPath,staleBook).ok);
 assert(staleBook.sources()[0].lastProductionTime==86400);
 std::vector<oem::PrimaryResourceIntent> afterRecovery;
 assert(staleBook.planDue(86400,locations,afterRecovery).ok&&afterRecovery.empty());
 oem::PrimaryResourceBook changedBook;
 oem::PrimaryResourceSource changed=source;changed.cargoId="water";
 assert(changedBook.add(changed).ok);
 assert(!journal.recoverPrimaryProductionTimes(journalPath,changedBook).ok);

 // Versioned source definitions and scheduling state survive a config round trip.
 const std::string configPath="OEMPrimaryResourceTests.cfg";std::remove(configPath.c_str());
 oem::PrimaryResourceBook configBook;assert(configBook.add(source).ok);
 oem::PrimaryResourceConfigStore configStore;assert(configStore.save(configPath,configBook).ok);
 oem::PrimaryResourceBook restoredBook;assert(configStore.load(configPath,restoredBook).ok);
 assert(restoredBook.sources().size()==1);
 assert(restoredBook.sources()[0].sourceId==source.sourceId);
 assert(restoredBook.sources()[0].lastProductionTime==source.lastProductionTime);
 std::remove(configPath.c_str());
 oem::PrimaryResourceSource invalid=source;invalid.sourceId="bad";invalid.maximumStock=0;assert(!book.add(invalid).ok);
 return 0;
}