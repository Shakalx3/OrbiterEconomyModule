#define STRICT
#define ORBITER_MODULE
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <map>
#include <set>
#include <vector>
#include <string>
#include "orbitersdk.h"
#include "../core/SpaceportLanding.h"
#include "../core/FlightTransition.h"
#include "../core/DockingTransition.h"
#include "../core/SimulationEvent.h"
#include "../core/SimulationEventJournal.h"
#include "../core/SurfaceDistance.h"
#include "../core/CargoServiceRegistry.h"
#include "../core/CargoManifest.h"
#include "../core/CargoTransferGate.h"
#include "../core/CargoServiceInventoryConfig.h"
#include "../core/EconomyJournal.h"
#include "../core/LedgerConfig.h"
#include "../core/CargoCatalogConfig.h"

namespace {


class OEMLandingBridge;
OEMLandingBridge *g_bridge = 0;
int g_flightLogMode = -1;
int g_cargoManifestMode = -1;
int g_cargoExchangeTestMode = -1;

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

std::string encodePathComponent(const std::string& value) {
    static const char digits[] = "0123456789ABCDEF";
    std::string encoded;
    for (std::size_t i = 0; i < value.size(); ++i) {
        const unsigned char c = static_cast<unsigned char>(value[i]);
        encoded.push_back(digits[c >> 4]);
        encoded.push_back(digits[c & 15]);
    }
    return encoded;
}

std::string makeCargoStockConfigPath(HINSTANCE module, const std::string& serviceId) {
    char modulePath[MAX_PATH] = {};
    const DWORD length = GetModuleFileNameA(module, modulePath, MAX_PATH);
    if (length == 0 || length >= MAX_PATH) return std::string();
    char *separator = strrchr(modulePath, '\\');
    if (!separator) separator = strrchr(modulePath, '/');
    if (!separator) return std::string();
    *separator = '\0';
    char dataDirectory[MAX_PATH] = {};
    sprintf(dataDirectory, "%s\\OEMData", modulePath);
    if (!CreateDirectoryA(dataDirectory, 0) && GetLastError() != ERROR_ALREADY_EXISTS) return std::string();
    const std::string filename = "cargo_stock_" + encodePathComponent(serviceId) + ".cfg";
    if (strlen(dataDirectory) + 1 + filename.size() >= MAX_PATH) return std::string();
    std::string path(dataDirectory); path += "\\"; path += filename; return path;
}

std::string makeCargoServicesPath(HINSTANCE module) {
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

    char configPath[MAX_PATH] = {};
    sprintf(configPath, "%s\\cargo_services.cfg", dataDirectory);
    return configPath;
}

class OEMLandingBridge : public oapi::Module {
public:
    explicit OEMLandingBridge(HINSTANCE hDLL)
        : oapi::Module(hDLL), module_(hDLL), landingMonitors_(), flightMonitors_(),
          dockingMonitors_(), eventQueue_(), journalStore_(), journalPath_(makeJournalPath(hDLL)),
          cargoServices_(), cargoServicesPath_(makeCargoServicesPath(hDLL)),
          cargoTransferGate_(), cargoManifests_(), cargoManifestLoadAttempted_(),
          cargoStockConfigStore_(), cargoServiceInventories_(), cargoServiceInventoryLoadAttempted_(),
          economyJournalStore_(), economyJournalPath_(makeEconomyDataFilePath(hDLL, "economy.journal")),
          ledgerConfigStore_(), cargoCatalogConfigStore_(), accountsPath_(makeEconomyDataFilePath(hDLL, "accounts.cfg")),
          cargoCatalogPath_(makeEconomyDataFilePath(hDLL, "cargo_catalog.cfg")),
          ledger_(), cargoCatalog_(), economyReady_(false), journalReady_(false), sessionId_(),
          currentSimulationTime_(0), cargoTransferSequence_(0), lastFocus_(0) {
        g_bridge = this;
    }

    const std::vector<oem::SimulationEvent>& events() const {
        return eventQueue_.events();
    }

    bool journalReady() const { return journalReady_; }
    bool isCargoService(const std::string& vesselId) const {
        return cargoServices_.isRegistered(vesselId);
    }

