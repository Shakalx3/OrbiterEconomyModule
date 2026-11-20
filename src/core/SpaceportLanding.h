#ifndef OEM_CORE_SPACEPORT_LANDING_H
#define OEM_CORE_SPACEPORT_LANDING_H

#include "Types.h"
#include <string>

namespace oem {

// Snapshot collected by the Orbiter bridge. The bridge must only set
// proximityKnown after a complete search on bodyId has finished.
struct LandingObservation {
    bool vesselPresent;
    std::string vesselId;
    SimulationSeconds simulationTime;

    bool landed;
    bool groundContact;

    bool proximityKnown;
    std::string bodyId;
    std::string nearestBaseId;
    double nearestBaseDistanceMetres;

    LandingObservation();
};

struct SpaceportLandingEvent {
    std::string vesselId;
    std::string bodyId;
    std::string baseId;
    double distanceMetres;
    SimulationSeconds simulationTime;

    SpaceportLandingEvent();
};

// Emits one event for an actual touchdown after a previously observed airborne
// or no-contact state. Initial parked vessels establish a baseline and do not
// emit an arrival. Unknown proximity evidence waits; a completed negative
// search disarms until the next takeoff/touchdown cycle.
class SpaceportLandingMonitor {
public:
    SpaceportLandingMonitor();

    bool observe(const LandingObservation& observation,
                 SpaceportLandingEvent& event);

    void reset();

private:
    bool hasVessel_;
    std::string vesselId_;
    bool armedForLanding_;
};

} // namespace oem

#endif
