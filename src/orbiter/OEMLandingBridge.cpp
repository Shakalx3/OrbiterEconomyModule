#define STRICT
#define ORBITER_MODULE
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <map>
#include <vector>
#include <string>
#include "orbitersdk.h"
#include "../core/SpaceportLanding.h"
#include "../core/FlightTransition.h"
#include "../core/SimulationEvent.h"
#include "../core/SurfaceDistance.h"

namespace {

class OEMLandingBridge : public oapi::Module {
public:
    explicit OEMLandingBridge(HINSTANCE hDLL)
        : oapi::Module(hDLL), landingMonitors_(), flightMonitors_(),
          eventQueue_(), sessionId_(), lastFocus_(0) {}

    void clbkSimulationStart(RenderMode mode) override {
        landingMonitors_.clear();
        flightMonitors_.clear();
        eventQueue_ = oem::SimulationEventQueue();
        sessionId_.clear();
        lastFocus_ = 0;
        oapiWriteLog((char*)"OEM Landing Bridge: simulation monitoring started.");
    }

    void clbkSimulationEnd() override {
        landingMonitors_.clear();
        flightMonitors_.clear();
        eventQueue_ = oem::SimulationEventQueue();
        sessionId_.clear();
        lastFocus_ = 0;
        oapiWriteLog((char*)"OEM Landing Bridge: simulation monitoring stopped.");
    }

    void clbkPostStep(double simt, double simdt, double mjd) override {
        // The first positive-time sample is the baseline. simt=0 telemetry can
        // contain transient vessel states during scenario initialization.
        if (simt <= 0.0) return;

        if (sessionId_.empty()) {
            const double startMjd = mjd - simt / 86400.0;
            char session[64];
            sprintf(session, "%.0f", startMjd * 86400000.0);
            sessionId_ = session;
        }

        const long long simMilliseconds = (long long)(simt * 1000.0 + 0.5);
        const oem::SimulationSeconds simSeconds =
            (oem::SimulationSeconds)(simt >= 0.0 ? simt : 0.0);

        // OEM follows only Orbiter's current focus vessel. A focus change
        // establishes a fresh baseline so inactive vessels cannot generate events.
        OBJHANDLE hVessel = oapiGetFocusObject();
        if (!hVessel || !oapiIsVessel(hVessel)) return;

        if (hVessel != lastFocus_) {
            landingMonitors_.clear();
            flightMonitors_.clear();
            lastFocus_ = hVessel;
        }

        VESSEL *vessel = oapiGetVesselInterface(hVessel);
        if (!vessel) return;

            oem::LandingObservation landing;
            landing.vesselPresent = true;
            landing.vesselId = vessel->GetName() ? vessel->GetName() : "";
            landing.simulationTime = simSeconds;

            const DWORD flightStatus = vessel->GetFlightStatus();
            landing.landed = (flightStatus & 1u) != 0;
            landing.groundContact = vessel->GroundContact();

            // Location is sampled whenever there is actual surface contact. This
            // also captures taxiing vessels whose landed flag is already clear.
            // The landing monitor still requires both landed and contact.
            if (landing.groundContact) {
                OBJHANDLE body = vessel->GetSurfaceRef();
                if (body) {
                    char bodyName[128] = {};
                    oapiGetObjectName(body, bodyName, sizeof(bodyName));

                    double vesselLng = 0.0, vesselLat = 0.0, vesselRadius = 0.0;
                    OBJHANDLE positionBody = vessel->GetEquPos(vesselLng, vesselLat, vesselRadius);
                    const double bodyRadius = oapiGetSize(body);

                    if (positionBody == body && bodyName[0] != '\0' &&
                        bodyRadius > 0.0 && _finite(bodyRadius) &&
                        _finite(vesselLat) && _finite(vesselLng)) {
                        landing.proximityKnown = true;
                        landing.bodyId = bodyName;

                        const DWORD baseCount = oapiGetBaseCount(body);
                        double nearestDistance = -1.0;
                        char nearestBase[128] = {};

                        for (DWORD baseIndex = 0; baseIndex < baseCount; ++baseIndex) {
                            OBJHANDLE hBase = oapiGetBaseByIndex(body, (int)baseIndex);
                            if (!hBase) continue;

                            double baseLng = 0.0, baseLat = 0.0, baseRadius = 0.0;
                            oapiGetBaseEquPos(hBase, &baseLng, &baseLat, &baseRadius);
                            const double distance = oem::surfaceDistanceMetres(
                                vesselLat, vesselLng, baseLat, baseLng, bodyRadius);

                            if (distance >= 0.0 &&
                                (nearestDistance < 0.0 || distance < nearestDistance)) {
                                nearestDistance = distance;
                                oapiGetObjectName(hBase, nearestBase, sizeof(nearestBase));
                            }
                        }

                        landing.nearestBaseDistanceMetres = nearestDistance;
                        landing.nearestBaseId = nearestBase;
                    }
                }
            }

            // Publish a takeoff only after a grounded baseline followed by
            // actual loss of GroundContact. Taxi transitions are not takeoffs.
            oem::FlightObservation flight;
            flight.vesselPresent = true;
            flight.vesselId = landing.vesselId;
            flight.landed = landing.landed;
            flight.groundContact = landing.groundContact;
            flight.simulationTime = simSeconds;
            flight.simulationTimeMilliseconds = simMilliseconds;
            flight.locationKnown = landing.proximityKnown;
            flight.bodyId = landing.bodyId;
            flight.nearestBaseId = landing.nearestBaseId;
            flight.nearestBaseDistanceMetres = landing.nearestBaseDistanceMetres;

            oem::SimulationEvent takeoff;
            if (flightMonitors_[hVessel].observe(flight, takeoff)) {
                takeoff.eventId = oem::makeSimulationEventId(
                    sessionId_, takeoff.type, takeoff.vesselId,
                    takeoff.simulationTimeMilliseconds);
                publishEvent(takeoff);
            }

            oem::SpaceportLandingEvent landingEvent;
            if (landingMonitors_[hVessel].observe(landing, landingEvent)) {
                oem::SimulationEvent event;
                event.type = oem::SIM_EVENT_LANDING;
                event.vesselId = landingEvent.vesselId;
                event.bodyId = landingEvent.bodyId;
                event.baseId = landingEvent.baseId;
                event.distanceMetres = landingEvent.distanceMetres;
                event.locationKnown = true;
                event.simulationTime = landingEvent.simulationTime;
                event.simulationTimeMilliseconds = simMilliseconds;
                event.eventId = oem::makeSimulationEventId(
                    sessionId_, event.type, event.vesselId,
                    event.simulationTimeMilliseconds);

                if (publishEvent(event)) {
                    char message[512];
                    sprintf(message,
                        "OEM SPACEPORT LANDING: vessel=%s body=%s base=%s distance=%.1f m simt=%lld",
                        landingEvent.vesselId.c_str(),
                        landingEvent.bodyId.c_str(),
                        landingEvent.baseId.c_str(),
                        landingEvent.distanceMetres,
                        landingEvent.simulationTime);
                    oapiWriteLog(message);
                }
            }
    }

