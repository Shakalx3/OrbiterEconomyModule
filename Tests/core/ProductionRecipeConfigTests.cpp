#include "../../src/core/ProductionRecipeConfig.h"
#include <cassert>
#include <cstdio>
#include <fstream>
int main(){
 const std::string path="OEMProductionRecipeConfigTests.cfg";std::remove(path.c_str());
 oem::ProductionRecipeBook book;oem::ProductionRecipe r;r.recipeId="water-electrolysis";r.locationId="ISS";r.cycleSeconds=86400;r.lastProductionTime=123;r.enabled=true;
 oem::ProductionMaterial in;in.cargoId="WATER";in.quantity=10;in.unitMassKg=1.0;r.inputs.push_back(in);
 oem::ProductionMaterial out;out.cargoId="OXYGEN";out.quantity=8;out.unitMassKg=1.0;r.outputs.push_back(out);
 assert(book.add(r).ok);oem::ProductionRecipeConfigStore store;assert(store.save(path,book).ok);
 oem::ProductionRecipeBook loaded;assert(store.load(path,loaded).ok);assert(loaded.recipes().size()==1);
 const oem::ProductionRecipe& got=loaded.recipes()[0];assert(got.recipeId==r.recipeId&&got.locationId==r.locationId);
 assert(got.cycleSeconds==86400&&got.lastProductionTime==123&&got.enabled);
 assert(got.inputs.size()==1&&got.inputs[0].cargoId=="WATER"&&got.inputs[0].quantity==10);
 assert(got.outputs.size()==1&&got.outputs[0].cargoId=="OXYGEN"&&got.outputs[0].quantity==8);
 {std::ofstream bad(path.c_str(),std::ios::binary|std::ios::trunc);bad<<"BROKEN\\n";}
 oem::ProductionRecipeBook unchanged;assert(unchanged.add(r).ok);assert(!store.load(path,unchanged).ok);
 assert(unchanged.recipes().size()==1&&unchanged.recipes()[0].recipeId==r.recipeId);
 std::remove(path.c_str());return 0;
}
