#include "DockingTransition.h"
namespace oem {
DockingPortObservation::DockingPortObservation() : portIndex(0), otherVesselId() {}
DockingPortObservation::DockingPortObservation(unsigned int index, const std::string& otherVessel)
    : portIndex(index), otherVesselId(otherVessel) {}
DockingObservation::DockingObservation()
    : vesselPresent(false), vesselId(), simulationTime(0), simulationTimeMilliseconds(0), ports() {}
DockingTransitionMonitor::DockingTransitionMonitor()
    : initialized_(false), vesselId_(), dockedPorts_() {}
bool DockingTransitionMonitor::observe(const DockingObservation& observation,
                                       std::vector<SimulationEvent>& events) {
    events.clear();
    if (!observation.vesselPresent || observation.vesselId.empty()) { reset(); return false; }
    std::map<unsigned int, std::string> current;
    for (std::size_t i=0; i<observation.ports.size(); ++i) {
        const DockingPortObservation& p=observation.ports[i];
        if (!p.otherVesselId.empty()) current[p.portIndex]=p.otherVesselId;
    }
    if (!initialized_ || vesselId_ != observation.vesselId) {
        initialized_=true; vesselId_=observation.vesselId; dockedPorts_=current; return false;
    }
    for (std::map<unsigned int,std::string>::const_iterator old=dockedPorts_.begin();
         old!=dockedPorts_.end(); ++old) {
        std::map<unsigned int,std::string>::const_iterator now=current.find(old->first);
        if (now==current.end() || now->second!=old->second) {
            SimulationEvent e; e.type=SIM_EVENT_UNDOCK; e.vesselId=observation.vesselId;
            e.relatedVesselId=old->second; e.dockingPortIndex=(int)old->first;
            e.simulationTime=observation.simulationTime;
            e.simulationTimeMilliseconds=observation.simulationTimeMilliseconds;
            events.push_back(e);
        }
    }
    for (std::map<unsigned int,std::string>::const_iterator now=current.begin();
         now!=current.end(); ++now) {
        std::map<unsigned int,std::string>::const_iterator old=dockedPorts_.find(now->first);
        if (old==dockedPorts_.end() || old->second!=now->second) {
            SimulationEvent e; e.type=SIM_EVENT_DOCK; e.vesselId=observation.vesselId;
            e.relatedVesselId=now->second; e.dockingPortIndex=(int)now->first;
            e.simulationTime=observation.simulationTime;
            e.simulationTimeMilliseconds=observation.simulationTimeMilliseconds;
            events.push_back(e);
        }
    }
    dockedPorts_.swap(current);
    return !events.empty();
}
void DockingTransitionMonitor::reset() {
    initialized_=false; vesselId_.clear(); dockedPorts_.clear();
}
}
