#include "../../src/core/SimulationEvent.h"
#include "../../src/core/FlightTransition.h"
#include <cassert>

static oem::FlightObservation surfaceObservation(const char* vessel,
                                                  bool landed,
                                                  bool contact,
                                                  long long simMs = 0) {
    oem::FlightObservation o;
    o.vesselPresent = true;
    o.vesselId = vessel;
    o.landed = landed;
    o.groundContact = contact;
    o.simulationTimeMilliseconds = simMs;
    o.simulationTime = simMs / 1000;
    o.locationKnown = contact;
    o.bodyId = contact ? "Earth" : "";
    o.nearestBaseId = contact ? "Habana" : "";
    o.nearestBaseDistanceMetres = contact ? 42.0 : -1.0;
    return o;
}

int main() {
    oem::SimulationEventQueue queue;
    oem::SimulationEvent event;
    event.eventId = oem::makeSimulationEventId("session-1", oem::SIM_EVENT_LANDING,
                                               "GL-NT", 125000);
    event.type = oem::SIM_EVENT_LANDING;
    event.vesselId = "GL-NT";
    event.bodyId = "Earth";
    event.baseId = "Habana";
    event.simulationTime = 125;
    event.simulationTimeMilliseconds = 125000;
    event.locationKnown = true;
    event.distanceMetres = 42.0;

    assert(queue.publish(event).ok);
    assert(queue.events().size() == 1);
    assert(queue.publish(event).ok);
    assert(queue.events().size() == 1);

    oem::SimulationEvent takeoff;

    // Parked baseline; brief loss of contact during liftoff is ignored.
    oem::FlightTransitionMonitor parked;
    oem::FlightObservation observation = surfaceObservation("GL-NT", true, true, 100000);
    assert(!parked.observe(observation, takeoff));
    observation = surfaceObservation("GL-NT", false, false, 101000);
    assert(!parked.observe(observation, takeoff)); // candidate begins
    observation = surfaceObservation("GL-NT", false, true, 101500);
    assert(!parked.observe(observation, takeoff)); // contact bounce cancels it
    observation = surfaceObservation("GL-NT", false, false, 102000);
    assert(!parked.observe(observation, takeoff)); // new candidate
    observation = surfaceObservation("GL-NT", false, false, 103499);
    assert(!parked.observe(observation, takeoff)); // just short of confirmation
    observation = surfaceObservation("GL-NT", false, false, 103500);
    assert(parked.observe(observation, takeoff));
    assert(takeoff.type == oem::SIM_EVENT_TAKEOFF);
    assert(takeoff.vesselId == "GL-NT");
    assert(takeoff.bodyId == "Earth");
    assert(takeoff.baseId == "Habana");
    assert(takeoff.simulationTimeMilliseconds == 103500);
    assert(!parked.observe(observation, takeoff)); // no duplicate

    // Contact bounce after confirmed liftoff must not re-arm takeoff.
    observation = surfaceObservation("GL-NT", false, true, 104000);
    assert(!parked.observe(observation, takeoff));
    observation = surfaceObservation("GL-NT", false, false, 105000);
    assert(!parked.observe(observation, takeoff));

    // A new landed+contact state permits a later departure cycle.
    observation = surfaceObservation("GL-NT", true, true, 110000);
    assert(!parked.observe(observation, takeoff));
    observation = surfaceObservation("GL-NT", false, false, 111000);
    assert(!parked.observe(observation, takeoff));
    observation = surfaceObservation("GL-NT", false, false, 112500);
    assert(parked.observe(observation, takeoff));

    // A vessel already taxiing when monitoring starts is a ground baseline.
    oem::FlightTransitionMonitor taxi;
    observation = surfaceObservation("TaxiShip", false, true, 50000);
    assert(!taxi.observe(observation, takeoff));
    observation = surfaceObservation("TaxiShip", false, false, 51000);
    assert(!taxi.observe(observation, takeoff));
    observation = surfaceObservation("TaxiShip", false, false, 52500);
    assert(taxi.observe(observation, takeoff));
    assert(takeoff.vesselId == "TaxiShip");
    assert(takeoff.bodyId == "Earth");
    assert(takeoff.baseId == "Habana");
    return 0;
}
