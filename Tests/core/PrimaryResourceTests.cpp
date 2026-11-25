#include "../../src/core/PrimaryResource.h"
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
 oem::PrimaryResourceSource invalid=source;invalid.sourceId="bad";invalid.maximumStock=0;assert(!book.add(invalid).ok);
 return 0;
}