    const oem::CargoServiceInventory* cargoServiceInventory(const std::string& serviceId) const {
        std::map<std::string, oem::CargoServiceInventory>::const_iterator it =
            cargoServiceInventories_.find(serviceId);
        return it == cargoServiceInventories_.end() ? 0 : &it->second;
    }

    std::string dockedCargoService(const std::string& vesselId) const {
        OBJHANDLE focus = oapiGetFocusObject();
        if (!focus || !oapiIsVessel(focus)) return std::string();
        VESSEL *vessel = oapiGetVesselInterface(focus);
        if (!vessel || !vessel->GetName() || vesselId != vessel->GetName())
            return std::string();
        const UINT count = vessel->DockCount();
        for (UINT i = 0; i < count; ++i) {
            DOCKHANDLE dock = vessel->GetDockHandle(i);
            if (!dock) continue;
            OBJHANDLE other = vessel->GetDockStatus(dock);
            if (!other || !oapiIsVessel(other)) continue;
            char otherName[128] = {};
            oapiGetObjectName(other, otherName, sizeof(otherName));
            if (otherName[0] != '\0' && cargoServices_.isRegistered(otherName))
                return std::string(otherName);
        }
        return std::string();
    }

    oem::Result performCargoExchangeTest(const std::string& vesselId,
                                         oem::CargoTransferDirection direction) {
        if (sessionId_.empty() || currentSimulationTime_ <= 0)
            return oem::Result::failure("CARGO_TEST_NOT_READY",
                "Wait until the simulation has advanced beyond its initialization frame.");
        const std::string serviceId = dockedCargoService(vesselId);
        if (serviceId.empty())
            return oem::Result::failure("CARGO_TEST_NOT_DOCKED",
                "The focused vessel must be docked to a registered cargo service.");
        ensureCargoManifest(vesselId);
        if (!ensureCargoServiceInventory(serviceId))
            return oem::Result::failure("CARGO_SERVICE_INVENTORY_UNAVAILABLE",
                "The registered service stock and exchange journal are not available.");
        std::map<std::string, oem::CargoManifest>::const_iterator manifest =
            cargoManifests_.find(vesselId);
        std::map<std::string, oem::CargoServiceInventory>::const_iterator inventory =
            cargoServiceInventories_.find(serviceId);
        if (manifest == cargoManifests_.end() || inventory == cargoServiceInventories_.end())
            return oem::Result::failure("CARGO_TEST_PROJECTION_UNAVAILABLE",
                "The cargo accounting projections are unavailable.");

        oem::CargoTransfer transfer;
        transfer.vesselId = vesselId;
        transfer.locationId = serviceId;
        transfer.direction = direction;
        transfer.quantity = 1;
        if (direction == oem::CARGO_LOAD) {
            if (inventory->second.lines().empty())
                return oem::Result::failure("CARGO_TEST_NO_STOCK",
                    "The service has no configured stock. Add an explicit stock line first.");
            const oem::CargoStockLine& line = inventory->second.lines()[0];
            transfer.cargoId = line.cargoId;
            transfer.unitMassKg = line.unitMassKg;
        } else if (direction == oem::CARGO_UNLOAD) {
            const std::vector<oem::CargoLine>& lines = manifest->second.lines();
            bool found = false;
            for (std::size_t i = 0; i < lines.size(); ++i) {
                if (lines[i].originLocationId == serviceId && lines[i].quantity > 0) {
                    transfer.cargoId = lines[i].cargoId;
                    transfer.unitMassKg = lines[i].unitMassKg;
                    found = true;
                    break;
                }
            }
            if (!found)
                return oem::Result::failure("CARGO_TEST_NO_CARGO",
                    "The vessel has no manifest cargo from this service to return.");
        } else {
            return oem::Result::failure("CARGO_TEST_DIRECTION",
                "Unknown diagnostic transfer direction.");
        }

        char transferId[256];
        sprintf(transferId, "OEMTEST-%s-%lu", sessionId_.c_str(),
                ++cargoTransferSequence_);
        transfer.transferId = transferId;
        return applyCargoTransfer(transfer, (double)currentSimulationTime_);
    }

    const oem::CargoManifest* cargoManifest(const std::string& vesselId) const {
        std::map<std::string, oem::CargoManifest>::const_iterator it = cargoManifests_.find(vesselId);
        return it == cargoManifests_.end() ? 0 : &it->second;
    }

