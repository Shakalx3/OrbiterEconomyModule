#include "../../src/core/SpaceportLanding.h"
#include <cassert>
#include <cmath>

static oem::LandingObservation makeObservation(bool landed,
                                                 bool groundContact,
                                                 bool proximityKnown,
                                                 double distance) {
    oem::LandingObservation o;
    o.vesselPresent = true;
    o.vesselId = "GL-NT";
    o.simulationTime = 100;
    o.landed = landed;
    o.groundContact = groundContact;
    o.proximityKnown = proximityKnown;
    o.bodyId = proximityKnown ? "Earth" : "";
    o.nearestBaseId = proximityKnown ? "Habana" : "";
    o.nearestBaseDistanceMetres = distance;
    return o;
}

int main() {
    oem::SpaceportLandingMonitor monitor;
    oem::SpaceportLandingEvent event;

    // A vessel already parked when monitoring begins establishes a baseline.
    oem::LandingObservation observation = makeObservation(true, true, true, 60.1);
    assert(!monitor.observe(observation, event));
    observation.simulationTime = 101;
    assert(!monitor.observe(observation, event));

    // A takeoff arms one future touchdown.
    observation = makeObservation(false, false, true, 60.1);
    observation.simulationTime = 110;
    assert(!monitor.observe(observation, event));

    // A fly-over, landed flag alone, or contact flag alone is insufficient.
    observation = makeObservation(false, false, true, 60.1);
    assert(!monitor.observe(observation, event));
    observation = makeObservation(true, false, true, 60.1);
    assert(!monitor.observe(observation, event));
    observation = makeObservation(false, true, true, 60.1);
    assert(!monitor.observe(observation, event));

    // After a real touchdown, incomplete location telemetry waits for evidence.
    observation = makeObservation(true, true, false, -1.0);
    assert(!monitor.observe(observation, event));

    // A completed search with no matching base rejects and disarms this
    // touchdown; merely moving into range while still landed is not arrival.
    observation = makeObservation(true, true, true, -1.0);
    observation.nearestBaseId.clear();
    assert(!monitor.observe(observation, event));
    observation = makeObservation(true, true, true, 60.1);
    assert(!monitor.observe(observation, event));

    // A new takeoff followed by a touchdown exactly 5,000 m away is accepted.
    observation = makeObservation(false, false, true, 60.1);
    assert(!monitor.observe(observation, event));
    observation = makeObservation(true, true, true, 5000.0);
    observation.simulationTime = 120;
    assert(monitor.observe(observation, event));
    assert(event.vesselId == "GL-NT");
    assert(event.bodyId == "Earth");
    assert(event.baseId == "Habana");
    assert(std::fabs(event.distanceMetres - 5000.0) < 1e-9);
    assert(event.simulationTime == 120);

    // Repeated frames and movement across the geofence cannot duplicate it.
    observation.simulationTime = 121;
    assert(!monitor.observe(observation, event));
    observation = makeObservation(true, true, true, 7000.0);
    assert(!monitor.observe(observation, event));
    observation = makeObservation(true, true, true, 20.0);
    assert(!monitor.observe(observation, event));

    // A second takeoff and landing is a new event.
    observation = makeObservation(false, false, true, 20.0);
    assert(!monitor.observe(observation, event));
    observation = makeObservation(true, true, true, 4999.0);
    observation.simulationTime = 130;
    assert(monitor.observe(observation, event));
    assert(std::fabs(event.distanceMetres - 4999.0) < 1e-9);

    // A different vessel starts with its own baseline and state.
    observation.vesselId = "TestShip";
    observation.simulationTime = 200;
    assert(!monitor.observe(observation, event));
    observation = makeObservation(false, false, true, 7000.0);
    observation.vesselId = "TestShip";
    assert(!monitor.observe(observation, event));
    observation = makeObservation(true, true, true, 7000.0);
    observation.vesselId = "TestShip";
    assert(!monitor.observe(observation, event));
    observation.nearestBaseDistanceMetres = 10.0;
    assert(!monitor.observe(observation, event));

    // Only another airborne -> touchdown cycle can produce its event.
    observation = makeObservation(false, false, true, 10.0);
    observation.vesselId = "TestShip";
    assert(!monitor.observe(observation, event));
    observation = makeObservation(true, true, true, 10.0);
    observation.vesselId = "TestShip";
    observation.simulationTime = 210;
    assert(monitor.observe(observation, event));
    assert(event.vesselId == "TestShip");

    monitor.reset();
    assert(!monitor.observe(observation, event));
    return 0;
}
