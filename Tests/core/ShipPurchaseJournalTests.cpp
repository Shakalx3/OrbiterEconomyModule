#include "../../src/core/EconomyJournal.h"
#include "../../src/core/ShipOwnership.h"
#include <cassert>
#include <cstdio>
#include <map>
#include <string>

int main(){
    const std::string path="oem_ship_purchase_journal_test.journal";
    std::remove(path.c_str());
    oem::Ledger ledger;assert(ledger.createAccount("company",100000).ok);
    oem::PlayerFleetRegistry fleet;
    oem::OwnedVesselRecord vessel;vessel.purchaseId="purchase-42";vessel.vesselId="OEM-SHIP-42";
    vessel.className="DeltaGlider";vessel.acquisitionLocationId="ISS";vessel.acquisitionPrice=25000;vessel.acquiredAt=900;
    oem::EconomyJournalStore journal;
    assert(fleet.registerPurchase(vessel).ok);
    assert(fleet.save("oem_player_fleet_test.registry").ok);
    assert(journal.chargePropellant(path,"ship-purchase:"+vessel.purchaseId,"company",vessel.acquisitionPrice,vessel.acquiredAt,ledger).ok);
    oem::Credits balance=0;assert(ledger.getBalance("company",balance)&&balance==75000);
    assert(fleet.isOwned(vessel.vesselId));
    assert(journal.chargePropellant(path,"ship-purchase:"+vessel.purchaseId,"company",vessel.acquisitionPrice,vessel.acquiredAt,ledger).ok);
    assert(ledger.getBalance("company",balance)&&balance==75000);
    oem::Ledger recoveredLedger;assert(recoveredLedger.createAccount("company",100000).ok);
    oem::PlayerFleetRegistry recoveredFleet;oem::CargoCatalog catalog;
    std::map<std::string,oem::CargoServiceInventory> inventories;
    std::map<std::string,oem::CargoManifest> manifests;
    assert(journal.load(path,catalog,recoveredLedger,inventories,manifests).ok);
    assert(recoveredFleet.load("oem_player_fleet_test.registry").ok);
    assert(recoveredLedger.getBalance("company",balance)&&balance==75000);
    oem::OwnedVesselRecord restored;assert(recoveredFleet.findByPurchase("purchase-42",restored));
    assert(restored.vesselId=="OEM-SHIP-42"&&restored.acquisitionPrice==25000);
    assert(journal.load(path,catalog,recoveredLedger,inventories,manifests).ok);
    std::remove("oem_player_fleet_test.registry");
    std::remove(path.c_str());return 0;
}