    void clbkDeleteVessel(OBJHANDLE hVessel) override {
        landingMonitors_.erase(hVessel);
        flightMonitors_.erase(hVessel);
    }

private:
    bool publishEvent(const oem::SimulationEvent& event) {
        const bool duplicate = eventQueue_.contains(event.eventId);
        const oem::Result result = eventQueue_.publish(event);
        if (!result.ok || duplicate) return false;

        char message[768];
        sprintf(message,
            "OEM EVENT QUEUED: id=%s type=%s vessel=%s body=%s base=%s simt=%lld",
            event.eventId.c_str(),
            oem::simulationEventTypeName(event.type),
            event.vesselId.c_str(),
            event.bodyId.c_str(),
            event.baseId.c_str(),
            event.simulationTime);
        oapiWriteLog(message);
        return true;
    }

    std::map<OBJHANDLE, oem::SpaceportLandingMonitor> landingMonitors_;
    std::map<OBJHANDLE, oem::FlightTransitionMonitor> flightMonitors_;
    oem::SimulationEventQueue eventQueue_;
    std::string sessionId_;
    OBJHANDLE lastFocus_;
};

}

DLLCLBK void InitModule(HINSTANCE hDLL) {
    oapiRegisterModule(new OEMLandingBridge(hDLL));
}

DLLCLBK void ExitModule(HINSTANCE hDLL) {
    // Orbiter owns and destroys registered oapi::Module instances.
}
