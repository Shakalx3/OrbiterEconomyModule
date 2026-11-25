#include "../../src/core/MissionIssuerRegistry.h"
#include <cassert>
#include <cstdio>
#include <fstream>
#include <string>

int main() {
    const std::string path="OEMMissionIssuerRegistryTests.cfg";
    std::remove(path.c_str());
    oem::MissionIssuerRegistry registry;
    assert(registry.loadOrCreate(path).ok);
    assert(registry.issuerForLocation("surface:Earth/Canaveral")=="issuer:surface:Earth/Canaveral");
    assert(registry.setIssuerForLocation("surface:Earth/Canaveral","issuer:NASA").ok);
    assert(registry.setIssuerForLocation("station:ISS","issuer:NASA").ok);
    assert(registry.setIssuerForLocation("surface:Mars/Olympus","issuer:ARES_CORP").ok);
    assert(registry.save(path).ok);

    oem::MissionIssuerRegistry recovered;
    assert(recovered.loadOrCreate(path).ok);
    assert(recovered.issuerForLocation("surface:Earth/Canaveral")=="issuer:NASA");
    assert(recovered.issuerForLocation("station:ISS")=="issuer:NASA");
    assert(recovered.issuerForLocation("surface:Mars/Olympus")=="issuer:ARES_CORP");
    assert(recovered.issuerForLocation("surface:Moon/Tranquility")=="issuer:surface:Moon/Tranquility");
    assert(recovered.mappings().size()==3);

    {
        std::ofstream bad(path.c_str(),std::ios::binary|std::ios::trunc);
        bad << "OEM_MISSION_ISSUERS|1\nL|4142|\n";
    }
    oem::MissionIssuerRegistry invalid;
    assert(!invalid.loadOrCreate(path).ok);
    std::remove(path.c_str());
    return 0;
}
