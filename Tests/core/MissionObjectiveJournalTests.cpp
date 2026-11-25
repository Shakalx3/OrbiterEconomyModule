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
static oem::MissionObjective dock(const std::string& id) {
    oem::MissionObjective o;
    o.missionId=id; o.vesselId="GL-01";
    o.kind=oem::MISSION_OBJECTIVE_DOCK_STATION;
    o.targetLocationId="ISS";
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

    // The same configurable failure policy applies to VISIT and DOCK expiries.
    const std::string policyJournalPath="OEMMissionObjectivePolicyTests.journal";
    std::remove(policyJournalPath.c_str());
    oem::MissionObjectiveBook policyBook;
    oem::MissionObjective policyVisit=visit("policy-visit-001");
    policyVisit.deadline=150;
    oem::MissionObjective policyDock=dock("policy-dock-001");
    policyDock.deadline=150;
    assert(store.acceptMissionObjective(policyJournalPath,policyBook,policyVisit,200).ok);
    assert(store.acceptMissionObjective(policyJournalPath,policyBook,policyDock,400).ok);
    oem::MissionFailurePolicy customPolicy;
    customPolicy.penaltyBasisPoints=2500;
    customPolicy.reputationDelta=-8;
    assert(customPolicy.valid());
    oem::Ledger policyLedger; assert(policyLedger.createAccount("company",1000).ok);
    assert(!store.expireMissionObjective(policyJournalPath,policyBook,"policy-visit-001",
        policyLedger,"company",150,customPolicy).ok);
    assert(store.expireMissionObjective(policyJournalPath,policyBook,"policy-visit-001",
        policyLedger,"company",151,customPolicy).ok);
    assert(store.expireMissionObjective(policyJournalPath,policyBook,"policy-dock-001",
        policyLedger,"company",151,customPolicy).ok);
    assert(store.expireMissionObjective(policyJournalPath,policyBook,"policy-visit-001",
        policyLedger,"company",151,customPolicy).ok);
    assert(store.expireMissionObjective(policyJournalPath,policyBook,"policy-dock-001",
        policyLedger,"company",151,customPolicy).ok);
    assert(policyBook.find("policy-visit-001")->status==oem::MISSION_OBJECTIVE_EXPIRED);
    assert(policyBook.find("policy-dock-001")->status==oem::MISSION_OBJECTIVE_EXPIRED);
    assert(policyLedger.getBalance("company",balance) && balance==850);
    int policyReputation=0;
    assert(policyLedger.getReputation("company",policyReputation) && policyReputation==34);

    oem::Ledger policyRecoveredLedger; assert(policyRecoveredLedger.createAccount("company",1000).ok);
    oem::MissionObjectiveBook policyRecoveredBook;
    std::map<std::string,oem::CargoServiceInventory> policyStocks;
    std::map<std::string,oem::CargoManifest> policyManifests;
    assert(store.load(policyJournalPath,catalog,policyRecoveredLedger,policyStocks,
        policyManifests,0,&policyRecoveredBook).ok);
    assert(policyRecoveredBook.find("policy-visit-001")->status==oem::MISSION_OBJECTIVE_EXPIRED);
    assert(policyRecoveredBook.find("policy-dock-001")->status==oem::MISSION_OBJECTIVE_EXPIRED);
    assert(policyRecoveredLedger.getBalance("company",balance) && balance==850);
    assert(policyRecoveredLedger.getReputation("company",policyReputation) && policyReputation==34);
    assert(store.load(policyJournalPath,catalog,policyRecoveredLedger,policyStocks,
        policyManifests,0,&policyRecoveredBook).ok);
    assert(policyRecoveredLedger.getBalance("company",balance) && balance==850);
    assert(policyRecoveredLedger.getReputation("company",policyReputation) && policyReputation==34);
    std::remove(policyJournalPath.c_str());

    // A fee larger than the available balance is capped, and a retry is harmless.
    const std::string poorPolicyPath="OEMMissionObjectivePoorPolicyTests.journal";
    std::remove(poorPolicyPath.c_str());
    oem::MissionObjectiveBook poorPolicyBook;
    oem::MissionObjective poorObjective=visit("policy-poor-001");
    poorObjective.deadline=150;
    assert(store.acceptMissionObjective(poorPolicyPath,poorPolicyBook,poorObjective,100).ok);
    oem::Ledger poorPolicyLedger; assert(poorPolicyLedger.createAccount("company",5).ok);
    assert(store.expireMissionObjective(poorPolicyPath,poorPolicyBook,"policy-poor-001",
        poorPolicyLedger,"company",151,customPolicy).ok);
    assert(store.expireMissionObjective(poorPolicyPath,poorPolicyBook,"policy-poor-001",
        poorPolicyLedger,"company",151,customPolicy).ok);
    assert(poorPolicyLedger.getBalance("company",balance) && balance==0);
    assert(poorPolicyLedger.getReputation("company",policyReputation) && policyReputation==42);
    oem::Ledger poorRecoveredLedger; assert(poorRecoveredLedger.createAccount("company",5).ok);
    oem::MissionObjectiveBook poorRecoveredBook;
    std::map<std::string,oem::CargoServiceInventory> poorPolicyStocks;
    std::map<std::string,oem::CargoManifest> poorPolicyManifests;
    assert(store.load(poorPolicyPath,catalog,poorRecoveredLedger,poorPolicyStocks,
        poorPolicyManifests,0,&poorRecoveredBook).ok);
    assert(poorRecoveredBook.find("policy-poor-001")->status==oem::MISSION_OBJECTIVE_EXPIRED);
    assert(poorRecoveredLedger.getBalance("company",balance) && balance==0);
    assert(poorRecoveredLedger.getReputation("company",policyReputation) && policyReputation==42);
    std::remove(poorPolicyPath.c_str());

    // Cancelling by itself changes neither company funds nor reputation.
    const std::string cancelOnlyPath="OEMMissionObjectiveCancelOnlyTests.journal";
    std::remove(cancelOnlyPath.c_str());
    oem::MissionObjectiveBook cancelOnlyBook;
    oem::MissionObjective cancelOnly=visit("cancel-only-001");
    assert(store.acceptMissionObjective(cancelOnlyPath,cancelOnlyBook,cancelOnly,300).ok);
    assert(store.cancelMissionObjective(cancelOnlyPath,cancelOnlyBook,"cancel-only-001").ok);
    oem::Ledger cancelOnlyLedger; assert(cancelOnlyLedger.createAccount("company",500).ok);
    oem::MissionObjectiveBook cancelOnlyRecovered;
    std::map<std::string,oem::CargoServiceInventory> cancelOnlyStocks;
    std::map<std::string,oem::CargoManifest> cancelOnlyManifests;
    assert(store.load(cancelOnlyPath,catalog,cancelOnlyLedger,cancelOnlyStocks,
        cancelOnlyManifests,0,&cancelOnlyRecovered).ok);
    assert(cancelOnlyRecovered.find("cancel-only-001")->status==oem::MISSION_OBJECTIVE_CANCELLED);
    assert(cancelOnlyLedger.getBalance("company",balance) && balance==500);
    assert(!cancelOnlyLedger.getReputation("company",policyReputation));
    std::remove(cancelOnlyPath.c_str());

    std::remove(path.c_str());
    return 0;
}
