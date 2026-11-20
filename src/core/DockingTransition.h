#ifndef OEM_CORE_DOCKING_TRANSITION_H
#define OEM_CORE_DOCKING_TRANSITION_H
#include "SimulationEvent.h"
#include <map>
#include <string>
#include <vector>
namespace oem {
struct DockingPortObservation {
    unsigned int portIndex;
    std::string otherVesselId;
    DockingPortObservation();
    DockingPortObservation(unsigned int index, const std::string& otherVessel);
};
struct DockingObservation {
    bool vesselPresent;
    std::string vesselId;
    SimulationSeconds simulationTime;
    long long simulationTimeMilliseconds;
    std::vector<DockingPortObservation> ports;
    DockingObservation();
};
// First sample establishes baseline; subsequent actual per-port changes emit events.
class DockingTransitionMonitor {
public:
    DockingTransitionMonitor();
    bool observe(const DockingObservation& observation, std::vector<SimulationEvent>& events);
    void reset();
private:
    bool initialized_;
    std::string vesselId_;
    std::map<unsigned int, std::string> dockedPorts_;
};
}
#endif
