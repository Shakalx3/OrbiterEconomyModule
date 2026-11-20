#ifndef OEM_CORE_SPACEPORT_LANDING_H
#define OEM_CORE_SPACEPORT_LANDING_H

#include "Types.h"
#include <string>

namespace oem {

// Snapshot collected by the Orbiter bridge. Coordinates and distance are in
// radians/metres at the bridge boundary; the core only consumes the verified
// nearest-base distance and its identity.
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

// Emits one event when a vessel transitions from a known non-qualifying state
// into a fully verified spaceport landing. The first sample for each vessel
// establishes a baseline and never emits an event.
class SpaceportLandingMonitor {
public:
    SpaceportLandingMonitor();

    // Returns true only when a new landing event is emitted. Unknown proximity
    // evidence does not change the previous known state.
    bool observe(const LandingObservation& observation,
                 SpaceportLandingEvent& event);

    void reset();

private:
    bool hasVessel_;
    std::string vesselId_;
    bool stateKnown_;
    bool qualified_;
};

} // namespace oem

#endif
