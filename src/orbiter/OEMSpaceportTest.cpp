#define STRICT
#define ORBITER_MODULE
#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "orbitersdk.h"
#include "../core/SurfaceDistance.h"

namespace {
    const double SPACEPORT_RADIUS_METRES = 5000.0;
    int g_mode = -1;

    class OEMSpaceportMFD : public MFD {
    public:
        OEMSpaceportMFD(DWORD w, DWORD h, VESSEL *vessel) : MFD(w, h, vessel) {}

        void Update(HDC hDC) override {
            Title(hDC, "OEM SPACEPORT TEST");
            SelectDefaultFont(hDC, 0);

            int y = (int)ch + 4;
            char line[256];
            sprintf(line, "Vessel: %s", pV ? pV->GetName() : "(none)");
            TextOutA(hDC, 4, y, line, (int)strlen(line)); y += (int)ch;

            if (!pV) {
                DrawLine(hDC, y, "No vessel interface.");
                return;
            }

            const DWORD flightStatus = pV->GetFlightStatus();
            const bool landed = (flightStatus & 1u) != 0;
            DrawLine(hDC, y, landed ? "Orbiter state: LANDED" : "Orbiter state: IN FLIGHT");
            sprintf(line, "Flight status flags: 0x%08lX", (unsigned long)flightStatus);
            DrawLine(hDC, y, line);

            OBJHANDLE body = pV->GetSurfaceRef();
            if (!body) {
                DrawLine(hDC, y, "Surface reference: unavailable");
                DrawLine(hDC, y, "Spaceport result: NOT VERIFIED");
                return;
            }

            char bodyName[64] = {};
            oapiGetObjectName(body, bodyName, sizeof(bodyName));
            sprintf(line, "Surface body: %s", bodyName);
            DrawLine(hDC, y, line);

            double vesselLng = 0.0, vesselLat = 0.0, vesselRadius = 0.0;
            OBJHANDLE posBody = pV->GetEquPos(vesselLng, vesselLat, vesselRadius);
            if (!posBody || posBody != body) {
                DrawLine(hDC, y, "Vessel coordinates: unavailable/mismatched");
                DrawLine(hDC, y, "Spaceport result: NOT VERIFIED");
                return;
            }

            sprintf(line, "Position: %.6f lat, %.6f lon", vesselLat * DEG, vesselLng * DEG);
            DrawLine(hDC, y, line);

            const double bodyRadius = oapiGetSize(body);
            if (!(bodyRadius > 0.0) || !isfinite(bodyRadius)) {
                DrawLine(hDC, y, "Body radius: invalid");
                DrawLine(hDC, y, "Spaceport result: NOT VERIFIED");
                return;
            }

            const DWORD baseCount = oapiGetBaseCount(body);
            sprintf(line, "Surface bases on body: %lu", (unsigned long)baseCount);
            DrawLine(hDC, y, line);

            double nearest = -1.0;
            char nearestName[96] = "(none)";
            for (DWORD i = 0; i < baseCount; ++i) {
                OBJHANDLE base = oapiGetBaseByIndex(body, (int)i);
                if (!base) continue;

                double baseLng = 0.0, baseLat = 0.0, baseRadius = 0.0;
                oapiGetBaseEquPos(base, &baseLng, &baseLat, &baseRadius);
                const double distance = oem::surfaceDistanceMetres(
                    vesselLat, vesselLng, baseLat, baseLng, bodyRadius);
                if (distance >= 0.0 && (nearest < 0.0 || distance < nearest)) {
                    nearest = distance;
                    oapiGetObjectName(base, nearestName, sizeof(nearestName));
                }
            }

            if (nearest < 0.0) {
                DrawLine(hDC, y, "Nearest spaceport: none / coordinates unavailable");
                DrawLine(hDC, y, "Spaceport result: NOT VERIFIED");
                return;
            }

            sprintf(line, "Nearest base: %s", nearestName);
            DrawLine(hDC, y, line);
            sprintf(line, "Distance: %.1f m (limit %.0f m)", nearest, SPACEPORT_RADIUS_METRES);
            DrawLine(hDC, y, line);

            const bool withinRange = oem::isWithinSpaceportRadius(nearest, SPACEPORT_RADIUS_METRES);
            if (landed && withinRange)
                DrawLine(hDC, y, "SPACEPORT LANDING: VALID");
            else if (!landed && withinRange)
                DrawLine(hDC, y, "SPACEPORT LANDING: NOT LANDED");
            else
                DrawLine(hDC, y, "SPACEPORT LANDING: OUTSIDE 5 KM");
        }

        static int MsgProc(UINT msg, UINT mfd, WPARAM wparam, LPARAM lparam) {
            if (msg == OAPI_MSG_MFD_OPENED) {
                MFDMODEOPENSPEC *spec = (MFDMODEOPENSPEC*)wparam;
                return (int)(new OEMSpaceportMFD(spec->w, spec->h, (VESSEL*)lparam));
            }
            return 0;
        }

    private:
        static void DrawLine(HDC hDC, int &y, const char *text) {
            TextOutA(hDC, 4, y, text, (int)strlen(text));
            y += 16;
        }
    };
}

DLLCLBK void InitModule(HINSTANCE hDLL) {
    MFDMODESPECEX spec;
    spec.name = "OEM Spaceport Test";
    spec.key = OAPI_KEY_O;
    spec.context = NULL;
    spec.msgproc = OEMSpaceportMFD::MsgProc;
    g_mode = oapiRegisterMFDMode(spec);
}

DLLCLBK void ExitModule(HINSTANCE hDLL) {
    if (g_mode >= 0) oapiUnregisterMFDMode(g_mode);
    g_mode = -1;
}
