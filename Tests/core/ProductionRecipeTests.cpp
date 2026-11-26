#include "../../src/core/ProductionRecipe.h"
#include "../../src/core/EconomyJournal.h"
#include <cassert>
#include <cstdio>
#include <map>
#include <string>
#undef assert
#define assert(expression) do { if (!(expression)) return __LINE__; } while (0)
int main(){
 oem::CargoServiceInventory stock("LunarFactory");
 assert(stock.setInitialStock("ore",100,2.0).ok);
 assert(stock.setInitialStock("water",40,1.0).ok);
 oem::ProductionRecipe r;r.recipeId="oxygen-electrolysis";r.locationId="LunarFactory";
 r.cycleSeconds=3600;r.lastProductionTime=100;
 oem::ProductionMaterial ore;ore.cargoId="ore";ore.quantity=10;ore.unitMassKg=2.0;r.inputs.push_back(ore);
 oem::ProductionMaterial water;water.cargoId="water";water.quantity=5;water.unitMassKg=1.0;r.inputs.push_back(water);
 oem::ProductionMaterial oxygen;oxygen.cargoId="oxygen";oxygen.quantity=8;oxygen.unitMassKg=1.0;r.outputs.push_back(oxygen);
 oem::ProductionRecipeBook book;assert(book.add(r).ok);
 std::map<std::string,oem::CargoServiceInventory> locations;locations["LunarFactory"]=stock;
 std::vector<oem::ProductionRecipeIntent> due;
 assert(book.planDue(3699,locations,due).ok&&due.empty());
 assert(book.planDue(3700,locations,due).ok&&due.size()==1);
 assert(due[0].inputs.size()==2&&due[0].outputs.size()==1&&due[0].operationId=="recipe-oxygen-electrolysis-3700");
 assert(book.markCommitted(r.recipeId,3700).ok);
 std::vector<oem::ProductionRecipeIntent> again;assert(book.planDue(3700,locations,again).ok&&again.empty());
 oem::CargoServiceInventory starved("LunarFactory");assert(starved.setInitialStock("ore",100,2.0).ok);
 locations["LunarFactory"]=starved;std::vector<oem::ProductionRecipeIntent> blocked;
 assert(book.planDue(7300,locations,blocked).ok&&blocked.empty());

 // Calendar scheduling is tested with a stocked inventory, independently
 // from the preceding starvation scenario.
 std::map<std::string,oem::CargoServiceInventory> calendarInventories;
 calendarInventories["LunarFactory"]=stock;

 // A newly loaded daily recipe is anchored to the Orbiter day boundary.
 oem::ProductionRecipe daily=r;daily.recipeId="daily-electrolysis";daily.cycleSeconds=86400;daily.lastProductionTime=0;
 oem::ProductionRecipeBook calendarBook;assert(calendarBook.add(daily).ok);
 bool clockInitialized=false;assert(calendarBook.initializeUnsetTimes(100000,clockInitialized).ok&&clockInitialized);
 assert(calendarBook.recipes()[0].lastProductionTime==86400);
 std::vector<oem::ProductionRecipeIntent> calendarDue;
 assert(calendarBook.planDue(172799,calendarInventories,calendarDue).ok&&calendarDue.empty());
 assert(calendarBook.planDue(172800,calendarInventories,calendarDue).ok&&calendarDue.size()==1);
 assert(calendarDue[0].simulationTime==172800);
 assert(calendarDue[0].operationId=="recipe-daily-electrolysis-172800");
 assert(calendarBook.markCommitted("daily-electrolysis",172800).ok);
 calendarDue.clear();assert(calendarBook.planDue(172800,locations,calendarDue).ok&&calendarDue.empty());
 oem::ProductionRecipe legacyDaily=daily;legacyDaily.recipeId="legacy-daily";legacyDaily.lastProductionTime=100000;
 oem::ProductionRecipeBook migratedBook;assert(migratedBook.add(legacyDaily).ok);
 clockInitialized=false;assert(migratedBook.initializeUnsetTimes(200000,clockInitialized).ok&&clockInitialized);
 assert(migratedBook.recipes()[0].lastProductionTime==172800);
 oem::ProductionRecipe bad=r;bad.recipeId="bad";bad.inputs[1].cargoId=bad.inputs[0].cargoId;assert(!book.add(bad).ok);

 std::fprintf(stderr,"checkpoint: journal section\\n");
 // A recipe is committed as one durable journal operation: all inputs are
 // consumed and all outputs created, or the live inventory remains unchanged.
 const std::string path="OEMProductionRecipeTests.journal";std::remove(path.c_str());
 std::map<std::string,oem::CargoServiceInventory> live;
 live["LunarFactory"]=oem::CargoServiceInventory("LunarFactory");
 assert(live["LunarFactory"].setInitialStock("ore",100,2.0).ok);
 assert(live["LunarFactory"].setInitialStock("water",40,1.0).ok);
 oem::ProductionRecipeIntent intent=due[0];
 oem::EconomyJournalStore journal;
 assert(journal.applyProductionRecipe(path,intent,live).ok);
 assert(live["LunarFactory"].quantity("ore")==90);
 assert(live["LunarFactory"].quantity("water")==35);
 assert(live["LunarFactory"].quantity("oxygen")==8);
 bool history=false;
 assert(journal.hasLocationCargoHistory(path,"LunarFactory","water",history).ok&&history);
 assert(journal.hasLocationCargoHistory(path,"LunarFactory","oxygen",history).ok&&history);
 assert(journal.hasLocationCargoHistory(path,"LunarFactory","food",history).ok&&!history);
 assert(journal.applyProductionRecipe(path,intent,live).ok);
 assert(live["LunarFactory"].quantity("ore")==90);
 assert(live["LunarFactory"].quantity("water")==35);
 assert(live["LunarFactory"].quantity("oxygen")==8);

 std::fprintf(stderr,"checkpoint: replay section\\n");
 // Replay from the same opening inventory restores the exact production result.
 std::map<std::string,oem::CargoServiceInventory> replay;
 replay["LunarFactory"]=oem::CargoServiceInventory("LunarFactory");
 assert(replay["LunarFactory"].setInitialStock("ore",100,2.0).ok);
 assert(replay["LunarFactory"].setInitialStock("water",40,1.0).ok);
 oem::CargoCatalog catalog;oem::Ledger ledger;
 std::map<std::string,oem::CargoManifest> manifests;
 assert(journal.load(path,catalog,ledger,replay,manifests).ok);
 assert(replay["LunarFactory"].quantity("ore")==90);
 assert(replay["LunarFactory"].quantity("water")==35);
 assert(replay["LunarFactory"].quantity("oxygen")==8);
 std::fprintf(stderr,"checkpoint: recover section\\n");
 oem::ProductionRecipeBook recoveredBook; oem::ProductionRecipe recoveryRecipe=r;
 recoveryRecipe.lastProductionTime=100; assert(recoveredBook.add(recoveryRecipe).ok);
 assert(journal.recoverProductionRecipeTimes(path,recoveredBook).ok);
 assert(recoveredBook.recipes()[0].lastProductionTime==3700);

 std::fprintf(stderr,"checkpoint: poor section\\n");
 // Insufficient input cannot partially consume another input or append a record.
 const std::string poorPath="OEMProductionRecipePoorTests.journal";std::remove(poorPath.c_str());
 std::map<std::string,oem::CargoServiceInventory> poor;
 poor["LunarFactory"]=oem::CargoServiceInventory("LunarFactory");
 assert(poor["LunarFactory"].setInitialStock("ore",100,2.0).ok);
 assert(poor["LunarFactory"].setInitialStock("water",2,1.0).ok);
 assert(!journal.applyProductionRecipe(poorPath,intent,poor).ok);
 assert(poor["LunarFactory"].quantity("ore")==100&&poor["LunarFactory"].quantity("water")==2);
 std::remove(path.c_str());std::remove(poorPath.c_str());
 return 0;
}
