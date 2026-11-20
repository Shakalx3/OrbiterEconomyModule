#include "../../src/core/CargoTransferGate.h"
#include <cassert>

static oem::CargoTransfer makeTransfer(const char* vessel,const char* location,
                                       oem::CargoTransferDirection direction=oem::CARGO_LOAD) {
    oem::CargoTransfer t;t.transferId="transfer-1";t.direction=direction;
    t.vesselId=vessel;t.locationId=location;t.cargoId="oxygen";
    t.quantity=2;t.unitMassKg=10.0;t.simulationTime=42;return t;
}
int main() {
    oem::CargoTransferGate gate;
    oem::CargoManifest manifest("GL-NT");
    oem::CargoTransferObservation o;o.vesselId="GL-NT";

    // Mere landed flag is insufficient: Orbiter must also report actual contact.
    o.landed=true;o.surfaceLocationKnown=true;o.surfaceBaseId="MoonBase";
    o.surfaceBaseDistanceMetres=100.0;
    assert(!gate.authorize(makeTransfer("GL-NT","MoonBase"),o).ok);
    o.groundContact=true;
    assert(gate.authorize(makeTransfer("GL-NT","MoonBase"),o).ok);
    assert(gate.apply(manifest,makeTransfer("GL-NT","MoonBase"),o).ok);
    assert(manifest.quantity("oxygen")==2);

    // Taxiing contact without the landed flag cannot transact.
    o.landed=false;
    assert(!gate.authorize(makeTransfer("GL-NT","MoonBase"),o).ok);
    o.landed=true;

    // A base farther than 5 km is not an eligible cargo location.
    o.surfaceBaseDistanceMetres=5000.01;
    assert(!gate.authorize(makeTransfer("GL-NT","MoonBase"),o).ok);
    o.surfaceBaseDistanceMetres=5000.0;
    assert(gate.authorize(makeTransfer("GL-NT","MoonBase"),o).ok);
    assert(!gate.authorize(makeTransfer("GL-NT","OtherBase"),o).ok);

    // An arbitrary docking partner is not a cargo terminal.
    o.landed=false;o.groundContact=false;o.surfaceLocationKnown=false;
    assert(!gate.authorize(makeTransfer("GL-NT","CargoShip"),o).ok);
    o.dockedToCargoService=true;o.cargoServiceVesselId="CargoShip";
    assert(gate.authorize(makeTransfer("GL-NT","CargoShip"),o).ok);
    assert(!gate.authorize(makeTransfer("GL-NT","OtherShip"),o).ok);

    // Service registry must not authorize a different vessel's manifest.
    assert(!gate.authorize(makeTransfer("OTHER","CargoShip"),o).ok);
    assert(manifest.quantity("oxygen")==2);
    return 0;
}
