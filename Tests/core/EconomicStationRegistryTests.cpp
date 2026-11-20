#include "../../src/core/EconomicStationRegistry.h"
#include <cassert>
#include <cstdio>
#include <fstream>
#include <string>

int main() {
    const std::string path="oem_economic_stations_test.registry";
    std::remove(path.c_str()); std::remove((path+".tmp").c_str());
    oem::EconomicStationRegistry stations;
    assert(stations.load(path).ok && stations.size()==0);
    assert(!stations.registerStation(""));
    assert(stations.registerStation("ISS"));
    assert(stations.registerStation("Cargo Ship|01"));
    assert(!stations.registerStation("ISS"));
    assert(stations.isStation("ISS") && stations.size()==2);
    assert(stations.save(path).ok);
    oem::EconomicStationRegistry restored;
    assert(restored.load(path).ok && restored.size()==2);
    assert(restored.isStation("ISS") && restored.isStation("Cargo Ship|01"));
    assert(restored.unregisterStation("ISS") && !restored.isStation("ISS"));
    assert(restored.save(path).ok);
    { std::ofstream out(path.c_str(),std::ios::binary|std::ios::trunc); out<<"OEM_ECONOMIC_STATIONS|1\nS|GG\n"; }
    assert(!restored.load(path).ok);
    assert(restored.size()==1 && restored.isStation("Cargo Ship|01"));
    { std::ofstream out(path.c_str(),std::ios::binary|std::ios::trunc);
      const unsigned char bom[]={0xEF,0xBB,0xBF}; out.write(reinterpret_cast<const char*>(bom),3);
      out<<"OEM_ECONOMIC_STATIONS|1\r\nS|495353\r\n"; }
    assert(restored.load(path).ok && restored.isStation("ISS"));
    restored.clear(); assert(restored.size()==0);
    std::remove(path.c_str()); std::remove((path+".tmp").c_str());
    return 0;
}
