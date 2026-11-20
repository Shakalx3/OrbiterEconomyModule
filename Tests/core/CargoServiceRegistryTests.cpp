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
    restored.clear();
    assert(restored.size()==0&&!restored.isRegistered("Cargo Ship|01"));

    // UTF-8 BOM + CRLF should be accepted (common Windows editor output).
    { std::ofstream out(path.c_str(),std::ios::binary|std::ios::trunc);
      const unsigned char bom[]={0xEF,0xBB,0xBF};
      out.write(reinterpret_cast<const char*>(bom),3);
      out<<"OEM_CARGO_SERVICES|1\r\nS|495353\r\n"; }
    assert(restored.load(path).ok);
    assert(restored.size()==1&&restored.isRegistered("ISS"));

    // UTF-16LE BOM should also be accepted.
    { std::ofstream out(path.c_str(),std::ios::binary|std::ios::trunc);
      const unsigned char bytes[]={0xFF,0xFE,
        'O',0,'E',0,'M',0,'_',0,'C',0,'A',0,'R',0,'G',0,'O',0,'_',0,
        'S',0,'E',0,'R',0,'V',0,'I',0,'C',0,'E',0,'S',0,'|',0,'1',0,
        '\n',0,'S',0,'|',0,'4',0,'9',0,'5',0,'3',0,'5',0,'3',0,'\n',0};
      out.write(reinterpret_cast<const char*>(bytes),sizeof(bytes)); }
    assert(restored.load(path).ok);
    assert(restored.size()==1&&restored.isRegistered("ISS"));

    std::remove(path.c_str());std::remove((path+".tmp").c_str());
    return 0;
}
