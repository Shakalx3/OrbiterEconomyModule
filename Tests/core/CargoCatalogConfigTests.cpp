#include "../../src/core/CargoCatalogConfig.h"
#include <cassert>
#include <cstdio>
#include <fstream>
int main(){const char*p="OEMCargoCatalogConfigTests.cfg";std::remove(p);oem::CargoCatalog c;assert(!oem::CargoCatalogConfigStore().load(p,c).ok);{std::ofstream o(p);o<<"OEM_CARGO_CATALOG|1\nC|6F787967656E|4F787967656E|10.0|125|80\n";}assert(oem::CargoCatalogConfigStore().load(p,c).ok);oem::CargoDefinition d;assert(c.find("oxygen",d)&&d.displayName=="Oxygen"&&d.unitMassKg==10.0&&d.buyPrice==125&&d.sellPrice==80);{std::ofstream o(p);o<<"OEM_CARGO_CATALOG|1\nC|6F787967656E|4F787967656E|10.0|-1|80\n";}oem::CargoCatalog keep;assert(keep.add(d).ok);assert(!oem::CargoCatalogConfigStore().load(p,keep).ok);assert(keep.find("oxygen",d));std::remove(p);return 0;}
