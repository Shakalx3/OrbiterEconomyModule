#include "../../src/core/CargoExchangeJournal.h"
#include <cassert>
#include <cstdio>
#include <fstream>
#include <map>
#include <string>
static oem::CargoTransfer make(const char* id,oem::CargoTransferDirection d,const char* vessel,long long q) {
    oem::CargoTransfer t;t.transferId=id;t.direction=d;t.vesselId=vessel;t.locationId="ISS";
    t.cargoId="oxygen";t.quantity=q;t.unitMassKg=10.0;t.simulationTime=77;return t;
}
int main() {
    const std::string path="OEMCargoExchangeTests.journal";std::remove(path.c_str());
    oem::CargoServiceInventory station("ISS");assert(station.setInitialStock("oxygen",12,10.0).ok);
    oem::CargoManifest ship("GL-01");oem::CargoExchangeJournalStore store;
    oem::CargoTransfer load=make("tx-1",oem::CARGO_LOAD,"GL-01",5);
    assert(store.apply(path,station,ship,load).ok);
    assert(station.quantity("oxygen")==7&&ship.quantity("oxygen")==5);
    assert(store.apply(path,station,ship,load).ok);
    assert(station.quantity("oxygen")==7&&ship.quantity("oxygen")==5);
    oem::CargoTransfer tooMuch=make("tx-2",oem::CARGO_LOAD,"GL-01",8);
    assert(!store.apply(path,station,ship,tooMuch).ok);
    oem::CargoTransfer unload=make("tx-3",oem::CARGO_UNLOAD,"GL-01",2);
    assert(store.apply(path,station,ship,unload).ok);
    assert(station.quantity("oxygen")==9&&ship.quantity("oxygen")==3);
    oem::CargoServiceInventory restoredStation("ISS");
    assert(restoredStation.setInitialStock("oxygen",12,10.0).ok);
    std::map<std::string,oem::CargoManifest> restoredShips;
    assert(store.load(path,restoredStation,restoredShips).ok);
    assert(restoredStation.quantity("oxygen")==9);
    assert(restoredShips.find("GL-01")!=restoredShips.end());
    assert(restoredShips["GL-01"].quantity("oxygen")==3);
    // A torn final fragment is discarded before recovery and before the next append.
    {std::ofstream out(path.c_str(),std::ios::app|std::ios::binary);out<<"X|torn";}
    oem::CargoServiceInventory afterTear("ISS");assert(afterTear.setInitialStock("oxygen",12,10.0).ok);
    std::map<std::string,oem::CargoManifest> afterTearShips;
    assert(store.load(path,afterTear,afterTearShips).ok&&afterTear.quantity("oxygen")==9);
    oem::CargoTransfer last=make("tx-4",oem::CARGO_LOAD,"GL-01",1);
    assert(store.apply(path,afterTear,afterTearShips["GL-01"],last).ok);
    // A complete malformed record must reject the whole replay without partial mutation.
    {std::ofstream out(path.c_str(),std::ios::app|std::ios::binary);out<<"BAD|record\n";}
    oem::CargoServiceInventory rejected("ISS");assert(rejected.setInitialStock("oxygen",12,10.0).ok);
    std::map<std::string,oem::CargoManifest> rejectedShips;
    assert(!store.load(path,rejected,rejectedShips).ok);
    assert(rejected.quantity("oxygen")==12&&rejectedShips.empty());
    std::remove(path.c_str());return 0;
}
