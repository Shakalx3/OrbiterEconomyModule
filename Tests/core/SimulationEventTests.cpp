#include "../../src/core/SimulationEvent.h"
#include "../../src/core/FlightTransition.h"
#include <cassert>

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

    oem::FlightTransitionMonitor monitor;
    oem::FlightObservation observation;
    observation.vesselPresent = true;
    observation.vesselId = "GL-NT";
    observation.landed = true;
    observation.groundContact = true;
    observation.locationKnown = true;
    observation.bodyId = "Earth";
    observation.nearestBaseId = "Habana";
    observation.nearestBaseDistanceMetres = 42.0;
    oem::SimulationEvent takeoff;
    assert(!monitor.observe(observation, takeoff)); // parked baseline

    observation.landed = false;
    observation.groundContact = true; // surface taxi transition
    assert(!monitor.observe(observation, takeoff));

    observation.groundContact = false;
    observation.simulationTime = 130;
    observation.simulationTimeMilliseconds = 130500;
    assert(monitor.observe(observation, takeoff));
    assert(takeoff.type == oem::SIM_EVENT_TAKEOFF);
    assert(takeoff.vesselId == "GL-NT");
    assert(takeoff.bodyId == "Earth");
    assert(takeoff.baseId == "Habana");
    assert(takeoff.simulationTimeMilliseconds == 130500);

    assert(!monitor.observe(observation, takeoff)); // no duplicate
    return 0;
}
