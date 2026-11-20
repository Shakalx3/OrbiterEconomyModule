#include "../../src/core/SimulationEvent.h"
#include "../../src/core/FlightTransition.h"
#include <cassert>

static oem::FlightObservation surfaceObservation(const char* vessel,
                                                  bool landed,
                                                  bool contact) {
    oem::FlightObservation o;
    o.vesselPresent = true;
    o.vesselId = vessel;
    o.landed = landed;
    o.groundContact = contact;
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

    oem::SimulationEvent conflicting = event;
    conflicting.baseId = "Brighton Beach";
    assert(!queue.publish(conflicting).ok);
    assert(queue.events().size() == 1);

    oem::SimulationEvent takeoff;

    // A parked vessel establishes a baseline; taxiing does not count as takeoff.
    oem::FlightTransitionMonitor parked;
    oem::FlightObservation observation = surfaceObservation("GL-NT", true, true);
    assert(!parked.observe(observation, takeoff));
    observation.landed = false;
    assert(!parked.observe(observation, takeoff));
    observation.groundContact = false;
    observation.simulationTime = 130;
    observation.simulationTimeMilliseconds = 130500;
    assert(parked.observe(observation, takeoff));
    assert(takeoff.type == oem::SIM_EVENT_TAKEOFF);
    assert(takeoff.vesselId == "GL-NT");
    assert(takeoff.bodyId == "Earth");
    assert(takeoff.baseId == "Habana");
    assert(takeoff.simulationTimeMilliseconds == 130500);
    assert(!parked.observe(observation, takeoff)); // no duplicate

    // A vessel already taxiing when monitoring starts is also a valid
    // ground-contact baseline even though Orbiter's landed flag is clear.
    oem::FlightTransitionMonitor taxi;
    observation = surfaceObservation("TaxiShip", false, true);
    assert(!taxi.observe(observation, takeoff));
    observation.groundContact = false;
    observation.locationKnown = false;
    observation.bodyId.clear();
    observation.nearestBaseId.clear();
    observation.simulationTime = 50;
    observation.simulationTimeMilliseconds = 50000;
    assert(taxi.observe(observation, takeoff));
    assert(takeoff.vesselId == "TaxiShip");
    assert(takeoff.bodyId == "Earth");
    assert(takeoff.baseId == "Habana");
    return 0;
}
