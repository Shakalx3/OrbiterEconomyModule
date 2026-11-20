#define STRICT
#define ORBITER_MODULE
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "orbitersdk.h"

namespace {
    int g_mode = -1;

    class OEMGroundContactMFD : public MFD {
    public:
        OEMGroundContactMFD(DWORD w, DWORD h, VESSEL *vessel) : MFD(w, h, vessel) {}

        void Update(HDC hDC) override {
            Title(hDC, "OEM GROUND CONTACT TEST");
            SelectDefaultFont(hDC, 0);

            int y = (int)ch + 4;
            char line[256];
            sprintf(line, "Vessel: %s", pV ? pV->GetName() : "(none)");
            DrawLine(hDC, y, line);

            if (!pV) {
                DrawLine(hDC, y, "No vessel interface.");
                return;
            }

            const DWORD flightStatus = pV->GetFlightStatus();
            const bool landed = (flightStatus & 1u) != 0;
            const bool groundContact = pV->GroundContact();

            sprintf(line, "Flight status flags: 0x%08lX", (unsigned long)flightStatus);
            DrawLine(hDC, y, line);
            DrawLine(hDC, y, landed ? "Orbiter landed flag: TRUE" : "Orbiter landed flag: FALSE");
            DrawLine(hDC, y, groundContact ? "VESSEL::GroundContact(): TRUE" : "VESSEL::GroundContact(): FALSE");

            if (landed && groundContact)
                DrawLine(hDC, y, "TOUCHDOWN STATE: CONFIRMED");
            else if (landed)
                DrawLine(hDC, y, "TOUCHDOWN STATE: LANDED FLAG ONLY");
            else if (groundContact)
                DrawLine(hDC, y, "TOUCHDOWN STATE: CONTACT ONLY");
            else
                DrawLine(hDC, y, "TOUCHDOWN STATE: AIRBORNE");

            OBJHANDLE body = pV->GetSurfaceRef();
            if (body) {
                char bodyName[64] = {};
                oapiGetObjectName(body, bodyName, sizeof(bodyName));
                sprintf(line, "Surface body: %s", bodyName);
                DrawLine(hDC, y, line);
                sprintf(line, "Altitude: %.2f m", pV->GetAltitude());
                DrawLine(hDC, y, line);
            } else {
                DrawLine(hDC, y, "Surface body: unavailable");
            }
        }

        static int MsgProc(UINT msg, UINT mfd, WPARAM wparam, LPARAM lparam) {
            if (msg == OAPI_MSG_MFD_OPENEDEX) {
                MFDMODEOPENSPEC *spec = (MFDMODEOPENSPEC*)wparam;
                return (int)(new OEMGroundContactMFD(spec->w, spec->h, (VESSEL*)lparam));
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
    MFDMODESPECEX spec = {};
    spec.name = "OEM Ground Contact Test";
    spec.key = OAPI_KEY_G;
    spec.context = NULL;
    spec.msgproc = OEMGroundContactMFD::MsgProc;
    g_mode = oapiRegisterMFDMode(spec);
}

DLLCLBK void ExitModule(HINSTANCE hDLL) {
    if (g_mode >= 0) oapiUnregisterMFDMode(g_mode);
    g_mode = -1;
}
