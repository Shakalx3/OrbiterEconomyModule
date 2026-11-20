#include "FlightTransition.h"

namespace oem {

FlightObservation::FlightObservation()
    : vesselPresent(false), vesselId(), landed(false), groundContact(false),
      simulationTime(0), simulationTimeMilliseconds(0), locationKnown(false),
      bodyId(), nearestBaseId(), nearestBaseDistanceMetres(-1.0) {}

FlightTransitionMonitor::FlightTransitionMonitor()
    : initialized_(false), vesselId_(), wasGrounded_(false),
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
        // GroundContact alone is enough to establish a surface baseline:
        // Orbiter can clear the landed flag while a vessel is taxiing.
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
        wasGrounded_ = true;
        if (observation.locationKnown) {
            lastLocationKnown_ = true;
            lastBodyId_ = observation.bodyId;
            lastBaseId_ = observation.nearestBaseId;
            lastDistanceMetres_ = observation.nearestBaseDistanceMetres;
        }
        return false;
    }

    if (wasGrounded_) {
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
        return true;
    }

    return false;
}

void FlightTransitionMonitor::reset() {
    initialized_ = false;
    vesselId_.clear();
    wasGrounded_ = false;
    lastLocationKnown_ = false;
    lastBodyId_.clear();
    lastBaseId_.clear();
    lastDistanceMetres_ = -1.0;
}

} // namespace oem
