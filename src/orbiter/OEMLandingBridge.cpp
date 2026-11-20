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
#include "../core/SimulationEventJournal.h"
#include "../core/SurfaceDistance.h"

namespace {

class OEMLandingBridge;
OEMLandingBridge *g_bridge = 0;
int g_flightLogMode = -1;

std::string makeJournalPath(HINSTANCE module) {
    char modulePath[MAX_PATH] = {};
    const DWORD length = GetModuleFileNameA(module, modulePath, MAX_PATH);
    if (length == 0 || length >= MAX_PATH) return std::string();

    char *separator = strrchr(modulePath, '\\');
    if (!separator) separator = strrchr(modulePath, '/');
    if (!separator) return std::string();
    *separator = '\0';

    char dataDirectory[MAX_PATH] = {};
    sprintf(dataDirectory, "%s\\OEMData", modulePath);
    if (!CreateDirectoryA(dataDirectory, 0) &&
        GetLastError() != ERROR_ALREADY_EXISTS)
        return std::string();

    char journalPath[MAX_PATH] = {};
    sprintf(journalPath, "%s\\events.journal", dataDirectory);
    return journalPath;
}

class OEMLandingBridge : public oapi::Module {
public:
    explicit OEMLandingBridge(HINSTANCE hDLL)
        : oapi::Module(hDLL), landingMonitors_(), flightMonitors_(),
          eventQueue_(), journalStore_(), journalPath_(makeJournalPath(hDLL)),
          journalReady_(false), sessionId_(), lastFocus_(0) {
        g_bridge = this;
    }

    const std::vector<oem::SimulationEvent>& events() const {
        return eventQueue_.events();
    }

    bool journalReady() const { return journalReady_; }

    void clbkSimulationStart(RenderMode mode) override {
        landingMonitors_.clear();
        flightMonitors_.clear();
        sessionId_.clear();
        lastFocus_ = 0;
        oem::SimulationEventQueue restored;
        const oem::Result loaded = journalPath_.empty()
            ? oem::Result::failure("JOURNAL_PATH", "Could not resolve module data directory.")
            : journalStore_.load(journalPath_, restored);
        journalReady_ = loaded.ok;
        if (journalReady_) {
            eventQueue_ = restored;
            char message[256];
            sprintf(message, "OEM Event Journal: loaded %lu records.",
                    (unsigned long)eventQueue_.events().size());
            oapiWriteLog(message);
        } else {
            char message[512];
            sprintf(message, "OEM Event Journal: load failed (%s): %s",
                    loaded.code.c_str(), loaded.message.c_str());
            oapiWriteLog(message);
        }
        oapiWriteLog((char*)"OEM Landing Bridge: simulation monitoring started.");
    }

    void clbkSimulationEnd() override {
        landingMonitors_.clear();
        flightMonitors_.clear();
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
            sprintf(session, "%.0f-%lu-%lu", startMjd * 86400000.0,
                    (unsigned long)GetCurrentProcessId(),
                    (unsigned long)GetTickCount());
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
        if (!journalReady_ || eventQueue_.contains(event.eventId)) return false;

        oem::SimulationEventQueue candidate = eventQueue_;
        const oem::Result queued = candidate.publish(event);
        if (!queued.ok) return false;

        const oem::Result saved = journalStore_.save(journalPath_, candidate);
        if (!saved.ok) {
            char error[512];
            sprintf(error, "OEM Event Journal: save failed (%s): %s",
                    saved.code.c_str(), saved.message.c_str());
            oapiWriteLog(error);
            return false;
        }
        eventQueue_ = candidate;

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
    oem::SimulationEventJournalStore journalStore_;
    std::string journalPath_;
    bool journalReady_;
    std::string sessionId_;
    OBJHANDLE lastFocus_;
};

class OEMFlightLogMFD : public MFD {
public:
    OEMFlightLogMFD(DWORD w, DWORD h, VESSEL *vessel)
        : MFD(w, h, vessel), page_(0) {}

