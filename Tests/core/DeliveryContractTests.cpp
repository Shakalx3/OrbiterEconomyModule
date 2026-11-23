#include "../../src/core/DeliveryContract.h"
#include <cassert>

static oem::CargoTransfer transfer(const char* id, oem::CargoTransferDirection direction,
                                   const char* location, long long quantity, long long time) {
    oem::CargoTransfer t;
    t.transferId=id;t.direction=direction;t.vesselId="GL-NT";
    t.locationId=location;t.cargoId="oxygen";t.quantity=quantity;
    t.unitMassKg=10.0;t.simulationTime=time;return t;
}
static oem::SimulationEvent arrival(oem::SimulationEventType type,
                                    const char* id, long long time) {
    oem::SimulationEvent e;e.eventId=id;e.type=type;e.vesselId="GL-NT";
    e.simulationTime=time;e.simulationTimeMilliseconds=time*1000;
    e.locationKnown=true;e.baseId="MoonBase";e.relatedVesselId="MoonStation";
    return e;
}
static oem::DeliveryContract contract() {
    oem::DeliveryContract c;c.contractId="delivery-1";c.vesselId="GL-NT";
    c.cargoId="oxygen";c.quantity=3;c.originLocationId="ISS";
    c.destinationLocationId="MoonBase";c.reward=500;
    c.acceptedAt=100;c.deadline=1000;return c;
}
int main() {
    oem::DeliveryContractBook book;oem::DeliveryContract c=contract();
    assert(book.accept(c).ok);
    assert(book.accept(c).ok); // exact acceptance replay is idempotent
    oem::DeliveryContract conflict=c;conflict.reward++;
    assert(!book.accept(conflict).ok);
    oem::CargoManifest manifest("GL-NT");
    assert(manifest.load(transfer("too-early",oem::CARGO_LOAD,"ISS",3,99)).ok);
    oem::SimulationEvent e=arrival(oem::SIM_EVENT_LANDING,"arrival-1",200);
    assert(!book.observe(e,manifest).ok); // before-acceptance cargo cannot qualify
    assert(manifest.load(transfer("origin-load",oem::CARGO_LOAD,"ISS",3,150)).ok);
    oem::SimulationEvent wrongVessel=e;wrongVessel.vesselId="OTHER";
    assert(!book.observe(wrongVessel,manifest).ok);
    oem::SimulationEvent wrongPlace=e;wrongPlace.baseId="OtherBase";
    assert(!book.observe(wrongPlace,manifest).ok);
    oem::SimulationEvent taxi=e;taxi.locationKnown=false;
    assert(!book.observe(taxi,manifest).ok);
    assert(book.observe(e,manifest).ok);
    const oem::DeliveryContract* done=book.find("delivery-1");
    assert(done&&done->status==oem::CONTRACT_COMPLETED);
    assert(done->completionEventId=="arrival-1"&&done->completedAt==200);

    oem::Ledger ledger;assert(ledger.createAccount("company",1000).ok);
    assert(book.settle("delivery-1","company",ledger).ok);
    oem::Credits balance=0;assert(ledger.getBalance("company",balance)&&balance==1500);
    assert(book.settle("delivery-1","company",ledger).ok);
    assert(ledger.getBalance("company",balance)&&balance==1500);
    assert(book.find("delivery-1")->status==oem::CONTRACT_SETTLED);

    oem::DeliveryContractBook dockBook;oem::DeliveryContract dock=contract();
    dock.contractId="dock-delivery";dock.destinationLocationId="MoonStation";
    assert(dockBook.accept(dock).ok);
    oem::CargoManifest dockManifest("GL-NT");
    assert(dockManifest.load(transfer("dock-load",oem::CARGO_LOAD,"ISS",3,150)).ok);
    oem::SimulationEvent dockEvent=arrival(oem::SIM_EVENT_DOCK,"dock-arrival",250);
    assert(dockBook.observe(dockEvent,dockManifest).ok);

    oem::DeliveryContractBook expiryBook;oem::DeliveryContract expired=contract();
    expired.contractId="expired";expired.deadline=180;
    assert(expiryBook.accept(expired).ok);
    oem::CargoManifest expiryManifest("GL-NT");
    assert(expiryManifest.load(transfer("expiry-load",oem::CARGO_LOAD,"ISS",3,150)).ok);
    oem::SimulationEvent late=arrival(oem::SIM_EVENT_LANDING,"late-arrival",200);
    assert(!expiryBook.observe(late,expiryManifest).ok);
    assert(expiryBook.find("expired")->status==oem::CONTRACT_EXPIRED);

    oem::DeliveryContractBook unloadBook;oem::DeliveryContract unloadContract=contract();
    unloadContract.contractId="unloaded";
    assert(unloadBook.accept(unloadContract).ok);
    oem::CargoManifest unloadManifest("GL-NT");
    assert(unloadManifest.load(transfer("load",oem::CARGO_LOAD,"ISS",3,150)).ok);
    assert(unloadManifest.unload(transfer("unload",oem::CARGO_UNLOAD,"ISS",3,190)).ok);
    assert(!unloadBook.observe(e,unloadManifest).ok); // cargo not aboard at event time
    oem::DeliveryContractBook replayBook;
    oem::DeliveryContract replay=contract();
    replay.contractId="replay-contract";
    assert(replayBook.accept(replay).ok);
    assert(replayBook.restoreCompleted("replay-contract","persisted-arrival",300).ok);
    assert(replayBook.restoreCompleted("replay-contract","persisted-arrival",300).ok);
    assert(!replayBook.restoreCompleted("replay-contract","different-arrival",300).ok);
    assert(replayBook.restoreSettled("replay-contract").ok);
    assert(replayBook.restoreSettled("replay-contract").ok);
    assert(replayBook.find("replay-contract")->status==oem::CONTRACT_SETTLED);

    oem::DeliveryContractBook clockBook;
    oem::DeliveryContract clockContract=contract();
    clockContract.contractId="clock-expiry";
    assert(clockBook.accept(clockContract).ok);
    assert(clockBook.expireDue(1000).empty());
    const std::vector<std::string> expiredIds=clockBook.expireDue(1001);
    assert(expiredIds.size()==1&&expiredIds[0]=="clock-expiry");
    assert(clockBook.expireDue(2000).empty());
    assert(clockBook.find("clock-expiry")->status==oem::CONTRACT_EXPIRED);
    return 0;
}