    oem::Result applyCargoTransfer(const oem::CargoTransfer& transfer, double simulationTime) {
        OBJHANDLE focus = oapiGetFocusObject();
        if (!focus || !oapiIsVessel(focus))
            return oem::Result::failure("CARGO_NO_FOCUS", "No focused vessel is available for a cargo transfer.");
        VESSEL *vessel = oapiGetVesselInterface(focus);
        if (!vessel)
            return oem::Result::failure("CARGO_NO_VESSEL", "Orbiter could not provide the focused vessel interface.");
        const std::string vesselId = vessel->GetName() ? vessel->GetName() : "";
        if (vesselId.empty() || transfer.vesselId != vesselId)
            return oem::Result::failure("CARGO_VESSEL_MISMATCH", "Transfer vessel is not the currently focused Orbiter vessel.");
        ensureCargoManifest(vesselId);
        std::map<std::string, oem::CargoManifest>::const_iterator current = cargoManifests_.find(vesselId);
        if (current == cargoManifests_.end())
            return oem::Result::failure("CARGO_MANIFEST_UNAVAILABLE", "The vessel cargo manifest could not be loaded safely.");

        oem::CargoTransferObservation observation;
        observation.vesselId = vesselId;
        observation.landed = (vessel->GetFlightStatus() & 1u) != 0;
        observation.groundContact = vessel->GroundContact();
        if (observation.groundContact) {
            OBJHANDLE body = vessel->GetSurfaceRef();
            if (body) {
                double vesselLng = 0.0, vesselLat = 0.0, vesselRadius = 0.0;
                OBJHANDLE positionBody = vessel->GetEquPos(vesselLng, vesselLat, vesselRadius);
                const double bodyRadius = oapiGetSize(body);
                if (positionBody == body && bodyRadius > 0.0 && _finite(bodyRadius) &&
                    _finite(vesselLat) && _finite(vesselLng)) {
                    observation.surfaceLocationKnown = true;
                    double nearestDistance = -1.0;
                    const DWORD baseCount = oapiGetBaseCount(body);
                    for (DWORD i = 0; i < baseCount; ++i) {
                        OBJHANDLE base = oapiGetBaseByIndex(body, (int)i);
                        if (!base) continue;
                        double baseLng = 0.0, baseLat = 0.0, baseRadius = 0.0;
                        oapiGetBaseEquPos(base, &baseLng, &baseLat, &baseRadius);
                        const double distance = oem::surfaceDistanceMetres(
                            vesselLat, vesselLng, baseLat, baseLng, bodyRadius);
                        if (distance >= 0.0 && (nearestDistance < 0.0 || distance < nearestDistance)) {
                            nearestDistance = distance;
                            char baseName[128] = {};
                            oapiGetObjectName(base, baseName, sizeof(baseName));
                            observation.surfaceBaseId = baseName;
                        }
                    }
                    observation.surfaceBaseDistanceMetres = nearestDistance;
                }
            }
        }
        const UINT dockCount = vessel->DockCount();
        for (UINT i = 0; i < dockCount; ++i) {
            DOCKHANDLE dock = vessel->GetDockHandle(i);
            if (!dock) continue;
            OBJHANDLE other = vessel->GetDockStatus(dock);
            if (!other || !oapiIsVessel(other)) continue;
            char otherName[128] = {};
            oapiGetObjectName(other, otherName, sizeof(otherName));
            if (otherName[0] != '\0' && cargoServices_.isRegistered(otherName)) {
                observation.docked = true;
                observation.dockedVesselId = otherName;
                break;
            }
        }

        oem::CargoTransfer candidateTransfer = transfer;
        candidateTransfer.simulationTime = simulationTime >= 0.0
            ? (oem::SimulationSeconds)simulationTime : 0;
        const oem::Result authorized = cargoTransferGate_.authorize(
            candidateTransfer, observation, cargoServices_);
        if (!authorized.ok) return authorized;
        if (!cargoServices_.isRegistered(candidateTransfer.locationId))
            return oem::Result::failure("CARGO_SPACEPORT_STOCK_UNAVAILABLE",
                "Spaceport authorization is verified, but spaceport stock accounting is not configured yet; only registered service-vessel inventories are supported.");
        if (!ensureCargoServiceInventory(candidateTransfer.locationId))
            return oem::Result::failure("CARGO_SERVICE_INVENTORY_UNAVAILABLE",
                "The registered cargo service stock or exchange history could not be loaded safely.");
        std::map<std::string, oem::CargoServiceInventory>::const_iterator station =
            cargoServiceInventories_.find(candidateTransfer.locationId);
        if (station == cargoServiceInventories_.end())
            return oem::Result::failure("CARGO_SERVICE_INVENTORY_UNAVAILABLE",
                "The registered cargo service inventory is unavailable.");

        if (!economyReady_ || economyJournalPath_.empty())
            return oem::Result::failure("ECONOMY_UNAVAILABLE",
                "The global economy journal or its opening configuration is unavailable.");
        const oem::Result committed = economyJournalStore_.applyExchange(
            economyJournalPath_, cargoServiceInventories_, cargoManifests_[vesselId], candidateTransfer);
        if (!committed.ok) return committed;
        return oem::Result::success();
    }

