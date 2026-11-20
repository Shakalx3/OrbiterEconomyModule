#include "../../src/core/CargoManifestJournal.h"
#include <cassert>
#include <cstdio>
#include <fstream>
#include <string>

static oem::CargoTransfer makeTransfer(const char* id,oem::CargoTransferDirection direction,
                                       const char* location,long long quantity) {
    oem::CargoTransfer t;t.transferId=id;t.direction=direction;t.vesselId="GL-NT";
    t.locationId=location;t.cargoId="fuel_cell";t.quantity=quantity;
    t.unitMassKg=12.5;t.simulationTime=345;return t;
}
int main() {
    const std::string path="oem_cargo_manifest_test.journal";
    std::remove(path.c_str());std::remove((path+".tmp").c_str());
    oem::CargoManifest original("GL-NT");
    assert(original.load(makeTransfer("load-a",oem::CARGO_LOAD,"ISS",8)).ok);
    assert(original.unload(makeTransfer("unload-a",oem::CARGO_UNLOAD,"MoonBase",3)).ok);
    oem::CargoManifestJournalStore store;
    assert(store.save(path,original).ok);

    oem::CargoManifest restored("GL-NT");
    assert(store.load(path,restored).ok);
    assert(restored.quantity("fuel_cell")==5);
    assert(restored.history().size()==2);
    assert(restored.hasTransfer("load-a")&&restored.hasTransfer("unload-a"));
    assert(restored.lines().size()==1);
    assert(restored.lines()[0].originLocationId=="ISS");

    // Re-loading the same journal into the same manifest is idempotent.
    assert(store.load(path,restored).ok);
    assert(restored.quantity("fuel_cell")==5&&restored.history().size()==2);

    oem::CargoManifest wrongVessel("OTHER");
    assert(!store.load(path,wrongVessel).ok);

    // A torn final line is ignored; all preceding complete records survive.
    { std::ofstream out((path+".torn").c_str(),std::ios::binary);
      out<<"OEM_CARGO_MANIFEST|1|474C2D4E\\nT|broken"; }
    oem::CargoManifest recovered("GL-NT");
    assert(store.load(path+".torn",recovered).ok);
    assert(recovered.quantity("fuel_cell")==0); // torn fixture contains no complete transfer

    std::remove(path.c_str());std::remove((path+".torn").c_str());std::remove((path+".tmp").c_str());
    return 0;
}
