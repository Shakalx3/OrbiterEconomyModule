#include "../../src/core/EconomyJournal.h"
#include <cassert>
#include <cstdio>
#include <fstream>
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
    // Failure policy is created once, then existing user values are preserved.
    const std::string policyPath="OEMMissionFailurePolicyTests.cfg";
    std::remove(policyPath.c_str());
    oem::MissionFailurePolicyStore policyStore;
    oem::MissionFailurePolicy policy;
    assert(policyStore.loadOrCreate(policyPath,policy).ok);
    assert(policy.penaltyBasisPoints==1000 && policy.reputationDelta==-5);
    {
        std::ofstream custom(policyPath.c_str(),std::ios::binary|std::ios::trunc);
        custom << "OEM_MISSION_FAILURE|1\nP|2500|-8\n";
    }
    oem::MissionFailurePolicy loadedPolicy;
    assert(policyStore.loadOrCreate(policyPath,loadedPolicy).ok);
    assert(loadedPolicy.penaltyBasisPoints==2500 && loadedPolicy.reputationDelta==-8);
    {
        std::ofstream invalid(policyPath.c_str(),std::ios::binary|std::ios::trunc);
        invalid << "OEM_MISSION_FAILURE|1\nP|12000|-8\n";
    }
    assert(!policyStore.loadOrCreate(policyPath,loadedPolicy).ok);
    std::remove(policyPath.c_str());
    const std::string path="OEMMissionObjectiveJournalTests.journal";
    std::remove(path.c_str());
    oem::EconomyJournalStore store;
    oem::MissionObjectiveBook book;
    oem::MissionObjective objective=visit("visit-001");
    assert(store.acceptMissionObjective(path,book,objective,750).ok);
    assert(store.acceptMissionObjective(path,book,objective,750).ok);
    assert(book.find("visit-001") && book.find("visit-001")->status==oem::MISSION_OBJECTIVE_ACCEPTED);
    assert(!store.completeMissionObjective(path,book,"visit-001","forged-event",200).ok);
    oem::CargoManifest manifest("GL-01");
    oem::SimulationEvent landing; landing.eventId="landing-200"; landing.type=oem::SIM_EVENT_LANDING;
    landing.vesselId="GL-01"; landing.baseId="MoonBase"; landing.bodyId="Moon";
    landing.locationKnown=true; landing.simulationTime=200;
    assert(book.observe(landing,manifest).ok);
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
    oem::Ledger terminalLedger; assert(terminalLedger.createAccount("company",500).ok);
    assert(store.expireMissionObjective(terminalPath,terminal,"expire-001",terminalLedger,"company",200).ok);
    oem::MissionObjectiveBook terminalRecovered;
    std::map<std::string,oem::CargoServiceInventory> terminalInventories;
    std::map<std::string,oem::CargoManifest> terminalManifests;
    assert(store.load(terminalPath,catalog,terminalLedger,terminalInventories,
                      terminalManifests,0,&terminalRecovered).ok);
    assert(terminalRecovered.find("cancel-001")->status==oem::MISSION_OBJECTIVE_CANCELLED);
    assert(terminalRecovered.find("expire-001")->status==oem::MISSION_OBJECTIVE_EXPIRED);
    assert(terminalLedger.getBalance("company",balance) && balance==470);
    int terminalReputation=0;
    assert(terminalLedger.getReputation("company",terminalReputation) && terminalReputation==45);
    assert(store.load(terminalPath,catalog,terminalLedger,terminalInventories,
                      terminalManifests,0,&terminalRecovered).ok);
    assert(terminalLedger.getBalance("company",balance) && balance==470);
    assert(terminalLedger.getReputation("company",terminalReputation) && terminalReputation==45);

    std::remove(terminalPath.c_str());
    std::remove(path.c_str());
    return 0;
}
