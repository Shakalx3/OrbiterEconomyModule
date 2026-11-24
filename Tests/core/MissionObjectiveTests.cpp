#include "../../src/core/MissionObjective.h"
#include <cassert>

static oem::CargoTransfer cargoTransfer(const char* id,
        oem::CargoTransferDirection direction, const char* location,
        long long quantity, long long time) {
    oem::CargoTransfer t;
    t.transferId=id; t.direction=direction; t.vesselId="GL-NT";
    t.locationId=location; t.cargoId="oxygen"; t.quantity=quantity;
    t.unitMassKg=10.0; t.simulationTime=time;
    return t;
}

static oem::SimulationEvent event(oem::SimulationEventType type,
        const char* id, long long time) {
    oem::SimulationEvent e;
    e.eventId=id; e.type=type; e.vesselId="GL-NT";
    e.simulationTime=time; e.simulationTimeMilliseconds=time*1000;
    e.locationKnown=true; e.baseId="MoonBase"; e.bodyId="Moon";
    e.relatedVesselId="MoonStation";
    return e;
}

int main() {
    oem::CargoManifest manifest("GL-NT");
    oem::MissionObjective visit;
    visit.missionId="visit-1"; visit.vesselId="GL-NT";
    visit.kind=oem::MISSION_OBJECTIVE_VISIT_SPACEPORT;
    visit.targetLocationId="MoonBase"; visit.targetBodyId="Moon";
    visit.acceptedAt=100; visit.deadline=500;
    assert(oem::validateMissionObjective(visit).ok);
    assert(oem::missionObjectiveMatchesEvent(visit,
        event(oem::SIM_EVENT_LANDING,"land-moon",200),manifest));
    oem::SimulationEvent wrongBase=event(oem::SIM_EVENT_LANDING,"wrong-base",200);
    wrongBase.baseId="MarsBase"; wrongBase.bodyId="Mars";
    assert(!oem::missionObjectiveMatchesEvent(visit,wrongBase,manifest));
    oem::SimulationEvent wrongBody=event(oem::SIM_EVENT_LANDING,"wrong-body",200);
    wrongBody.bodyId="Mars";
    assert(!oem::missionObjectiveMatchesEvent(visit,wrongBody,manifest));
    assert(!oem::missionObjectiveMatchesEvent(visit,
        event(oem::SIM_EVENT_TAKEOFF,"takeoff",200),manifest));
    oem::SimulationEvent before=event(oem::SIM_EVENT_LANDING,"before",99);
    assert(!oem::missionObjectiveMatchesEvent(visit,before,manifest));
    oem::SimulationEvent late=event(oem::SIM_EVENT_LANDING,"late",501);
    assert(!oem::missionObjectiveMatchesEvent(visit,late,manifest));
    oem::SimulationEvent wrongVessel=event(oem::SIM_EVENT_LANDING,"other",200);
    wrongVessel.vesselId="OTHER";
    assert(!oem::missionObjectiveMatchesEvent(visit,wrongVessel,manifest));

    oem::MissionObjective dock;
    dock.missionId="dock-1"; dock.vesselId="GL-NT";
    dock.kind=oem::MISSION_OBJECTIVE_DOCK_STATION;
    dock.targetLocationId="MoonStation"; dock.acceptedAt=100;
    assert(oem::validateMissionObjective(dock).ok);
    assert(oem::missionObjectiveMatchesEvent(dock,
        event(oem::SIM_EVENT_DOCK,"dock",200),manifest));
    oem::SimulationEvent wrongStation=event(oem::SIM_EVENT_DOCK,"wrong-station",200);
    wrongStation.relatedVesselId="OtherStation";
    assert(!oem::missionObjectiveMatchesEvent(dock,wrongStation,manifest));
    assert(!oem::missionObjectiveMatchesEvent(dock,
        event(oem::SIM_EVENT_UNDOCK,"undock",200),manifest));

    oem::MissionObjective delivery;
    delivery.missionId="delivery-1"; delivery.vesselId="GL-NT";
    delivery.kind=oem::MISSION_OBJECTIVE_DELIVER_CARGO;
    delivery.originLocationId="EarthStation";
    delivery.targetLocationId="MoonBase"; delivery.targetBodyId="Moon";
    delivery.cargoId="oxygen"; delivery.quantity=3;
    delivery.acceptedAt=100; delivery.deadline=500;
    assert(oem::validateMissionObjective(delivery).ok);
    assert(manifest.load(cargoTransfer("load-too-early",oem::CARGO_LOAD,
        "EarthStation",3,99)).ok);
    oem::SimulationEvent arrival=event(oem::SIM_EVENT_LANDING,"arrival",200);
    assert(!oem::missionObjectiveMatchesEvent(delivery,arrival,manifest));
    assert(manifest.load(cargoTransfer("load-after-accept",oem::CARGO_LOAD,
        "EarthStation",3,150)).ok);
    assert(oem::missionObjectiveMatchesEvent(delivery,arrival,manifest));
    assert(manifest.unload(cargoTransfer("unload",oem::CARGO_UNLOAD,
        "EarthStation",3,190)).ok);
    assert(!oem::missionObjectiveMatchesEvent(delivery,arrival,manifest));

    oem::MissionObjective invalid=visit;
    invalid.targetBodyId.clear();
    assert(!oem::validateMissionObjective(invalid).ok);
    invalid=delivery; invalid.quantity=0;
    assert(!oem::validateMissionObjective(invalid).ok);
    invalid=delivery; invalid.deadline=99;
    assert(!oem::validateMissionObjective(invalid).ok);
    return 0;
}
