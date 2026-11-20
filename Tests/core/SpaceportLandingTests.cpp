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

    // Repeated samples do not duplicate the landing event.
    observation.simulationTime = 101;
    assert(!monitor.observe(observation, event));

    // Leaving the ground is authoritative negative evidence.
    observation = makeObservation(false, false, true, 60.1);
    observation.simulationTime = 110;
    assert(!monitor.observe(observation, event));

    // A fly-over within 5 km is not a landing.
    observation = makeObservation(false, false, true, 60.1);
    assert(!monitor.observe(observation, event));

    // Landed status without actual touchdown contact is not a landing.
    observation = makeObservation(true, false, true, 60.1);
    assert(!monitor.observe(observation, event));

    // Ground contact without Orbiter's landed state is not a landing.
    observation = makeObservation(false, true, true, 60.1);
    assert(!monitor.observe(observation, event));

    // Missing proximity evidence is unknown, not a fabricated rejection or
    // acceptance. A later complete observation can still be evaluated.
    observation = makeObservation(true, true, false, -1.0);
    assert(!monitor.observe(observation, event));

    observation = makeObservation(true, true, true, 5000.0);
    observation.simulationTime = 120;
    assert(monitor.observe(observation, event));
    assert(event.vesselId == "GL-NT");
    assert(event.bodyId == "Earth");
    assert(event.baseId == "Habana");
    assert(std::fabs(event.distanceMetres - 5000.0) < 1e-9);
    assert(event.simulationTime == 120);

    // Exactly one event for a continuous landing.
    observation.simulationTime = 121;
    assert(!monitor.observe(observation, event));

    // Moving beyond the inclusive 5 km boundary ends qualification.
    observation = makeObservation(true, true, true, 5000.01);
    assert(!monitor.observe(observation, event));

    // Returning inside range is a new verified landing transition.
    observation = makeObservation(true, true, true, 4999.0);
    observation.simulationTime = 130;
    assert(monitor.observe(observation, event));
    assert(std::fabs(event.distanceMetres - 4999.0) < 1e-9);

    // A new vessel gets its own baseline and cannot inherit the old state.
    observation.vesselId = "TestShip";
    observation.simulationTime = 200;
    assert(!monitor.observe(observation, event));

    // A known no-base / out-of-range result disqualifies the vessel.
    observation.vesselId = "TestShip";
    observation.nearestBaseDistanceMetres = 7000.0;
    assert(!monitor.observe(observation, event));
    observation.nearestBaseDistanceMetres = 10.0;
    observation.simulationTime = 210;
    assert(monitor.observe(observation, event));
    assert(event.vesselId == "TestShip");

    monitor.reset();
    assert(!monitor.observe(observation, event));
    return 0;
}
