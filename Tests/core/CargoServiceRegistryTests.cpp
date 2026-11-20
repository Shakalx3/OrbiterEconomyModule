#include "../../src/core/CargoServiceRegistry.h"
#include <cassert>
#include <cstdio>
#include <fstream>
#include <string>

int main() {
    const std::string path="oem_cargo_services_test.cfg";
    std::remove(path.c_str());std::remove((path+".tmp").c_str());
    oem::CargoServiceRegistry registry;
    assert(registry.load(path).ok);
    assert(registry.size()==0);
    assert(!registry.registerService(""));
    assert(registry.registerService("Cargo Ship|01"));
    assert(registry.registerService("ISS"));
    assert(!registry.registerService("ISS"));
    assert(registry.size()==2);
    assert(registry.isRegistered("Cargo Ship|01"));
    assert(registry.save(path).ok);

    oem::CargoServiceRegistry restored;
    assert(restored.load(path).ok);
    assert(restored.size()==2);
    assert(restored.isRegistered("Cargo Ship|01"));
    assert(restored.isRegistered("ISS"));
    assert(restored.unregisterService("ISS"));
    assert(!restored.isRegistered("ISS"));
    assert(restored.save(path).ok);

    // Invalid input must not partially replace the active registry.
    { std::ofstream out(path.c_str(),std::ios::binary|std::ios::trunc);
      out<<"OEM_CARGO_SERVICES|1\nS|GG\n"; }
    assert(!restored.load(path).ok);
    assert(restored.size()==1&&restored.isRegistered("Cargo Ship|01"));

    std::remove(path.c_str());std::remove((path+".tmp").c_str());
    return 0;
}
