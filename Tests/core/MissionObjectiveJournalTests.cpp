#include "../../src/core/EconomyJournal.h"
#include <cassert>
#include <cstdio>
#include <map>
#include <string>

static oem::MissionObjective visit(const std::string& id) {
    oem::MissionObjective o;
    o.missionId=id; o.vesselId="GL-01";
    o.kind=oem::MISSION_OBJECTIVE_VISIT_SPACEPORT;
    o.targetLocationId="MoonBase"; o.targetBodyId="Moon";
    o.acceptedAt=100; o.deadline=1000;
    return o;
}
int main() {
    const std::string path="OEMMissionObjectiveJournalTests.journal";
    std::remove(path.c_str());
    oem::EconomyJournalStore store;
    oem::MissionObjectiveBook book;
    oem::MissionObjective objective=visit("visit-001");
    assert(store.acceptMissionObjective(path,book,objective,750).ok);
    assert(store.acceptMissionObjective(path,book,objective,750).ok);
    assert(book.find("visit-001") && book.find("visit-001")->status==oem::MISSION_OBJECTIVE_ACCEPTED);
    assert(store.completeMissionObjective(path,book,"visit-001","landing-200",200).ok);
    assert(store.completeMissionObjective(path,book,"visit-001","landing-200",200).ok);
    assert(!store.completeMissionObjective(path,book,"visit-001","different-event",200).ok);
    oem::Ledger ledger; assert(ledger.createAccount("company",1000).ok);
    assert(store.settleMissionObjective(path,book,"visit-001","company",ledger).ok);
    assert(store.settleMissionObjective(path,book,"visit-001","company",ledger).ok);
    oem::Credits balance=0;
    assert(ledger.getBalance("company",balance) && balance==1750);
    assert(book.find("visit-001")->status==oem::MISSION_OBJECTIVE_SETTLED);

    // Replay all objective lifecycle records through the same global economy loader.
    oem::CargoCatalog catalog;
    oem::Ledger recoveredLedger; assert(recoveredLedger.createAccount("company",1000).ok);
    std::map<std::string,oem::CargoServiceInventory> inventories;
    std::map<std::string,oem::CargoManifest> manifests;
    oem::DeliveryContractBook deliveryContracts;
    oem::MissionObjectiveBook recovered;
    assert(store.load(path,catalog,recoveredLedger,inventories,manifests,
                      &deliveryContracts,&recovered).ok);
    const oem::MissionObjectiveContract* restored=recovered.find("visit-001");
    assert(restored && restored->status==oem::MISSION_OBJECTIVE_SETTLED);
    assert(restored->completionEventId=="landing-200" && restored->completedAt==200);
    assert(restored->reward==750 && restored->objective.targetBodyId=="Moon");
    assert(recoveredLedger.getBalance("company",balance) && balance==1750);
    assert(store.load(path,catalog,recoveredLedger,inventories,manifests,
                      &deliveryContracts,&recovered).ok);
    assert(recoveredLedger.getBalance("company",balance) && balance==1750);

    // The journal refuses to replay objective records without their owning book.
    oem::Ledger noBookLedger; assert(noBookLedger.createAccount("company",1000).ok);
    std::map<std::string,oem::CargoServiceInventory> noBookInventories;
    std::map<std::string,oem::CargoManifest> noBookManifests;
    assert(!store.load(path,catalog,noBookLedger,noBookInventories,noBookManifests).ok);
    assert(noBookLedger.getBalance("company",balance) && balance==1000);

    const std::string terminalPath="OEMMissionObjectiveTerminalTests.journal";
    std::remove(terminalPath.c_str());
    oem::MissionObjectiveBook terminal;
    oem::MissionObjective cancelled=visit("cancel-001");
    assert(store.acceptMissionObjective(terminalPath,terminal,cancelled,200).ok);
    assert(store.cancelMissionObjective(terminalPath,terminal,"cancel-001").ok);
    assert(store.cancelMissionObjective(terminalPath,terminal,"cancel-001").ok);
    oem::MissionObjective expired=visit("expire-001");
    expired.deadline=150;
    assert(store.acceptMissionObjective(terminalPath,terminal,expired,300).ok);
    assert(store.expireMissionObjective(terminalPath,terminal,"expire-001").ok);
    oem::Ledger terminalLedger; assert(terminalLedger.createAccount("company",500).ok);
    oem::MissionObjectiveBook terminalRecovered;
    std::map<std::string,oem::CargoServiceInventory> terminalInventories;
    std::map<std::string,oem::CargoManifest> terminalManifests;
    assert(store.load(terminalPath,catalog,terminalLedger,terminalInventories,
                      terminalManifests,0,&terminalRecovered).ok);
    assert(terminalRecovered.find("cancel-001")->status==oem::MISSION_OBJECTIVE_CANCELLED);
    assert(terminalRecovered.find("expire-001")->status==oem::MISSION_OBJECTIVE_EXPIRED);
    assert(terminalLedger.getBalance("company",balance) && balance==500);

    std::remove(terminalPath.c_str());
    std::remove(path.c_str());
    return 0;
}
