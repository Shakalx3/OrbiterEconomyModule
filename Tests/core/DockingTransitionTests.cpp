#include "../../src/core/DockingTransition.h"
#include <cassert>
#include <vector>
static oem::DockingObservation sample(const char* vessel,long long ms) {
    oem::DockingObservation o;o.vesselPresent=true;o.vesselId=vessel;
    o.simulationTimeMilliseconds=ms;o.simulationTime=ms/1000;return o;
}
int main() {
    oem::DockingTransitionMonitor m;std::vector<oem::SimulationEvent> events;
    oem::DockingObservation o=sample("GL-NT",1000);
    o.ports.push_back(oem::DockingPortObservation(0,"ISS"));
    assert(!m.observe(o,events)); // existing connection is baseline
    o=sample("GL-NT",2000);o.ports.push_back(oem::DockingPortObservation(0,"ISS"));
    o.ports.push_back(oem::DockingPortObservation(1,"CargoShip"));
    assert(m.observe(o,events));assert(events.size()==1);
    assert(events[0].type==oem::SIM_EVENT_DOCK&&events[0].vesselId=="GL-NT");
    assert(events[0].relatedVesselId=="CargoShip"&&events[0].dockingPortIndex==1);
    assert(!m.observe(o,events));assert(events.empty());
    o=sample("GL-NT",3000);o.ports.push_back(oem::DockingPortObservation(0,"ISS"));
    assert(m.observe(o,events));assert(events.size()==1);
    assert(events[0].type==oem::SIM_EVENT_UNDOCK&&events[0].relatedVesselId=="CargoShip");
    o=sample("GL-NT",4000);o.ports.push_back(oem::DockingPortObservation(0,"StationB"));
    assert(m.observe(o,events));assert(events.size()==2);
    assert(events[0].type==oem::SIM_EVENT_UNDOCK&&events[0].relatedVesselId=="ISS");
    assert(events[1].type==oem::SIM_EVENT_DOCK&&events[1].relatedVesselId=="StationB");
    o=sample("OtherShip",5000);o.ports.push_back(oem::DockingPortObservation(0,"StationB"));
    assert(!m.observe(o,events));assert(events.empty());
    o=sample("OtherShip",6000);assert(!m.observe(o,events));return 0;
}
