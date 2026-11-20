#include "../../src/core/SimulationEventJournal.h"
#include <cassert>
#include <cstdio>
#include <fstream>

int main() {
    const char* path = "OEMSimulationEventJournalTests.tmp";
    std::remove(path);
    std::remove("OEMSimulationEventJournalTests.tmp.tmp");

    oem::SimulationEventQueue original;
    oem::SimulationEvent event;
    event.eventId = "orbiter:session-1:LANDING:GL-NT:125000";
    event.type = oem::SIM_EVENT_LANDING;
    event.vesselId = "GL-NT";
    event.bodyId = "Earth";
    event.baseId = "Cape Canaveral";
    event.simulationTime = 125;
    event.simulationTimeMilliseconds = 125000;
    event.distanceMetres = 42.25;
    event.locationKnown = true;
    assert(original.publish(event).ok);

    oem::SimulationEventJournalStore store;
    assert(store.save(path, original).ok);

    oem::SimulationEventQueue restored;
    assert(store.load(path, restored).ok);
    assert(restored.events().size() == 1);
    assert(restored.events()[0].eventId == event.eventId);
    assert(restored.events()[0].baseId == event.baseId);
    assert(restored.events()[0].distanceMetres == event.distanceMetres);

    // Re-loading the same journal into an existing queue is idempotent.
    assert(store.load(path, restored).ok);
    assert(restored.events().size() == 1);

    // A torn final record is ignored; a subsequent save repairs the file.
    {
        std::ofstream damaged(path, std::ios::out | std::ios::app | std::ios::binary);
        damaged << "E|ABC";
    }
    oem::SimulationEventQueue recovered;
    assert(store.load(path, recovered).ok);
    assert(recovered.events().size() == 1);
    assert(store.save(path, recovered).ok);

    oem::SimulationEventQueue afterRepair;
    assert(store.load(path, afterRepair).ok);
    assert(afterRepair.events().size() == 1);

    // A complete malformed record is not silently accepted.
    {
        std::ofstream damaged(path, std::ios::out | std::ios::app | std::ios::binary);
        damaged << "not-a-record\n";
    }
    oem::SimulationEventQueue corrupt;
    assert(!store.load(path, corrupt).ok);

    std::remove(path);
    std::remove("OEMSimulationEventJournalTests.tmp.tmp");
    return 0;
}
