#ifndef OEM_CORE_FLIGHT_TRANSITION_H
#define OEM_CORE_FLIGHT_TRANSITION_H

#include "SimulationEvent.h"

namespace oem {

struct FlightObservation {
    bool vesselPresent;
    std::string vesselId;
    bool landed;
    bool groundContact;
    SimulationSeconds simulationTime;
    long long simulationTimeMilliseconds;
    bool locationKnown;
    std::string bodyId;
    std::string nearestBaseId;
    double nearestBaseDistanceMetres;

    FlightObservation();
};

// Emits one takeoff when a previously grounded vessel loses actual ground
// contact. The first observation is always a baseline, never a departure.
class FlightTransitionMonitor {
public:
    FlightTransitionMonitor();
    bool observe(const FlightObservation& observation, SimulationEvent& event);
    void reset();

private:
    bool initialized_;
    std::string vesselId_;
    bool wasGrounded_;
    bool lastLocationKnown_;
    std::string lastBodyId_;
    std::string lastBaseId_;
    double lastDistanceMetres_;
};

} // namespace oem

#endif