    void clbkSimulationStart(RenderMode mode) override {
        landingMonitors_.clear();
        flightMonitors_.clear();
        dockingMonitors_.clear();
        cargoManifests_.clear();
        cargoManifestLoadAttempted_.clear();
        cargoServiceInventories_.clear();
        cargoServiceInventoryLoadAttempted_.clear();
        ledger_ = oem::Ledger();
        cargoCatalog_ = oem::CargoCatalog();
        economyReady_ = false;
        sessionId_.clear();
        currentSimulationTime_ = 0;
        cargoTransferSequence_ = 0;
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
        cargoServices_.clear();
        const oem::Result servicesLoaded = cargoServicesPath_.empty()
            ? oem::Result::failure("CARGO_SERVICES_PATH", "Could not resolve cargo service configuration path.")
            : cargoServices_.load(cargoServicesPath_);
        if (servicesLoaded.ok) {
            char message[256];
            sprintf(message, "OEM Cargo Services: loaded %lu registered vessels.",
                    (unsigned long)cargoServices_.size());
            oapiWriteLog(message);
        } else {
            char message[512];
            sprintf(message, "OEM Cargo Services: load failed (%s): %s",
                    servicesLoaded.code.c_str(), servicesLoaded.message.c_str());
            oapiWriteLog(message);
        }
        if (servicesLoaded.ok) {
            const std::set<std::string>& serviceIds = cargoServices_.vesselIds();
            for (std::set<std::string>::const_iterator it = serviceIds.begin(); it != serviceIds.end(); ++it)
                ensureCargoServiceInventory(*it);
            initializeEconomyState();
        }
        oapiWriteLog((char*)"OEM Landing Bridge: simulation monitoring started.");
    }

    void clbkSimulationEnd() override {
        landingMonitors_.clear();
        flightMonitors_.clear();
        dockingMonitors_.clear();
        sessionId_.clear();
        lastFocus_ = 0;
        oapiWriteLog((char*)"OEM Landing Bridge: simulation monitoring stopped.");
    }

    void clbkPostStep(double simt, double simdt, double mjd) override {
        // The first positive-time sample is the baseline. simt=0 telemetry can
        // contain transient vessel states during scenario initialization.
        if (simt <= 0.0) return;
        currentSimulationTime_ = (oem::SimulationSeconds)simt;

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
            dockingMonitors_.clear();
            lastFocus_ = hVessel;
        }

        VESSEL *vessel = oapiGetVesselInterface(hVessel);
        if (!vessel) return;

        oem::LandingObservation landing;
        landing.vesselPresent = true;
        landing.vesselId = vessel->GetName() ? vessel->GetName() : "";
        landing.simulationTime = simSeconds;
        ensureCargoManifest(landing.vesselId);

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

