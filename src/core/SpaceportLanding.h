#ifndef OEM_CORE_SPACEPORT_LANDING_H
#define OEM_CORE_SPACEPORT_LANDING_H

#include "Types.h"
#include <string>

namespace oem {

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

// Emits one event after an observed loss of ground contact followed by a
// landed + ground-contact state within 5 km of a same-body surface base.
// Ground contact without the landed flag is treated as a transitional
// surface/taxi state, not as an airborne state. Initially parked or taxiing
// vessels establish a baseline and do not emit a false arrival.
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
