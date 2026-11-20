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
    oem::SpaceportLandingEvent event;

    // A vessel already parked when monitoring begins establishes a baseline.
    oem::SpaceportLandingMonitor parkedMonitor;
    oem::LandingObservation observation = makeObservation(true, true, true, 60.1);
    assert(!parkedMonitor.observe(observation, event));
    observation.simulationTime = 101;
    assert(!parkedMonitor.observe(observation, event));

    // A vessel already rolling on the surface also establishes a baseline.
    // GroundContact=true means it is not airborne, even if landed=false.
    oem::SpaceportLandingMonitor taxiMonitor;
    observation = makeObservation(false, true, true, 60.1);
    assert(!taxiMonitor.observe(observation, event));
    observation = makeObservation(true, true, true, 60.1);
    assert(!taxiMonitor.observe(observation, event));

    // An airborne vessel arms the next touchdown.
    oem::SpaceportLandingMonitor monitor;
    observation = makeObservation(false, false, false, -1.0);
    assert(!monitor.observe(observation, event));

    // Surface contact can begin before Orbiter sets its landed flag.
    observation = makeObservation(false, true, false, -1.0);
    assert(!monitor.observe(observation, event));

    // Incomplete location telemetry waits; it must not accept or reject.
    observation = makeObservation(true, true, false, -1.0);
    assert(!monitor.observe(observation, event));

    // A completed search with no matching base rejects and disarms this
    // touchdown; moving into range without taking off is not a new landing.
    observation = makeObservation(true, true, true, -1.0);
    observation.nearestBaseId.clear();
    assert(!monitor.observe(observation, event));
    observation = makeObservation(true, true, true, 60.1);
    assert(!monitor.observe(observation, event));

    // A new loss of contact followed by a touchdown exactly 5,000 m away
    // produces one event.
    observation = makeObservation(false, false, false, -1.0);
    assert(!monitor.observe(observation, event));
    observation = makeObservation(false, true, false, -1.0);
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

    // A second airborne -> touchdown cycle is a new event.
    observation = makeObservation(false, false, false, -1.0);
    assert(!monitor.observe(observation, event));
    observation = makeObservation(true, true, true, 4999.0);
    observation.simulationTime = 130;
    assert(monitor.observe(observation, event));
    assert(std::fabs(event.distanceMetres - 4999.0) < 1e-9);

    // A different vessel gets independent state and starts with its own baseline.
    oem::SpaceportLandingMonitor secondVessel;
    observation = makeObservation(true, true, true, 10.0);
    observation.vesselId = "TestShip";
    assert(!secondVessel.observe(observation, event));
    observation = makeObservation(false, false, false, -1.0);
    observation.vesselId = "TestShip";
    assert(!secondVessel.observe(observation, event));
    observation = makeObservation(true, true, true, 10.0);
    observation.vesselId = "TestShip";
    observation.simulationTime = 210;
    assert(secondVessel.observe(observation, event));
    assert(event.vesselId == "TestShip");

    monitor.reset();
    assert(!monitor.observe(observation, event));
    return 0;
}
