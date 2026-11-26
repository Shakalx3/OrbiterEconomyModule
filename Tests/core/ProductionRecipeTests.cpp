#include "../../src/core/ProductionRecipe.h"
#include <cassert>
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
 oem::CargoServiceInventory starved("LunarFactory");
 assert(starved.setInitialStock("ore",100,2.0).ok);
 locations["LunarFactory"]=starved;
 std::vector<oem::ProductionRecipeIntent> blocked;assert(book.planDue(7300,locations,blocked).ok&&blocked.empty());
 oem::ProductionRecipe bad=r;bad.recipeId="bad";bad.inputs[1].cargoId=bad.inputs[0].cargoId;
 assert(!book.add(bad).ok);
 return 0;
}
