#include "../../src/core/CargoServiceInventoryConfig.h"
#include <cassert>
#include <cstdio>
#include <fstream>
int main(){
 const std::string path="OEMCargoStockTests.cfg";std::remove(path.c_str());
 oem::CargoServiceInventory source("ISS");assert(source.setInitialStock("oxygen",17,12.5).ok);assert(source.setInitialStock("fuel",0,1.25).ok);
 oem::CargoServiceInventoryConfigStore store;assert(store.save(path,source).ok);
 oem::CargoServiceInventory loaded("ISS");assert(store.load(path,"ISS",loaded).ok);
 assert(loaded.quantity("oxygen")==17&&loaded.quantity("fuel")==0);
 oem::CargoServiceInventory wrong("MoonBase");assert(!store.load(path,"MoonBase",wrong).ok);
 {std::ofstream out(path.c_str(),std::ios::app|std::ios::binary);out<<"BAD|record\n";}
 oem::CargoServiceInventory unchanged("ISS");assert(unchanged.setInitialStock("oxygen",4,12.5).ok);
 assert(!store.load(path,"ISS",unchanged).ok&&unchanged.quantity("oxygen")==4);
 std::remove(path.c_str());oem::CargoServiceInventory absent("ISS");assert(store.load(path,"ISS",absent).ok&&absent.lines().empty());
 return 0;
}