    void Update(HDC hDC) override {
        Title(hDC, "OEM FLIGHT LOG");
        SelectDefaultFont(hDC, 0);

        int y = (int)ch + 3;
        if (!g_bridge || !g_bridge->journalReady()) {
            DrawLine(hDC, y, "Event journal unavailable.");
            DrawLine(hDC, y, "Check OEM Event Journal in Orbiter.log.");
            return;
        }

        const std::vector<oem::SimulationEvent>& events = g_bridge->events();
        const std::size_t count = events.size();
        char line[256];
        if (count == 0) {
            DrawLine(hDC, y, "No flight events recorded.");
            DrawLine(hDC, y, "Takeoff and landing events");
            DrawLine(hDC, y, "will appear here.");
            return;
        }

        int availableLines = ((int)H - y - (int)ch) / (int)ch;
        if (availableLines < 2) availableLines = 2;
        const int perPage = availableLines / 2;
        const int pageCount = (int)((count + (std::size_t)perPage - 1) /
                                    (std::size_t)perPage);
        if (page_ >= pageCount) page_ = pageCount - 1;
        if (page_ < 0) page_ = 0;

        sprintf(line, "Events: %lu   Page: %d/%d",
                (unsigned long)count, page_ + 1, pageCount);
        DrawLine(hDC, y, line);

        const std::size_t newestOffset = (std::size_t)page_ * (std::size_t)perPage;
        for (int row = 0; row < perPage; ++row) {
            const std::size_t offset = newestOffset + (std::size_t)row;
            if (offset >= count) break;

            const oem::SimulationEvent& event = events[count - 1 - offset];
            const long long totalSeconds = event.simulationTime < 0 ? 0 : event.simulationTime;
            const long long hours = totalSeconds / 3600;
            const long long minutes = (totalSeconds / 60) % 60;
            const long long seconds = totalSeconds % 60;
            const char *type = oem::simulationEventTypeName(event.type);
            sprintf(line, "%s T+%02lld:%02lld:%02lld",
                    type, hours, minutes, seconds);
            DrawLine(hDC, y, line);

            if (event.locationKnown) {
                sprintf(line, "%.15s @ %.9s/%.9s",
                        event.vesselId.c_str(),
                        event.bodyId.c_str(),
                        event.baseId.c_str());
            } else {
                sprintf(line, "%.15s @ location unknown", event.vesselId.c_str());
            }
            DrawLine(hDC, y, line);
        }
    }

    bool ConsumeKeyBuffered(DWORD key) override {
        if (!g_bridge) return false;
        const std::size_t count = g_bridge->events().size();
        int availableLines = ((int)H - ((int)ch + 3) - (int)ch) / (int)ch;
        if (availableLines < 2) availableLines = 2;
        int perPage = availableLines / 2;
        if (perPage < 1) perPage = 1;
        const int pages = count == 0 ? 1 :
            (int)((count + (std::size_t)perPage - 1) / (std::size_t)perPage);

        switch (key) {
        case OAPI_KEY_P:
            if (page_ + 1 < pages) ++page_;
            return true;
        case OAPI_KEY_N:
            if (page_ > 0) --page_;
            return true;
        case OAPI_KEY_T:
            page_ = 0;
            return true;
        default:
            return false;
        }
    }

    bool ConsumeButton(int bt, int event) override {
        if (!(event & PANEL_MOUSE_LBDOWN)) return false;
        static const DWORD keys[3] = { OAPI_KEY_P, OAPI_KEY_N, OAPI_KEY_T };
        return bt >= 0 && bt < 3 ? ConsumeKeyBuffered(keys[bt]) : false;
    }

    char *ButtonLabel(int bt) override {
        static char *labels[3] = { (char*)"PRE", (char*)"NXT", (char*)"NEW" };
        return bt >= 0 && bt < 3 ? labels[bt] : 0;
    }

    int ButtonMenu(const MFDBUTTONMENU **menu) const override {
        static const MFDBUTTONMENU items[3] = {
            { "Older events", 0, 'P' },
            { "Newer events", 0, 'N' },
            { "Latest events", 0, 'T' }
        };
        if (menu) *menu = items;
        return 3;
    }

    static int MsgProc(UINT msg, UINT mfd, WPARAM wparam, LPARAM lparam) {
        if (msg == OAPI_MSG_MFD_OPENEDEX && g_bridge) {
            MFDMODEOPENSPEC *spec = (MFDMODEOPENSPEC*)wparam;
            return (int)(new OEMFlightLogMFD(
                spec->w, spec->h, (VESSEL*)lparam));
        }
        return 0;
    }

private:
    void DrawLine(HDC hDC, int &y, const char *text) {
        TextOutA(hDC, 4, y, text, (int)strlen(text));
        y += (int)ch;
    }

    int page_;
};

} // namespace

DLLCLBK void InitModule(HINSTANCE hDLL) {
    oapiRegisterModule(new OEMLandingBridge(hDLL));

    MFDMODESPECEX spec;
    spec.name = "OEM Flight Log";
    spec.key = OAPI_KEY_L;
    spec.context = NULL;
    spec.msgproc = OEMFlightLogMFD::MsgProc;
    g_flightLogMode = oapiRegisterMFDMode(spec);
}

DLLCLBK void ExitModule(HINSTANCE hDLL) {
    if (g_flightLogMode >= 0) oapiUnregisterMFDMode(g_flightLogMode);
    g_flightLogMode = -1;
    g_bridge = 0;
    // Orbiter owns and destroys registered oapi::Module instances.
}
