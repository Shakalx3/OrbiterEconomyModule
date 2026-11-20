#define STRICT
#define ORBITER_MODULE
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <map>
#include <vector>
#include "orbitersdk.h"
#include "../core/SpaceportLanding.h"
#include "../core/SurfaceDistance.h"

namespace {

class OEMLandingBridge : public oapi::Module {
public:
    explicit OEMLandingBridge(HINSTANCE hDLL) : oapi::Module(hDLL), monitors_() {}

    void clbkSimulationStart(RenderMode mode) override {
        monitors_.clear();
        oapiWriteLog((char*)"OEM Landing Bridge: simulation monitoring started.");
    }

    void clbkSimulationEnd() override {
        monitors_.clear();
        oapiWriteLog((char*)"OEM Landing Bridge: simulation monitoring stopped.");
    }

    void clbkPostStep(double simt, double simdt, double mjd) override {
        // Orbiter can invoke the first post-step callback at simt=0 while
        // vessel touchdown telemetry is still being initialized. Do not let
        // that transient sample arm a landing and create false startup events.
        // The first positive-time sample becomes the monitor's baseline.
        if (simt <= 0.0) return;

        const DWORD vesselCount = oapiGetVesselCount();
        std::vector<OBJHANDLE> seen;
        seen.reserve(vesselCount);

        for (DWORD i = 0; i < vesselCount; ++i) {
            OBJHANDLE hVessel = oapiGetVesselByIndex((int)i);
            if (!hVessel) continue;

            seen.push_back(hVessel);
            VESSEL *vessel = oapiGetVesselInterface(hVessel);
            if (!vessel) continue;

            oem::LandingObservation observation;
            observation.vesselPresent = true;
            observation.vesselId = vessel->GetName() ? vessel->GetName() : "";
            observation.simulationTime = (oem::SimulationSeconds)(simt >= 0.0 ? simt : 0.0);

            const DWORD flightStatus = vessel->GetFlightStatus();
            observation.landed = (flightStatus & 1u) != 0;
            observation.groundContact = vessel->GroundContact();

            // Base enumeration is only needed after both independent touchdown
            // signals are true. Airborne and rolling/taxiing vessels still
            // update the monitor's transition state without a base scan.
            if (observation.landed && observation.groundContact) {
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
                        observation.proximityKnown = true;
                        observation.bodyId = bodyName;

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

                        observation.nearestBaseDistanceMetres = nearestDistance;
                        observation.nearestBaseId = nearestBase;
                    }
                }
            }

            oem::SpaceportLandingEvent event;
            if (monitors_[hVessel].observe(observation, event)) {
                char message[512];
                sprintf(message,
                    "OEM SPACEPORT LANDING: vessel=%s body=%s base=%s distance=%.1f m simt=%lld",
                    event.vesselId.c_str(),
                    event.bodyId.c_str(),
                    event.baseId.c_str(),
                    event.distanceMetres,
                    event.simulationTime);
                oapiWriteLog(message);
            }
        }

        // Handles can disappear without a useful follow-up sample. Remove any
        // monitor whose vessel is no longer in Orbiter's current object list.
        for (std::map<OBJHANDLE, oem::SpaceportLandingMonitor>::iterator it = monitors_.begin();
             it != monitors_.end();) {
            bool present = false;
            for (size_t i = 0; i < seen.size(); ++i) {
                if (seen[i] == it->first) {
                    present = true;
                    break;
                }
            }
            if (!present)
                monitors_.erase(it++);
            else
                ++it;
        }
    }

    void clbkDeleteVessel(OBJHANDLE hVessel) override {
        monitors_.erase(hVessel);
    }

private:
    std::map<OBJHANDLE, oem::SpaceportLandingMonitor> monitors_;
};

}

DLLCLBK void InitModule(HINSTANCE hDLL) {
    oapiRegisterModule(new OEMLandingBridge(hDLL));
}

DLLCLBK void ExitModule(HINSTANCE hDLL) {
    // Orbiter owns and destroys registered oapi::Module instances.
}
