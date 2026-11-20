#include "../../src/core/SimulationEventJournal.h"
#include <cassert>
#include <cstdio>
#include <fstream>
int main() {
    const char* path="OEMSimulationEventJournalTests.tmp";
    std::remove(path);std::remove("OEMSimulationEventJournalTests.tmp.tmp");
    oem::SimulationEventQueue original;oem::SimulationEvent event;
    event.eventId="orbiter:session-1:LANDING:GL-NT:125000";event.type=oem::SIM_EVENT_LANDING;
    event.vesselId="GL-NT";event.bodyId="Earth";event.baseId="Cape Canaveral";
    event.simulationTime=125;event.simulationTimeMilliseconds=125000;
    event.distanceMetres=42.25;event.locationKnown=true;assert(original.publish(event).ok);
    oem::SimulationEvent dock;dock.eventId="orbiter:session-1:DOCK:GL-NT:126000:PORT0:ISS";
    dock.type=oem::SIM_EVENT_DOCK;dock.vesselId="GL-NT";dock.relatedVesselId="ISS";
    dock.dockingPortIndex=0;dock.simulationTime=126;dock.simulationTimeMilliseconds=126000;
    assert(original.publish(dock).ok);
    oem::SimulationEventJournalStore store;assert(store.save(path,original).ok);
    oem::SimulationEventQueue restored;assert(store.load(path,restored).ok);
    assert(restored.events().size()==2);assert(restored.events()[0].eventId==event.eventId);
    assert(restored.events()[0].baseId==event.baseId);
    assert(restored.events()[0].distanceMetres==event.distanceMetres);
    assert(restored.events()[1].relatedVesselId=="ISS");
    assert(restored.events()[1].dockingPortIndex==0);
    assert(store.load(path,restored).ok);assert(restored.events().size()==2);
    // Legacy version 1 records load with empty docking metadata.
    {std::ofstream f(path,std::ios::out|std::ios::trunc|std::ios::binary);
     f<<"OEM_EVENT_JOURNAL|1\n"
      <<"E|6F7262697465723A73657373696F6E2D313A4C414E44494E473A474C2D4E543A313235303030|2|474C2D4E54|4561727468|436170652043616E61766572616C|125|125000|42.25|1\n";}
    oem::SimulationEventQueue legacy;assert(store.load(path,legacy).ok);
    assert(legacy.events().size()==1&&legacy.events()[0].relatedVesselId.empty());
    assert(legacy.events()[0].dockingPortIndex==-1);assert(store.save(path,legacy).ok);
    {std::ofstream f(path,std::ios::out|std::ios::app|std::ios::binary);f<<"E|ABC";}
    oem::SimulationEventQueue recovered;assert(store.load(path,recovered).ok);
    assert(recovered.events().size()==1);assert(store.save(path,recovered).ok);
    {std::ofstream f(path,std::ios::out|std::ios::app|std::ios::binary);f<<"not-a-record\n";}
    oem::SimulationEventQueue corrupt;assert(!store.load(path,corrupt).ok);
    std::remove(path);std::remove("OEMSimulationEventJournalTests.tmp.tmp");return 0;
}
