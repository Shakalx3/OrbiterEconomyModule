#include "../../src/core/CargoServiceInventory.h"
#include <cassert>
static oem::CargoTransfer makeTransfer(const char* id, oem::CargoTransferDirection direction, const char* vessel, long long quantity) {
    oem::CargoTransfer t; t.transferId=id; t.direction=direction; t.vesselId=vessel;
    t.locationId="ISS"; t.cargoId="oxygen_canister"; t.quantity=quantity;
    t.unitMassKg=50.0; t.simulationTime=100; return t;
}
int main() {
    oem::CargoServiceInventory station("ISS");
    assert(station.setInitialStock("oxygen_canister",10,50.0).ok);
    assert(station.setInitialStock("fuel",20,1.0).ok);
    assert(station.quantity("oxygen_canister")==10 && station.quantity("fuel")==20);
    oem::CargoTransfer load=makeTransfer("xfer-1",oem::CARGO_LOAD,"GL-01",4);
    assert(station.apply(load).ok && station.quantity("oxygen_canister")==6);
    assert(station.apply(load).ok && station.quantity("oxygen_canister")==6);
    assert(station.history().size()==1 && station.hasTransfer("xfer-1"));
    oem::CargoTransfer conflict=load; conflict.quantity=5; assert(!station.apply(conflict).ok);
    oem::CargoTransfer excessive=makeTransfer("xfer-2",oem::CARGO_LOAD,"GL-01",7);
    assert(!station.apply(excessive).ok && station.quantity("oxygen_canister")==6);
    oem::CargoTransfer unload=makeTransfer("xfer-3",oem::CARGO_UNLOAD,"GL-01",3);
    assert(station.apply(unload).ok && station.quantity("oxygen_canister")==9);
    oem::CargoTransfer wrong=load; wrong.transferId="wrong"; wrong.locationId="MoonBase";
    assert(!station.apply(wrong).ok);
    assert(!station.setInitialStock("fuel",1,0.0).ok);
    // Transfers of oxygen must not block first-run seeding of an unrelated new cargo.
    assert(station.setInitialStock("food",12,1.0).ok && station.quantity("food")==12);
    assert(!station.setInitialStock("oxygen_canister",1,50.0).ok);
    return 0;
}
