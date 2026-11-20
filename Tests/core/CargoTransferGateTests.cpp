#include "../../src/core/CargoTransferGate.h"
#include <cassert>

static oem::CargoTransfer transfer(const char* vessel,const char* location,
 oem::CargoTransferDirection direction=oem::CARGO_LOAD) {
 oem::CargoTransfer t;t.transferId="transfer-1";t.direction=direction;t.vesselId=vessel;
 t.locationId=location;t.cargoId="oxygen";t.quantity=2;t.unitMassKg=10.0;t.simulationTime=42;return t;
}
int main() {
 oem::CargoTransferGate gate;
 oem::CargoServiceRegistry services;
 oem::CargoManifest manifest("GL-NT");
 oem::CargoTransferObservation o;o.vesselId="GL-NT";
 o.landed=true;o.surfaceLocationKnown=true;o.surfaceBaseId="MoonBase";o.surfaceBaseDistanceMetres=100;
 assert(!gate.authorize(transfer("GL-NT","MoonBase"),o,services).ok);
 o.groundContact=true;assert(gate.apply(manifest,transfer("GL-NT","MoonBase"),o,services).ok);
 assert(manifest.quantity("oxygen")==2);
 o.landed=false;assert(!gate.authorize(transfer("GL-NT","MoonBase"),o,services).ok);o.landed=true;
 o.surfaceBaseDistanceMetres=5000.01;assert(!gate.authorize(transfer("GL-NT","MoonBase"),o,services).ok);
 o.surfaceBaseDistanceMetres=5000;assert(gate.authorize(transfer("GL-NT","MoonBase"),o,services).ok);
 assert(!gate.authorize(transfer("GL-NT","OtherBase"),o,services).ok);
 o.landed=false;o.groundContact=false;o.surfaceLocationKnown=false;
 o.docked=true;o.dockedVesselId="CargoShip";
 assert(!gate.authorize(transfer("GL-NT","CargoShip"),o,services).ok);
 assert(!services.registerService(""));
 assert(services.registerService("CargoShip"));
 assert(!services.registerService("CargoShip"));
 assert(services.size()==1);
 assert(gate.authorize(transfer("GL-NT","CargoShip"),o,services).ok);
 assert(!gate.authorize(transfer("GL-NT","OtherShip"),o,services).ok);
 assert(!gate.authorize(transfer("OTHER","CargoShip"),o,services).ok);
 o.docked=false;assert(!gate.authorize(transfer("GL-NT","CargoShip"),o,services).ok);
 assert(services.unregisterService("CargoShip"));
 assert(!gate.authorize(transfer("GL-NT","CargoShip"),o,services).ok);
 assert(manifest.quantity("oxygen")==2);return 0;
}
