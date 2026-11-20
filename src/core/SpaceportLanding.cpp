#include "SpaceportLanding.h"
#include "SurfaceDistance.h"

namespace oem {

LandingObservation::LandingObservation()
    : vesselPresent(false),
      vesselId(),
      simulationTime(0),
      landed(false),
      groundContact(false),
      proximityKnown(false),
      bodyId(),
      nearestBaseId(),
      nearestBaseDistanceMetres(-1.0) {}

SpaceportLandingEvent::SpaceportLandingEvent()
    : vesselId(), bodyId(), baseId(), distanceMetres(-1.0), simulationTime(0) {}

SpaceportLandingMonitor::SpaceportLandingMonitor()
    : hasVessel_(false), vesselId_(), armedForLanding_(false) {}

bool SpaceportLandingMonitor::observe(const LandingObservation& observation,
                                      SpaceportLandingEvent& event) {
    if (!observation.vesselPresent || observation.vesselId.empty()) {
        reset();
        return false;
    }

    if (!hasVessel_ || vesselId_ != observation.vesselId) {
        hasVessel_ = true;
        vesselId_ = observation.vesselId;
        // Do not invent an arrival when monitoring starts with a vessel
        // already parked. A vessel first observed airborne is armed.
        armedForLanding_ = !observation.landed || !observation.groundContact;
    }

    // Only a confirmed airborne / no-contact state arms the next touchdown.
    if (!observation.landed || !observation.groundContact) {
        armedForLanding_ = true;
        return false;
    }

    if (!armedForLanding_)
        return false;

    // After touchdown, wait for complete location evidence. Unknown telemetry
    // must neither accept the landing nor disarm the observed touchdown.
    if (!observation.proximityKnown || observation.bodyId.empty())
        return false;

    // A completed search with no base, or a base outside the inclusive 5 km
    // radius, is a real touchdown but not a spaceport landing. Disarm until
    // the vessel has left the surface again.
    if (observation.nearestBaseId.empty() ||
        !isWithinSpaceportRadius(observation.nearestBaseDistanceMetres, 5000.0)) {
        armedForLanding_ = false;
        return false;
    }

    event.vesselId = observation.vesselId;
    event.bodyId = observation.bodyId;
    event.baseId = observation.nearestBaseId;
    event.distanceMetres = observation.nearestBaseDistanceMetres;
    event.simulationTime = observation.simulationTime;
    armedForLanding_ = false;
    return true;
}

void SpaceportLandingMonitor::reset() {
    hasVessel_ = false;
    vesselId_.clear();
    armedForLanding_ = false;
}

} // namespace oem
