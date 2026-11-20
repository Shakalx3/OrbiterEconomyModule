#include "../../src/core/EconomicLocationRegistry.h"
#include <cassert>

int main() {
    oem::EconomicLocationRegistry registry;
    assert(registry.addSpaceport("Earth", "Canaveral", -1.4, 0.5).ok);
    assert(registry.addSpaceport("Moon", "Canaveral", 0.2, -0.1).ok);
    assert(registry.addStation("ISS").ok);
    assert(registry.entries().size() == 3);

    const oem::EconomicLocation* earth = registry.find("surface:Earth/Canaveral");
    const oem::EconomicLocation* moon = registry.find("surface:Moon/Canaveral");
    const oem::EconomicLocation* iss = registry.find("station:ISS");
    assert(earth && moon && iss);
    assert(earth->storageId == "Canaveral");
    assert(moon->storageId == "Canaveral");
    assert(iss->storageId == "ISS");
    assert(earth->kind == oem::ECONOMIC_LOCATION_SPACEPORT);
    assert(iss->kind == oem::ECONOMIC_LOCATION_STATION);
    assert(earth->celestialBody == "Earth");
    assert(!registry.addSpaceport("Earth", "Canaveral", 0.0, 0.0).ok);
    assert(!registry.addSpaceport("Mars", "Bad", 0.0, 2.0).ok);
    assert(!registry.addStation("").ok);
    assert(registry.findStorageId("ISS", oem::ECONOMIC_LOCATION_STATION) == iss);
    assert(registry.findStorageId("Canaveral", oem::ECONOMIC_LOCATION_SPACEPORT) == 0);
    assert(registry.findStorageId("ISS", oem::ECONOMIC_LOCATION_SPACEPORT) == 0);
    return 0;
}
