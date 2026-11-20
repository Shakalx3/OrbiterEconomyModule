#include "FlightTransition.h"

namespace oem {

namespace {
const long long TAKEOFF_CONFIRMATION_MS = 1500;
}

FlightObservation::FlightObservation()
    : vesselPresent(false), vesselId(), landed(false), groundContact(false),
      simulationTime(0), simulationTimeMilliseconds(0), locationKnown(false),
      bodyId(), nearestBaseId(), nearestBaseDistanceMetres(-1.0) {}

FlightTransitionMonitor::FlightTransitionMonitor()
    : initialized_(false), vesselId_(), wasGrounded_(false),
      takeoffCandidate_(false), takeoffCandidateSinceMs_(0),
      lastLocationKnown_(false), lastBodyId_(), lastBaseId_(),
      lastDistanceMetres_(-1.0) {}

bool FlightTransitionMonitor::observe(const FlightObservation& observation,
                                      SimulationEvent& event) {
    if (!observation.vesselPresent || observation.vesselId.empty()) {
        reset();
        return false;
    }

    if (!initialized_ || vesselId_ != observation.vesselId) {
        initialized_ = true;
        vesselId_ = observation.vesselId;
        // GroundContact alone establishes a baseline while taxiing.
        wasGrounded_ = observation.groundContact;
        if (wasGrounded_ && observation.locationKnown) {
            lastLocationKnown_ = true;
            lastBodyId_ = observation.bodyId;
            lastBaseId_ = observation.nearestBaseId;
            lastDistanceMetres_ = observation.nearestBaseDistanceMetres;
        }
        return false;
    }

    if (observation.groundContact) {
        takeoffCandidate_ = false;

        // Do not re-arm on a momentary contact bounce after liftoff.
        // A landed + contact sample is required to establish a new ground
        // cycle and permit the next takeoff event.
        if (observation.landed) wasGrounded_ = true;

        if (observation.locationKnown) {
            lastLocationKnown_ = true;
            lastBodyId_ = observation.bodyId;
            lastBaseId_ = observation.nearestBaseId;
            lastDistanceMetres_ = observation.nearestBaseDistanceMetres;
        }
        return false;
    }

    // Orbiter can briefly report no contact while the vessel is still in its
    // landed state. Start the debounce only once both signals say airborne.
    if (observation.landed || !wasGrounded_) {
        takeoffCandidate_ = false;
        return false;
    }

    if (!takeoffCandidate_) {
        takeoffCandidate_ = true;
        takeoffCandidateSinceMs_ = observation.simulationTimeMilliseconds;
        return false;
    }

    if (observation.simulationTimeMilliseconds < takeoffCandidateSinceMs_ ||
        observation.simulationTimeMilliseconds - takeoffCandidateSinceMs_ <
            TAKEOFF_CONFIRMATION_MS) {
        return false;
    }

    event = SimulationEvent();
    event.type = SIM_EVENT_TAKEOFF;
    event.vesselId = observation.vesselId;
    event.bodyId = lastBodyId_;
    event.baseId = lastBaseId_;
    event.simulationTime = observation.simulationTime;
    event.simulationTimeMilliseconds = observation.simulationTimeMilliseconds;
    event.distanceMetres = lastDistanceMetres_;
    event.locationKnown = lastLocationKnown_;

    wasGrounded_ = false;
    takeoffCandidate_ = false;
    return true;
}

void FlightTransitionMonitor::reset() {
    initialized_ = false;
    vesselId_.clear();
    wasGrounded_ = false;
    takeoffCandidate_ = false;
    takeoffCandidateSinceMs_ = 0;
    lastLocationKnown_ = false;
    lastBodyId_.clear();
    lastBaseId_.clear();
    lastDistanceMetres_ = -1.0;
}

} // namespace oem
