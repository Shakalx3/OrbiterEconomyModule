#include "../../src/core/CargoManifest.h"
#include <cassert>
#include <cmath>

static oem::CargoTransfer transfer(const char* id, oem::CargoTransferDirection direction,
                                   const char* location, long long quantity) {
    oem::CargoTransfer t;
    t.transferId = id;
    t.direction = direction;
    t.vesselId = "GL-NT";
    t.locationId = location;
    t.cargoId = "oxygen_canister";
    t.quantity = quantity;
    t.unitMassKg = 50.0;
    t.simulationTime = 120;
    return t;
}

int main() {
    oem::CargoManifest manifest("GL-NT");
    oem::CargoTransfer load = transfer("load-001", oem::CARGO_LOAD, "ISS", 4);
    assert(manifest.load(load).ok);
    assert(manifest.quantity("oxygen_canister") == 4);
    assert(std::fabs(manifest.totalMassKg() - 200.0) < 1e-9);
    assert(manifest.lines().size() == 1);
    assert(manifest.lines()[0].originLocationId == "ISS");
    assert(manifest.history().size() == 1);

    assert(manifest.load(load).ok); // exact replay is idempotent
    assert(manifest.quantity("oxygen_canister") == 4);
    assert(manifest.history().size() == 1);

    oem::CargoTransfer conflict = load;
    conflict.quantity = 5;
    assert(!manifest.load(conflict).ok);
    assert(manifest.quantity("oxygen_canister") == 4);

    oem::CargoTransfer wrongVessel = load;
    wrongVessel.transferId = "wrong-vessel";
    wrongVessel.vesselId = "OTHER";
    assert(!manifest.load(wrongVessel).ok);

    oem::CargoTransfer tooMuch = transfer("unload-too-much", oem::CARGO_UNLOAD, "MoonBase", 5);
    assert(!manifest.unload(tooMuch).ok);
    assert(manifest.quantity("oxygen_canister") == 4);

    oem::CargoTransfer unload = transfer("unload-001", oem::CARGO_UNLOAD, "MoonBase", 2);
    assert(manifest.unload(unload).ok);
    assert(manifest.quantity("oxygen_canister") == 2);
    assert(std::fabs(manifest.totalMassKg() - 100.0) < 1e-9);

    oem::CargoTransfer unloadRest = transfer("unload-002", oem::CARGO_UNLOAD, "MarsBase", 2);
    assert(manifest.unload(unloadRest).ok);
    assert(manifest.quantity("oxygen_canister") == 0);
    assert(manifest.lines().empty());
    assert(manifest.history().size() == 3);

    oem::CargoTransfer bad = transfer("bad", oem::CARGO_LOAD, "ISS", 1);
    bad.unitMassKg = 0.0;
    assert(!manifest.load(bad).ok);
    assert(manifest.history().size() == 3);
    return 0;
}