        // Compare Orbiter's actual per-port state with the previous sample.
        // The first sample is a baseline, not a docking event.
        oem::DockingObservation docking;
        docking.vesselPresent = true;
        docking.vesselId = landing.vesselId;
        docking.simulationTime = simSeconds;
        docking.simulationTimeMilliseconds = simMilliseconds;
        const UINT dockCount = vessel->DockCount();
        for (UINT dockIndex = 0; dockIndex < dockCount; ++dockIndex) {
            DOCKHANDLE hDock = vessel->GetDockHandle(dockIndex);
            if (!hDock) continue;
            OBJHANDLE hOther = vessel->GetDockStatus(hDock);
            if (!hOther || !oapiIsVessel(hOther)) continue;
            char otherName[128] = {};
            oapiGetObjectName(hOther, otherName, sizeof(otherName));
            if (otherName[0] != '\0')
                docking.ports.push_back(oem::DockingPortObservation(dockIndex, otherName));
        }
        std::vector<oem::SimulationEvent> dockingEvents;
        if (dockingMonitors_[hVessel].observe(docking, dockingEvents)) {
            for (std::size_t i = 0; i < dockingEvents.size(); ++i) {
                oem::SimulationEvent& dockEvent = dockingEvents[i];
                dockEvent.eventId = oem::makeSimulationEventId(
                    sessionId_, dockEvent.type, dockEvent.vesselId,
                    dockEvent.simulationTimeMilliseconds);
                char discriminator[256];
                sprintf(discriminator, ":PORT%d:%s", dockEvent.dockingPortIndex,
                        dockEvent.relatedVesselId.c_str());
                dockEvent.eventId += discriminator;
                if (publishEvent(dockEvent)) {
                    char message[512];
                    sprintf(message, "OEM %s: vessel=%s other=%s port=%d simt=%lld",
                            oem::simulationEventTypeName(dockEvent.type),
                            dockEvent.vesselId.c_str(), dockEvent.relatedVesselId.c_str(),
                            dockEvent.dockingPortIndex, dockEvent.simulationTime);
                    oapiWriteLog(message);
                }
                if (cargoServices_.isRegistered(dockEvent.relatedVesselId)) {
                    char serviceMessage[512];
                    sprintf(serviceMessage,
                        "OEM CARGO SERVICE %s: vessel=%s service=%s port=%d",
                        dockEvent.type == oem::SIM_EVENT_DOCK ? "DOCKED" : "UNDOCKED",
                        dockEvent.vesselId.c_str(), dockEvent.relatedVesselId.c_str(),
                        dockEvent.dockingPortIndex);
                    oapiWriteLog(serviceMessage);
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
        dockingMonitors_.erase(hVessel);
    }

private:

    bool ensureCargoServiceInventory(const std::string& serviceId) {
        if (serviceId.empty() || !cargoServices_.isRegistered(serviceId)) return false;
        if (cargoServiceInventories_.find(serviceId) != cargoServiceInventories_.end()) return true;
        if (cargoServiceInventoryLoadAttempted_.find(serviceId) != cargoServiceInventoryLoadAttempted_.end()) return false;
        cargoServiceInventoryLoadAttempted_.insert(serviceId);
        const std::string configPath = makeCargoStockConfigPath(module_, serviceId);
        if (configPath.empty()) {
            oapiWriteLog((char*)"OEM Cargo Stock: could not resolve stock configuration path.");
            return false;
        }
        oem::CargoServiceInventory candidate(serviceId);
        const oem::Result configured = cargoStockConfigStore_.load(configPath, serviceId, candidate);
        if (!configured.ok) {
            char message[512];
            sprintf(message, "OEM Cargo Stock: load failed service=%s (%s): %s",
                    serviceId.c_str(), configured.code.c_str(), configured.message.c_str());
            oapiWriteLog(message);
            return false;
        }
        cargoServiceInventories_.insert(std::make_pair(serviceId, candidate));
        char message[512];
        sprintf(message, "OEM Cargo Stock: opening state loaded service=%s lines=%lu.",
                serviceId.c_str(), (unsigned long)candidate.lines().size());
        oapiWriteLog(message);
        return true;
    }

    bool initializeEconomyState() {
        if (economyJournalPath_.empty() || accountsPath_.empty() || cargoCatalogPath_.empty()) {
            oapiWriteLog((char*)"OEM Economy: could not resolve economy configuration paths.");
            return false;
        }

        oem::Ledger openingLedger;
        const oem::Result accounts = ledgerConfigStore_.load(accountsPath_, openingLedger);
        if (!accounts.ok && accounts.code != "LEDGER_CONFIG_MISSING") {
            char message[512];
            sprintf(message, "OEM Economy: account configuration failed (%s): %s",
                    accounts.code.c_str(), accounts.message.c_str());
            oapiWriteLog(message);
            return false;
        }
        if (accounts.ok) {
            char message[256];
            sprintf(message, "OEM Economy: loaded %lu company accounts.",
                    (unsigned long)openingLedger.accounts().size());
            oapiWriteLog(message);
        } else {
            oapiWriteLog((char*)"OEM Economy: accounts.cfg missing; market trading remains unavailable until explicitly configured.");
        }

        oem::CargoCatalog openingCatalog;
        const oem::Result catalog = cargoCatalogConfigStore_.load(cargoCatalogPath_, openingCatalog);
        if (!catalog.ok && catalog.code != "CATALOG_CONFIG_MISSING") {
            char message[512];
            sprintf(message, "OEM Economy: cargo catalog failed (%s): %s",
                    catalog.code.c_str(), catalog.message.c_str());
            oapiWriteLog(message);
            return false;
        }
        if (catalog.ok) {
            char message[256];
            sprintf(message, "OEM Economy: loaded %lu cargo definitions.",
                    (unsigned long)openingCatalog.entries().size());
            oapiWriteLog(message);
        } else {
            oapiWriteLog((char*)"OEM Economy: cargo_catalog.cfg missing; market trading remains unavailable until explicitly configured.");
        }

        std::map<std::string, oem::CargoManifest> openingManifests;
        const oem::Result recovered = economyJournalStore_.load(
            economyJournalPath_, openingCatalog, openingLedger,
            cargoServiceInventories_, openingManifests);
        if (!recovered.ok) {
            char message[512];
            sprintf(message, "OEM Economy: global journal recovery failed (%s): %s",
                    recovered.code.c_str(), recovered.message.c_str());
            oapiWriteLog(message);
            return false;
        }

        ledger_ = openingLedger;
        cargoCatalog_ = openingCatalog;
        cargoManifests_.swap(openingManifests);
        for (std::map<std::string, oem::CargoManifest>::const_iterator it = cargoManifests_.begin();
             it != cargoManifests_.end(); ++it)
            cargoManifestLoadAttempted_.insert(it->first);
        economyReady_ = true;
        oapiWriteLog((char*)"OEM Economy: global journal ready; legacy test journals are ignored.");
        return true;
    }

    void ensureCargoManifest(const std::string& vesselId) {
        if (vesselId.empty() || cargoManifestLoadAttempted_.find(vesselId) != cargoManifestLoadAttempted_.end()) return;
        cargoManifestLoadAttempted_.insert(vesselId);
        cargoManifests_.insert(std::make_pair(vesselId, oem::CargoManifest(vesselId)));
    }

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
    HINSTANCE module_;
    std::map<OBJHANDLE, oem::DockingTransitionMonitor> dockingMonitors_;
    oem::SimulationEventQueue eventQueue_;
    oem::SimulationEventJournalStore journalStore_;
    std::string journalPath_;
    oem::CargoServiceRegistry cargoServices_;
    std::string cargoServicesPath_;
    oem::CargoTransferGate cargoTransferGate_;
    std::map<std::string, oem::CargoManifest> cargoManifests_;
    std::set<std::string> cargoManifestLoadAttempted_;
    oem::CargoServiceInventoryConfigStore cargoStockConfigStore_;
    std::map<std::string, oem::CargoServiceInventory> cargoServiceInventories_;
    std::set<std::string> cargoServiceInventoryLoadAttempted_;
    oem::EconomyJournalStore economyJournalStore_;
    std::string economyJournalPath_;
    oem::LedgerConfigStore ledgerConfigStore_;
    oem::CargoCatalogConfigStore cargoCatalogConfigStore_;
    std::string accountsPath_;
    std::string cargoCatalogPath_;
    oem::Ledger ledger_;
    oem::CargoCatalog cargoCatalog_;
    bool economyReady_;
    bool journalReady_;
    std::string sessionId_;
    oem::SimulationSeconds currentSimulationTime_;
    unsigned long cargoTransferSequence_;
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

            if (event.type == oem::SIM_EVENT_DOCK ||
                event.type == oem::SIM_EVENT_UNDOCK) {
                sprintf(line, "%.12s <-> %.12s port %d",
                        event.vesselId.c_str(), event.relatedVesselId.c_str(),
                        event.dockingPortIndex);
            } else if (event.locationKnown) {
                sprintf(line, "%.15s @ %.9s/%.9s",
                        event.vesselId.c_str(), event.bodyId.c_str(), event.baseId.c_str());
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

class OEMCargoManifestMFD : public MFD {
public:
    OEMCargoManifestMFD(DWORD w, DWORD h, VESSEL *vessel)
        : MFD(w, h, vessel),
          vesselId_(vessel && vessel->GetName() ? vessel->GetName() : "") {}

    void Update(HDC hDC) override {
        Title(hDC, "OEM CARGO MANIFEST");
        SelectDefaultFont(hDC, 0);
        int y = (int)ch + 3;
        char line[256];
        sprintf(line, "VESSEL: %.32s", vesselId_.c_str());
        DrawLine(hDC, y, line);
        if (!g_bridge) {
            DrawLine(hDC, y, "OEM bridge unavailable.");
            return;
        }
        const oem::CargoManifest *manifest = g_bridge->cargoManifest(vesselId_);
        if (!manifest) {
            DrawLine(hDC, y, "Manifest not loaded.");
            DrawLine(hDC, y, "Focus this vessel to initialize it.");
            return;
        }
        sprintf(line, "ENTRIES: %lu  TRANSFERS: %lu",
                (unsigned long)manifest->lines().size(),
                (unsigned long)manifest->history().size());
        DrawLine(hDC, y, line);
        sprintf(line, "ACCOUNTED MASS: %.3f kg", manifest->totalMassKg());
        DrawLine(hDC, y, line);
        DrawLine(hDC, y, "------------------------------");
        if (manifest->lines().empty()) {
            DrawLine(hDC, y, "No OEM cargo entries recorded.");
            DrawLine(hDC, y, "Physical inventory is not inferred.");
            return;
        }
        const int available = ((int)H - y - (int)ch) / (int)ch;
        const std::vector<oem::CargoLine>& lines = manifest->lines();
        for (std::size_t i = 0; i < lines.size() && (int)i < available; ++i) {
            sprintf(line, "%.18s x %lld", lines[i].cargoId.c_str(), lines[i].quantity);
            DrawLine(hDC, y, line);
            sprintf(line, "  %.3f kg/unit from %.14s",
                    lines[i].unitMassKg, lines[i].originLocationId.c_str());
            DrawLine(hDC, y, line);
        }
    }

    static int MsgProc(UINT msg, UINT mfd, WPARAM wparam, LPARAM lparam) {
        if (msg == OAPI_MSG_MFD_OPENEDEX && g_bridge) {
            MFDMODEOPENSPEC *spec = (MFDMODEOPENSPEC*)wparam;
            return (int)(new OEMCargoManifestMFD(
                spec->w, spec->h, (VESSEL*)lparam));
        }
        return 0;
    }

private:
    void DrawLine(HDC hDC, int &y, const char *text) {
        TextOutA(hDC, 4, y, text, (int)strlen(text));
        y += (int)ch;
    }

    std::string vesselId_;
};

class OEMCargoExchangeTestMFD : public MFD {
public:
    OEMCargoExchangeTestMFD(DWORD w, DWORD h, VESSEL *vessel)
        : MFD(w, h, vessel),
          vesselId_(vessel && vessel->GetName() ? vessel->GetName() : ""),
          status_("Ready. Ledger only; no physical cargo is moved.") {}

    void Update(HDC hDC) override {
        Title(hDC, "OEM CARGO EXCHANGE TEST");
        SelectDefaultFont(hDC, 0);
        int y = (int)ch + 3;
        DrawLine(hDC, y, "LEDGER ONLY - NO PHYSICAL CARGO");
        DrawLine(hDC, y, "One unit per command; restart to test recovery.");
        if (!g_bridge) {
            DrawLine(hDC, y, "OEM bridge unavailable.");
            return;
        }
        const std::string serviceId = g_bridge->dockedCargoService(vesselId_);
        if (serviceId.empty()) {
            DrawLine(hDC, y, "Dock to a registered cargo service.");
            DrawLine(hDC, y, "ISS is registered by default.");
        } else {
            char line[256];
            sprintf(line, "SERVICE: %.36s", serviceId.c_str());
            DrawLine(hDC, y, line);
            const oem::CargoServiceInventory *stock =
                g_bridge->cargoServiceInventory(serviceId);
            if (!stock) {
                DrawLine(hDC, y, "Service inventory unavailable.");
            } else if (stock->lines().empty()) {
                DrawLine(hDC, y, "STOCK: empty (configure explicitly)");
            } else {
                const oem::CargoStockLine& item = stock->lines()[0];
                sprintf(line, "LOAD: %.24s x %lld", item.cargoId.c_str(), item.quantity);
                DrawLine(hDC, y, line);
            }
        }
        const oem::CargoManifest *manifest = g_bridge->cargoManifest(vesselId_);
        if (!manifest) {
            DrawLine(hDC, y, "Vessel manifest unavailable.");
        } else if (manifest->lines().empty()) {
            DrawLine(hDC, y, "VESSEL: empty");
        } else {
            const oem::CargoLine& item = manifest->lines()[0];
            char line[256];
            sprintf(line, "VESSEL: %.24s x %lld", item.cargoId.c_str(), item.quantity);
            DrawLine(hDC, y, line);
        }
        // Diagnostic transfers update only OEM accounting; Orbiter physics is untouched.
        DrawLine(hDC, y, "L: LOAD 1 UNIT     U: UNLOAD 1 UNIT");
        DrawLine(hDC, y, status_.c_str());
    }

    bool ConsumeKeyBuffered(DWORD key) override {
        if (!g_bridge) return false;
        oem::Result result;
        if (key == OAPI_KEY_L) {
            result = g_bridge->performCargoExchangeTest(vesselId_, oem::CARGO_LOAD);
        } else if (key == OAPI_KEY_U) {
            result = g_bridge->performCargoExchangeTest(vesselId_, oem::CARGO_UNLOAD);
        } else {
            return false;
        }
        status_ = result.ok ? "Transfer committed to durable OEM journal." :
            result.code + ": " + result.message;
        return true;
    }

    bool ConsumeButton(int bt, int event) override {
        if (!(event & PANEL_MOUSE_LBDOWN)) return false;
        if (bt == 0) return ConsumeKeyBuffered(OAPI_KEY_L);
        if (bt == 1) return ConsumeKeyBuffered(OAPI_KEY_U);
        return false;
    }

    char *ButtonLabel(int bt) override {
        static char *labels[2] = { (char*)"LOAD", (char*)"UNLD" };
        return bt >= 0 && bt < 2 ? labels[bt] : 0;
    }

    int ButtonMenu(const MFDBUTTONMENU **menu) const override {
        static const MFDBUTTONMENU items[2] = {
            { "Load one unit to OEM manifest", 0, 'L' },
            { "Unload one unit to service stock", 0, 'U' }
        };
        if (menu) *menu = items;
        return 2;
    }

    static int MsgProc(UINT msg, UINT mfd, WPARAM wparam, LPARAM lparam) {
        if (msg == OAPI_MSG_MFD_OPENEDEX && g_bridge) {
            MFDMODEOPENSPEC *spec = (MFDMODEOPENSPEC*)wparam;
            return (int)(new OEMCargoExchangeTestMFD(
                spec->w, spec->h, (VESSEL*)lparam));
        }
        return 0;
    }

private:
    void DrawLine(HDC hDC, int &y, const char *text) {
        TextOutA(hDC, 4, y, text, (int)strlen(text));
        y += (int)ch;
    }

    std::string vesselId_;
    std::string status_;
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

    spec.name = "OEM Cargo Manifest";
    spec.key = OAPI_KEY_C;
    spec.msgproc = OEMCargoManifestMFD::MsgProc;
    g_cargoManifestMode = oapiRegisterMFDMode(spec);

    spec.name = "OEM Cargo Exchange Test";
    spec.key = OAPI_KEY_E;
    spec.msgproc = OEMCargoExchangeTestMFD::MsgProc;
    g_cargoExchangeTestMode = oapiRegisterMFDMode(spec);
}

DLLCLBK void ExitModule(HINSTANCE hDLL) {
    if (g_flightLogMode >= 0) oapiUnregisterMFDMode(g_flightLogMode);
    if (g_cargoManifestMode >= 0) oapiUnregisterMFDMode(g_cargoManifestMode);
    if (g_cargoExchangeTestMode >= 0) oapiUnregisterMFDMode(g_cargoExchangeTestMode);
    g_flightLogMode = -1;
    g_cargoManifestMode = -1;
    g_cargoExchangeTestMode = -1;
    g_bridge = 0;
    // Orbiter owns and destroys registered oapi::Module instances.
}
