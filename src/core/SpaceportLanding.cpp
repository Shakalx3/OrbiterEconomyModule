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
    : hasVessel_(false), vesselId_(), stateKnown_(false), qualified_(false) {}

bool SpaceportLandingMonitor::observe(const LandingObservation& observation,
                                      SpaceportLandingEvent& event) {
    if (!observation.vesselPresent || observation.vesselId.empty()) {
        reset();
        return false;
    }

    if (!hasVessel_ || vesselId_ != observation.vesselId) {
        hasVessel_ = true;
        vesselId_ = observation.vesselId;
        stateKnown_ = false;
        qualified_ = false;
    }

    // Either condition is authoritative negative evidence, even if position
    // or base enumeration is temporarily unavailable.
    if (!observation.landed || !observation.groundContact) {
        stateKnown_ = true;
        qualified_ = false;
        return false;
    }

    // A landed vessel in contact is not enough: require completed, valid
    // same-body base matching from the bridge before changing state.
    if (!observation.proximityKnown || observation.bodyId.empty() ||
        observation.nearestBaseId.empty() ||
        !isWithinSpaceportRadius(observation.nearestBaseDistanceMetres, 5000.0)) {
        if (observation.proximityKnown) {
            stateKnown_ = true;
            qualified_ = false;
        }
        return false;
    }

    if (!stateKnown_) {
        // Baseline only. Loading a scenario with a vessel already parked at a
        // spaceport must not fabricate a fresh arrival event.
        stateKnown_ = true;
        qualified_ = true;
        return false;
    }

    if (qualified_)
        return false;

    qualified_ = true;
    event.vesselId = observation.vesselId;
    event.bodyId = observation.bodyId;
    event.baseId = observation.nearestBaseId;
    event.distanceMetres = observation.nearestBaseDistanceMetres;
    event.simulationTime = observation.simulationTime;
    return true;
}

void SpaceportLandingMonitor::reset() {
    hasVessel_ = false;
    vesselId_.clear();
    stateKnown_ = false;
    qualified_ = false;
}

} // namespace oem
