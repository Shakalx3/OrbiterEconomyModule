#include "../../src/core/MissionObjectiveBook.h"
#include <cassert>
static oem::SimulationEvent ev(oem::SimulationEventType type,const char* id,long long t,const char* vessel,const char* target){oem::SimulationEvent e;e.type=type;e.eventId=id;e.vesselId=vessel;e.simulationTime=t;e.simulationTimeMilliseconds=t*1000;e.locationKnown=true;e.baseId=target;e.bodyId="Moon";e.relatedVesselId=target;return e;}
static oem::MissionObjective visit(const char* id){oem::MissionObjective o;o.missionId=id;o.vesselId="GL-NT";o.kind=oem::MISSION_OBJECTIVE_VISIT_SPACEPORT;o.targetLocationId="MoonBase";o.targetBodyId="Moon";o.acceptedAt=100;o.deadline=1000;return o;}
int main(){
 oem::CargoManifest manifest("GL-NT");oem::MissionObjectiveBook book;oem::MissionObjective o=visit("visit-01");
 assert(book.accept(o,750).ok);assert(book.accept(o,750).ok);assert(!book.accept(o,751).ok);
 assert(!book.observe(ev(oem::SIM_EVENT_LANDING,"wrong",200,"GL-NT","MarsBase"),manifest).ok);
 assert(!book.observe(ev(oem::SIM_EVENT_LANDING,"wrong-vessel",200,"OTHER","MoonBase"),manifest).ok);
 assert(book.observe(ev(oem::SIM_EVENT_LANDING,"landed",200,"GL-NT","MoonBase"),manifest).ok);
 const oem::MissionObjectiveContract* done=book.find("visit-01");assert(done&&done->status==oem::MISSION_OBJECTIVE_COMPLETED&&done->completionEventId=="landed");
 oem::Ledger ledger;assert(ledger.createAccount("company",100).ok);assert(book.settle("visit-01","company",ledger).ok);assert(book.settle("visit-01","company",ledger).ok);
 oem::Credits balance=0;assert(ledger.getBalance("company",balance)&&balance==850);
 oem::MissionObjectiveBook dockBook;oem::MissionObjective d;d.missionId="dock-01";d.vesselId="GL-NT";d.kind=oem::MISSION_OBJECTIVE_DOCK_STATION;d.targetLocationId="OrbitalStation";d.acceptedAt=50;d.deadline=500;
 assert(dockBook.accept(d,400).ok);assert(!dockBook.observe(ev(oem::SIM_EVENT_DOCK,"wrong-station",100,"GL-NT","OtherStation"),manifest).ok);
 assert(dockBook.observe(ev(oem::SIM_EVENT_DOCK,"right-station",100,"GL-NT","OrbitalStation"),manifest).ok);
 oem::MissionObjectiveBook expiry;oem::MissionObjective x=visit("expires");x.deadline=150;assert(expiry.accept(x,100).ok);
 // An unrelated post-deadline event must not silently mutate mission state;
 // expiry is committed separately by the durable lifecycle path.
 assert(!expiry.observe(ev(oem::SIM_EVENT_DOCK,"late-unrelated",200,"GL-NT","OtherStation"),manifest).ok);
 const oem::MissionObjectiveContract* stillAccepted=expiry.find("expires");
 assert(stillAccepted&&stillAccepted->status==oem::MISSION_OBJECTIVE_ACCEPTED);
 assert(expiry.expireDue(150).empty());
 std::vector<std::string> expired=expiry.expireDue(151);assert(expired.size()==1&&expired[0]=="expires");
 oem::MissionObjectiveBook cancel;oem::MissionObjective c=visit("cancel");assert(cancel.accept(c,100).ok);assert(cancel.cancel("cancel").ok);
 assert(!cancel.restoreCompleted("cancel","late-event",200).ok);
 oem::MissionObjectiveBook replay;oem::MissionObjective r=visit("replay");assert(replay.accept(r,100).ok);
 assert(replay.restoreCompleted("replay","persisted-event",300).ok);assert(replay.restoreCompleted("replay","persisted-event",300).ok);
 assert(!replay.restoreCompleted("replay","different-event",300).ok);assert(replay.restoreSettled("replay").ok);assert(replay.restoreSettled("replay").ok);
 return 0;
}