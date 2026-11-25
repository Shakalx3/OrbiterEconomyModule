#define STRICT
#define ORBITER_MODULE
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <map>
#include <algorithm>
#include <set>
#include <vector>
#include <string>
#include <math.h>
#include <fstream>
#include <sstream>
#include <locale>
#include <stdlib.h>
#include <limits.h>
#include "orbitersdk.h"
#include "../core/SpaceportLanding.h"
#include "../core/FlightTransition.h"
#include "../core/DockingTransition.h"
#include "../core/SimulationEvent.h"
#include "../core/SimulationEventJournal.h"
#include "../core/SurfaceDistance.h"
#include "../core/SpaceportRegistry.h"
#include "../core/CargoServiceRegistry.h"
#include "../core/CargoManifest.h"
#include "../core/CargoTransferGate.h"
#include "../core/CargoServiceInventoryConfig.h"
#include "../core/EconomyJournal.h"
#include "../core/MissionBoard.h"
#include "../core/MissionEconomics.h"
#include "../core/MissionObjectiveBook.h"
#include "../core/MissionObjectiveBoard.h"
#include "../core/LedgerConfig.h"
#include "../core/CargoCatalogConfig.h"
#include "../core/EconomicLocationRegistry.h"
#include "../core/EconomicStationRegistry.h"
#include "../core/MissionIssuerRegistry.h"
#include "../core/PropellantPlan.h"
#include "../core/PropellantQuote.h"
#include "../core/PropellantTransaction.h"
#include "../core/PropellantTransactionJournal.h"
#include "../core/PrimaryResourceConfig.h"

namespace {


class OEMLandingBridge;
OEMLandingBridge *g_bridge = 0;
int g_flightLogMode = -1;
int g_cargoManifestMode = -1;
int g_marketMode = -1;
int g_missionBoardMode = -1;
int g_economicAdminMode = -1;
int g_propellantMonitorMode = -1;
int g_propellantRefuelMode = -1;

unsigned long deliveryDestinationScore(const std::string& seed,
                                       const std::string& destination) {
    unsigned long hash = 2166136261UL;
    const std::string value = seed + "|" + destination;
    for (std::size_t i = 0; i < value.size(); ++i) {
        hash ^= static_cast<unsigned char>(value[i]);
        hash *= 16777619UL;
    }
    return hash;
}

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

std::string makeEconomyDataFilePath(HINSTANCE module, const char *filename) {
    char modulePath[MAX_PATH] = {};
    const DWORD length = GetModuleFileNameA(module, modulePath, MAX_PATH);
    if (length == 0 || length >= MAX_PATH) return std::string();
    char *separator = strrchr(modulePath, 92);
    if (!separator) separator = strrchr(modulePath, '/');
    if (!separator) return std::string();
    *separator = 0;
    char dataDirectory[MAX_PATH] = {};
    sprintf(dataDirectory, "%s/OEMData", modulePath);
    if (!CreateDirectoryA(dataDirectory, 0) && GetLastError() != ERROR_ALREADY_EXISTS)
        return std::string();
    if (strlen(dataDirectory) + 1 + strlen(filename) >= MAX_PATH) return std::string();
    std::string result(dataDirectory);
    result += "/";
    result += filename;
    return result;
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


std::string makeMissionBoardPath(HINSTANCE module) {
    return makeEconomyDataFilePath(module, "mission_boards.dat");
}

std::string makeObjectiveMissionBoardPath(HINSTANCE module) {
    return makeEconomyDataFilePath(module, "objective_mission_boards.dat");
}

class OEMLandingBridge : public oapi::Module {
public:
    explicit OEMLandingBridge(HINSTANCE hDLL)
        : oapi::Module(hDLL), module_(hDLL), landingMonitors_(), flightMonitors_(),
          dockingMonitors_(), eventQueue_(), journalStore_(), journalPath_(makeJournalPath(hDLL)),
          cargoServices_(), cargoServicesPath_(makeCargoServicesPath(hDLL)),
          cargoTransferGate_(), cargoManifests_(), cargoManifestLoadAttempted_(),
          cargoStockConfigStore_(), cargoServiceInventories_(), cargoServiceInventoryLoadAttempted_(),
          economyJournalStore_(), primaryResourceConfigStore_(), primaryResources_(),
          primaryResourcesPath_(makeEconomyDataFilePath(hDLL, "primary_resources.cfg")),
          primaryResourcesReady_(false), contractBook_(), objectiveBook_(), pendingContractOffers_(), contractOfferSequence_(0), contractOfferDay_(-1), missionBoardStore_(), missionBoards_(), missionBoardPath_(makeMissionBoardPath(hDLL)), missionBoardsReady_(false), objectiveBoardStore_(), objectiveBoards_(), objectiveBoardPath_(makeObjectiveMissionBoardPath(hDLL)), objectiveBoardsReady_(false), economyJournalPath_(makeEconomyDataFilePath(hDLL, "economy.journal")),
          missionFailurePolicyStore_(), missionFailurePolicy_(),
          missionFailurePolicyPath_(makeEconomyDataFilePath(hDLL, "mission_failure.cfg")),
          missionIssuerRegistry_(), missionIssuerRegistryPath_(makeEconomyDataFilePath(hDLL, "mission_issuers.cfg")),
          propellantTransactionJournal_(), propellantTransactionJournalPath_(makeEconomyDataFilePath(hDLL, "propellant_transactions.journal")), propellantPricePath_(makeEconomyDataFilePath(hDLL, "propellant_prices.cfg")), propellantRecoveryReady_(false), propellantOperationSequence_(0),
          ledgerConfigStore_(), cargoCatalogConfigStore_(), accountsPath_(makeEconomyDataFilePath(hDLL, "accounts.cfg")),
          cargoCatalogPath_(makeEconomyDataFilePath(hDLL, "cargo_catalog.cfg")),
          spaceportRegistryPath_(makeEconomyDataFilePath(hDLL, "spaceports.registry")),
          economicStations_(), economicStationsPath_(makeEconomyDataFilePath(hDLL, "economic_stations.registry")),
          spaceportRegistry_(), economicLocations_(), economicStationsReady_(false), cargoServicesReady_(false), ledger_(), cargoCatalog_(), economyReady_(false), journalReady_(false), sessionId_(),
          currentSimulationTime_(0), cargoTransferSequence_(0), lastFocus_(0), completionNote_(0), notificationUntil_(0) {
        g_bridge = this;
    }

    const std::vector<oem::SimulationEvent>& events() const {
        return eventQueue_.events();
    }

    bool journalReady() const { return journalReady_; }
    const oem::EconomicLocationRegistry& economicLocations() const { return economicLocations_; }
    bool isCargoService(const std::string& vesselId) const {
        return cargoServices_.isRegistered(vesselId);
    }

    bool isEconomicStation(const std::string& vesselId) const {
        return economicStations_.isStation(vesselId);
    }

    oem::Result setEconomicStationRegistered(const std::string& vesselId, bool enabled) {
        if (!economicStationsReady_)
            return oem::Result::failure("ECONOMIC_STATION_UNAVAILABLE", "Station registry did not load safely; existing data was left untouched.");
        if (!isFocusedVessel(vesselId))
            return oem::Result::failure("ECONOMIC_STATION_FOCUS",
                "Register the currently focused vessel from its OEM Economic Admin MFD.");
        oem::EconomicStationRegistry candidate(economicStations_);
        const bool changed = enabled ? candidate.registerStation(vesselId)
                                     : candidate.unregisterStation(vesselId);
        if (!changed)
            return oem::Result::failure("ECONOMIC_STATION_UNCHANGED",
                enabled ? "This vessel is already registered as a station."
                        : "This vessel is not registered as a station.");
        if (economicStationsPath_.empty())
            return oem::Result::failure("ECONOMIC_STATION_PATH", "Could not resolve station registry path.");
        const oem::Result saved = candidate.save(economicStationsPath_);
        if (!saved.ok) return saved;
        economicStations_ = candidate;
        rebuildEconomicLocationRegistry(spaceportRegistry_);
        return oem::Result::success();
    }

    oem::Result setCargoServiceRegistered(const std::string& vesselId, bool enabled) {
        if (!cargoServicesReady_)
            return oem::Result::failure("CARGO_SERVICE_UNAVAILABLE", "Cargo-service registry did not load safely; existing data was left untouched.");
        if (!isFocusedVessel(vesselId))
            return oem::Result::failure("CARGO_SERVICE_FOCUS",
                "Register the currently focused vessel from its OEM Economic Admin MFD.");
        oem::CargoServiceRegistry candidate(cargoServices_);
        const bool changed = enabled ? candidate.registerService(vesselId)
                                     : candidate.unregisterService(vesselId);
        if (!changed)
            return oem::Result::failure("CARGO_SERVICE_UNCHANGED",
                enabled ? "This vessel is already a cargo service."
                        : "This vessel is not a cargo service.");
        const oem::Result saved = cargoServicesPath_.empty()
            ? oem::Result::failure("CARGO_SERVICES_PATH", "Could not resolve cargo service registry path.")
            : candidate.save(cargoServicesPath_);
        if (!saved.ok) return saved;
        cargoServices_ = candidate;
        if (enabled) ensureCargoServiceInventory(vesselId);
        return oem::Result::success();
    }

    bool isFocusedVessel(const std::string& vesselId) const {
        OBJHANDLE focus = oapiGetFocusObject();
        if (!focus || !oapiIsVessel(focus) || vesselId.empty()) return false;
        char name[128] = {};
        oapiGetObjectName(focus, name, sizeof(name));
        return vesselId == name;
    }

    oem::Result resolvePropellantContext(const std::string& vesselId,
            oem::PropellantPriceContext& context, std::string& displayLocation) const {
        OBJHANDLE focus = oapiGetFocusObject();
        if (!focus || !oapiIsVessel(focus))
            return oem::Result::failure("PROPELLANT_NO_FOCUS", "No focused vessel is available.");
        VESSEL* vessel = oapiGetVesselInterface(focus);
        if (!vessel || !vessel->GetName() || vesselId != vessel->GetName())
            return oem::Result::failure("PROPELLANT_VESSEL_MISMATCH", "Refuelling requires the focused vessel.");

        for (UINT i = 0; i < vessel->DockCount(); ++i) {
            DOCKHANDLE dock = vessel->GetDockHandle(i);
            if (!dock) continue;
            OBJHANDLE other = vessel->GetDockStatus(dock);
            if (!other || !oapiIsVessel(other)) continue;
            char stationName[128] = {};
            oapiGetObjectName(other, stationName, sizeof(stationName));
            if (!stationName[0] || !economicStations_.isStation(stationName)) continue;
            VESSEL* station = oapiGetVesselInterface(other);
            if (!station) continue;
            OBJHANDLE body = station->GetGravityRef();
            if (!body) continue;
            char bodyName[128] = {};
            oapiGetObjectName(body, bodyName, sizeof(bodyName));
            const oem::EconomicLocation* location =
                economicLocations_.find(std::string("station:") + stationName);
            if (!location || location->kind != oem::ECONOMIC_LOCATION_STATION || !bodyName[0])
                return oem::Result::failure("PROPELLANT_LOCATION_UNKNOWN",
                    "The registered station has no resolvable celestial-body context.");
            context.locationId = location->canonicalId;
            context.locationKind = oem::PROPELLANT_LOCATION_ORBITAL_STATION;
            context.celestialBody = bodyName;
            displayLocation = stationName;
            return oem::Result::success();
        }

        if (!(vessel->GetFlightStatus() & 1u) || !vessel->GroundContact())
            return oem::Result::failure("PROPELLANT_NOT_AT_SERVICE",
                "Dock to a registered economic station or land at a registered planetary spaceport.");
        OBJHANDLE body = vessel->GetSurfaceRef();
        if (!body) return oem::Result::failure("PROPELLANT_BODY_UNKNOWN", "Could not resolve the surface body.");
        char bodyName[128] = {};
        oapiGetObjectName(body, bodyName, sizeof(bodyName));
        const std::string baseId = currentMarketLocation(vesselId);
        if (!bodyName[0] || baseId.empty())
            return oem::Result::failure("PROPELLANT_NOT_AT_SPACEPORT",
                "The landed vessel is not within the registered spaceport service radius.");
        const std::string canonical = std::string("surface:") + bodyName + "/" + baseId;
        const oem::EconomicLocation* location = economicLocations_.find(canonical);
        if (!location || location->kind != oem::ECONOMIC_LOCATION_SPACEPORT)
            return oem::Result::failure("PROPELLANT_LOCATION_UNKNOWN",
                "The planetary base is not present in the economic spaceport registry.");
        context.locationId = location->canonicalId;
        context.locationKind = oem::PROPELLANT_LOCATION_PLANETARY_SPACEPORT;
        context.celestialBody = bodyName;
        displayLocation = baseId;
        return oem::Result::success();
    }

    oem::Result resolvePropellantUnitPrice(const oem::PropellantPriceContext& context,
                                            double& unitPrice) const {
        if (propellantPricePath_.empty())
            return oem::Result::failure("PROPELLANT_PRICE_PATH", "Could not resolve propellant_prices.cfg.");
        DWORD attributes = GetFileAttributesA(propellantPricePath_.c_str());
        if (attributes == INVALID_FILE_ATTRIBUTES) {
            const DWORD error = GetLastError();
            if (error != ERROR_FILE_NOT_FOUND && error != ERROR_PATH_NOT_FOUND)
                return oem::Result::failure("PROPELLANT_PRICE_ACCESS", "Could not inspect propellant price configuration.");
            std::ofstream defaults(propellantPricePath_.c_str(), std::ios::out | std::ios::trunc);
            if (!defaults) return oem::Result::failure("PROPELLANT_PRICE_CREATE", "Could not create default propellant price configuration.");
            defaults.imbue(std::locale::classic());
            defaults << "OEM_PROPELLANT_PRICES|1\n"
                     << "# Optional exact-body rows override the wildcard rate for that location type.\n"
                     << "ORBITAL|*|2.0\n"
                     << "PLANETARY|*|3.0\n";
            defaults.close();
            if (!defaults) return oem::Result::failure("PROPELLANT_PRICE_CREATE", "Could not finish writing default propellant prices.");
        }
        std::ifstream input(propellantPricePath_.c_str());
        if (!input) return oem::Result::failure("PROPELLANT_PRICE_READ", "Could not read propellant price configuration.");
        std::string line;
        if (!std::getline(input, line) || line != "OEM_PROPELLANT_PRICES|1")
            return oem::Result::failure("PROPELLANT_PRICE_HEADER", "Propellant price configuration header/version is invalid.");
        const std::string wantedKind = context.locationKind == oem::PROPELLANT_LOCATION_ORBITAL_STATION
            ? "ORBITAL" : "PLANETARY";
        bool haveDefault = false, haveExact = false;
        double defaultPrice = 0.0, exactPrice = 0.0;
        std::set<std::string> seen;
        while (std::getline(input, line)) {
            if (!line.empty() && line[line.size() - 1] == '\r') line.erase(line.size() - 1);
            if (line.empty() || line[0] == '#') continue;
            std::istringstream row(line);
            std::string kind, body, value, extra;
            if (!std::getline(row, kind, '|') || !std::getline(row, body, '|') ||
                !std::getline(row, value, '|') || std::getline(row, extra, '|'))
                return oem::Result::failure("PROPELLANT_PRICE_RECORD", "A propellant price row is malformed.");
            char* end = 0;
            const double price = strtod(value.c_str(), &end);
            if (!end || *end || !_finite(price) || price <= 0.0 ||
                (kind != "ORBITAL" && kind != "PLANETARY") || body.empty())
                return oem::Result::failure("PROPELLANT_PRICE_VALUE", "A propellant price row contains an invalid kind, body, or positive rate.");
            const std::string key = kind + "|" + body;
            if (!seen.insert(key).second)
                return oem::Result::failure("PROPELLANT_PRICE_DUPLICATE", "Propellant price configuration contains a duplicate rule.");
            if (kind != wantedKind) continue;
            if (body == "*") { haveDefault = true; defaultPrice = price; }
            else if (body == context.celestialBody) { haveExact = true; exactPrice = price; }
        }
        if (input.bad()) return oem::Result::failure("PROPELLANT_PRICE_READ", "An error occurred while reading propellant price configuration.");
        if (haveExact) unitPrice = exactPrice;
        else if (haveDefault) unitPrice = defaultPrice;
        else return oem::Result::failure("PROPELLANT_PRICE_MISSING", "No default or body-specific propellant rate exists for this location type.");
        return oem::Result::success();
    }

    oem::Result readPropellantSnapshot(const std::string& vesselId,
            std::vector<oem::PropellantResourceState>& resources) const {
        OBJHANDLE focus = oapiGetFocusObject();
        if (!focus || !oapiIsVessel(focus))
            return oem::Result::failure("PROPELLANT_NO_FOCUS",
                "No focused vessel is available.");
        VESSEL *vessel = oapiGetVesselInterface(focus);
        if (!vessel || !vessel->GetName() || vesselId != vessel->GetName())
            return oem::Result::failure("PROPELLANT_VESSEL_MISMATCH",
                "The MFD vessel is not the current Orbiter focus vessel.");

        std::vector<oem::PropellantResourceState> snapshot;
        const DWORD count = vessel->GetPropellantCount();
        snapshot.reserve(count);
        for (DWORD i = 0; i < count; ++i) {
            PROPELLANT_HANDLE handle = vessel->GetPropellantHandleByIndex(i);
            if (!handle)
                return oem::Result::failure("PROPELLANT_INVALID_HANDLE",
                    "Orbiter returned an invalid propellant resource handle.");
            const double current = vessel->GetPropellantMass(handle);
            const double capacity = vessel->GetPropellantMaxMass(handle);
            if (!_finite(current) || !_finite(capacity) ||
                current < 0.0 || capacity < 0.0 || current > capacity)
                return oem::Result::failure("PROPELLANT_INVALID_RESOURCE",
                    "Orbiter returned invalid propellant mass or capacity.");
            snapshot.push_back(oem::PropellantResourceState(current, capacity));
        }
        if (!snapshot.empty()) {
            oem::PropellantTotals totals;
            const oem::Result checked =
                oem::inspectPropellantResources(snapshot, totals);
            if (!checked.ok) return checked;
        }
        resources.swap(snapshot);
        return oem::Result::success();
    }

    oem::Result performPropellantPurchase(const std::string& vesselId, bool fillAll) {
        if (!economyReady_ || !propellantRecoveryReady_ || economyJournalPath_.empty() ||
            propellantTransactionJournalPath_.empty())
            return oem::Result::failure("PROPELLANT_UNAVAILABLE",
                "Economy or propellant transaction recovery is not ready.");
        if (currentSimulationTime_ <= 0 || !isFocusedVessel(vesselId))
            return oem::Result::failure("PROPELLANT_FOCUS",
                "Wait for simulation time to advance and keep the purchasing vessel focused.");

        oem::PropellantPriceContext context;
        std::string locationName;
        oem::Result result = resolvePropellantContext(vesselId, context, locationName);
        if (!result.ok) return result;
        double unitPrice = 0.0;
        result = resolvePropellantUnitPrice(context, unitPrice);
        if (!result.ok) return result;

        VESSEL* vessel = oapiGetVesselInterface(oapiGetFocusObject());
        if (!vessel) return oem::Result::failure("PROPELLANT_VESSEL", "Could not access the focused vessel.");
        std::vector<oem::PropellantResourceState> before;
        result = readPropellantSnapshot(vesselId, before);
        if (!result.ok) return result;
        oem::PropellantTotals totals;
        result = oem::inspectPropellantResources(before, totals);
        if (!result.ok) return result;
        if (!(totals.freeCapacityKg > 1.0e-9))
            return oem::Result::failure("PROPELLANT_TANKS_FULL", "All propellant resources are already full.");

        const double requested = fillAll || totals.freeCapacityKg < 100.0
            ? totals.freeCapacityKg : 100.0;
        oem::Credits balance = 0;
        if (!ledger_.getBalance("company", balance))
            return oem::Result::failure("PROPELLANT_ACCOUNT", "Company account is not configured.");
        FILETIME fileTime;
        GetSystemTimeAsFileTime(&fileTime);
        const unsigned __int64 stamp =
            (static_cast<unsigned __int64>(fileTime.dwHighDateTime) << 32) |
            static_cast<unsigned __int64>(fileTime.dwLowDateTime);
        char operation[256];
        sprintf(operation, "REFUEL-%s-%I64u-%lu", vesselId.c_str(), stamp,
            ++propellantOperationSequence_);
        const std::string operationId(operation);
        oem::PropellantQuote quote;
        char quoteId[288];
        sprintf(quoteId, "QUOTE-%s", operation);
        result = oem::createPropellantQuote(before, quoteId, context,
            requested, unitPrice, balance, quote);
        if (!result.ok) return result;

        oem::PropellantTransaction transaction;
        result = oem::createPreparedPropellantTransaction(operationId, vesselId,
            before, quote, transaction, currentSimulationTime_);
        if (!result.ok) return result;
        result = propellantTransactionJournal_.prepare(
            propellantTransactionJournalPath_, transaction);
        if (!result.ok) return result;

        std::vector<oem::PropellantResourceState> immediate;
        result = readPropellantSnapshot(vesselId, immediate);
        std::vector<double> immediateRollback;
        if (!result.ok || oem::classifyPropellantRecovery(transaction, immediate,
                immediateRollback) != oem::PROPELLANT_RECOVERY_ABORT_UNCHANGED) {
            propellantRecoveryReady_ = false;
            return oem::Result::failure("PROPELLANT_SNAPSHOT_CHANGED",
                "Tank state changed after PREPARED was recorded; automatic refuelling is stopped for recovery.");
        }

        const DWORD count = vessel->GetPropellantCount();
        if (count != before.size()) {
            propellantRecoveryReady_ = false;
            return oem::Result::failure("PROPELLANT_RESOURCE_CHANGED",
                "Propellant resource count changed after PREPARED was recorded.");
        }
        for (DWORD i = 0; i < count; ++i) {
            PROPELLANT_HANDLE resource = vessel->GetPropellantHandleByIndex(i);
            if (!resource) {
                propellantRecoveryReady_ = false;
                return oem::Result::failure("PROPELLANT_INVALID_HANDLE",
                    "Orbiter returned an invalid resource handle during refuelling.");
            }
            vessel->SetPropellantMass(resource, quote.fillPlan.targetMassKg[i]);
        }

        std::vector<oem::PropellantResourceState> after;
        result = readPropellantSnapshot(vesselId, after);
        std::vector<double> rollback;
        const oem::PropellantRecoveryAction action = result.ok
            ? oem::classifyPropellantRecovery(transaction, after, rollback)
            : oem::PROPELLANT_RECOVERY_CONFLICT;
        if (action == oem::PROPELLANT_RECOVERY_FINALIZE_CHARGE) {
            result = economyJournalStore_.chargePropellant(economyJournalPath_,
                operationId, "company", quote.totalCostCredits,
                currentSimulationTime_, ledger_);
            if (!result.ok) {
                propellantRecoveryReady_ = false;
                return oem::Result::failure("PROPELLANT_CHARGE_PENDING",
                    "Physical fill is complete, but its durable charge is pending recovery.");
            }
            result = propellantTransactionJournal_.setTerminalState(
                propellantTransactionJournalPath_, operationId, oem::PROPELLANT_TX_COMMITTED);
            if (!result.ok) {
                propellantRecoveryReady_ = false;
                return oem::Result::failure("PROPELLANT_COMMIT_PENDING",
                    "Fuel and payment are committed; the transaction terminal marker will be recovered on restart.");
            }
            char log[512];
            sprintf(log, "OEM REFUEL COMMITTED: vessel=%s location=%s body=%s mass=%.3f kg cost=%lld cr",
                vesselId.c_str(), locationName.c_str(), context.celestialBody.c_str(),
                requested, (long long)quote.totalCostCredits);
            oapiWriteLog(log);
            return oem::Result::success();
        }

        if (action == oem::PROPELLANT_RECOVERY_ROLLBACK_PARTIAL) {
            for (DWORD i = 0; i < count; ++i) {
                PROPELLANT_HANDLE resource = vessel->GetPropellantHandleByIndex(i);
                if (!resource || i >= rollback.size()) {
                    propellantRecoveryReady_ = false;
                    return oem::Result::failure("PROPELLANT_ROLLBACK_FAILED",
                        "Partial fill could not be safely rolled back.");
                }
                vessel->SetPropellantMass(resource, rollback[i]);
            }
            std::vector<oem::PropellantResourceState> restored;
            result = readPropellantSnapshot(vesselId, restored);
            if (!result.ok || oem::classifyPropellantRecovery(transaction, restored,
                    rollback) != oem::PROPELLANT_RECOVERY_ABORT_UNCHANGED) {
                propellantRecoveryReady_ = false;
                return oem::Result::failure("PROPELLANT_ROLLBACK_FAILED",
                    "Rollback verification failed; transaction remains PREPARED.");
            }
            result = propellantTransactionJournal_.setTerminalState(
                propellantTransactionJournalPath_, operationId, oem::PROPELLANT_TX_ABORTED);
            if (!result.ok) {
                propellantRecoveryReady_ = false;
                return result;
            }
            return oem::Result::failure("PROPELLANT_WRITE_PARTIAL",
                "Partial resource write was rolled back; no charge was made.");
        }
        if (action == oem::PROPELLANT_RECOVERY_ABORT_UNCHANGED) {
            result = propellantTransactionJournal_.setTerminalState(
                propellantTransactionJournalPath_, operationId, oem::PROPELLANT_TX_ABORTED);
            if (!result.ok) propellantRecoveryReady_ = false;
            return oem::Result::failure("PROPELLANT_WRITE_UNCHANGED",
                "Orbiter did not apply the requested fuel change; no charge was made.");
        }
        propellantRecoveryReady_ = false;
        return oem::Result::failure("PROPELLANT_WRITE_CONFLICT",
            "Orbiter resource values do not match the prepared transaction; no automatic correction was attempted.");
    }

    const oem::CargoCatalog* marketCatalog() const {
        return economyReady_ ? &cargoCatalog_ : 0;
    }

    const oem::DeliveryContractBook& deliveryContractBook() const {
        return contractBook_;
    }

    const oem::MissionObjectiveBook& missionObjectiveBook() const {
        return objectiveBook_;
    }

    oem::SimulationSeconds missionSimulationTime() const { return currentSimulationTime_; }

    bool currentMissionOffers(const std::string& vesselId, std::vector<oem::DeliveryContract>& offers) {
        offers.clear();
        if (!economyReady_ || !missionBoardsReady_ || sessionId_.empty() ||
            currentSimulationTime_ <= 0 || vesselId.empty() || !isFocusedVessel(vesselId))
            return false;
        const std::vector<oem::DeliveryContract>& accepted = contractBook_.contracts();
        for (std::size_t i=0;i<accepted.size();++i) {
            if (accepted[i].vesselId==vesselId &&
                accepted[i].status!=oem::CONTRACT_SETTLED &&
                accepted[i].status!=oem::CONTRACT_EXPIRED &&
                accepted[i].status!=oem::CONTRACT_CANCELLED)
                return false;
        }
        const std::string origin=currentMarketLocation(vesselId);
        if(origin.empty() || !ensureCargoServiceInventory(origin)) return false;
        oem::EconomicLocation originLocation;
        VECTOR3 originGlobal = _V(0,0,0);
        OBJHANDLE originBody = 0;
        double originLng=0.0, originLat=0.0, originRadius=0.0;
        const bool originResolved = resolveCurrentEconomicLocation(vesselId, origin, originLocation) &&
            resolveEconomicLocationPosition(originLocation, originGlobal, originBody,
                originLng, originLat, originRadius);
        if(!originResolved) return false;
        const long long day=(long long)currentSimulationTime_/86400;
        std::map<std::string,oem::MissionBoard>::iterator existing=missionBoards_.find(originLocation.canonicalId);
        if(existing!=missionBoards_.end() && existing->second.simulationDay==day) {
            offers=existing->second.offers;
            return !offers.empty();
        }
        std::map<std::string,oem::CargoServiceInventory>::const_iterator stock=cargoServiceInventories_.find(origin);
        if(stock==cargoServiceInventories_.end()) return false;
        std::vector<oem::MissionCandidate> candidates;
        const std::vector<oem::EconomicLocation>& locations=economicLocations_.entries();
        for(std::size_t d=0;d<locations.size();++d) {
            const std::string& destination=locations[d].storageId;
            if(locations[d].canonicalId==originLocation.canonicalId || destination==origin || !ensureCargoServiceInventory(destination)) continue;
            for(std::size_t c=0;c<cargoCatalog_.entries().size();++c) {
                const oem::CargoDefinition& item=cargoCatalog_.entries()[c];
                if(stock->second.quantity(item.cargoId)<=0 || item.buyPrice<=0) continue;
                oem::MissionCandidate candidate;
                candidate.issuerId=issuerForLocation(originLocation.canonicalId);
                candidate.cargoId=item.cargoId;
                candidate.destinationLocationId=destination;
                candidate.destinationBodyId=locations[d].celestialBody;
                candidate.destinationCanonicalId=locations[d].canonicalId;
                candidate.quantity=1;
                VECTOR3 destinationGlobal = _V(0,0,0);
                OBJHANDLE destinationBody = 0;
                double destinationLng=0.0, destinationLat=0.0, destinationRadius=0.0;
                if (!resolveEconomicLocationPosition(locations[d], destinationGlobal,
                        destinationBody, destinationLng, destinationLat, destinationRadius))
                    continue;
                oem::MissionRouteDistance route;
                if (locations[d].kind == oem::ECONOMIC_LOCATION_SPACEPORT &&
                    originLocation.kind == oem::ECONOMIC_LOCATION_SPACEPORT &&
                    destinationBody == originBody && originRadius > 0.0) {
                    route.metres = oem::surfaceDistanceMetres(originLat, originLng,
                        destinationLat, destinationLng, originRadius);
                    route.kind = oem::MISSION_ROUTE_SURFACE;
                } else {
                    const double dx = destinationGlobal.x - originGlobal.x;
                    const double dy = destinationGlobal.y - originGlobal.y;
                    const double dz = destinationGlobal.z - originGlobal.z;
                    route.metres = sqrt(dx*dx + dy*dy + dz*dz);
                    route.kind = oem::MISSION_ROUTE_ESTIMATE;
                }
                route.valid = _finite(route.metres) && route.metres >= 0.0;
                const oem::MissionEconomicsConfig economicsConfig;
                const oem::MissionQuote quote = oem::quoteMission(
                    item.buyPrice, candidate.quantity, route, economicsConfig);
                if (!quote.valid) continue;
                candidate.reward = quote.reward;
                candidate.allowanceSeconds = quote.allowanceSeconds;
                const unsigned long flavor = deliveryDestinationScore(
                    sessionId_+"|"+originLocation.canonicalId+"|"+locations[d].canonicalId+"|"+item.cargoId,
                    "mission-flavor") % 3UL;
                if (flavor == 0) {
                    candidate.missionType = "SUPPLY";
                    candidate.missionTitle = "Routine supply run";
                    candidate.missionBriefing = "The local operator requests a routine replenishment of "+item.cargoId+
                        ". Load the specified cargo here and deliver it to "+destination+".";
                } else if (flavor == 1) {
                    candidate.missionType = "PRIORITY";
                    candidate.missionTitle = "Priority cargo delivery";
                    candidate.missionBriefing = "A reported shortage has made this shipment a priority. Deliver "+
                        item.cargoId+" to "+destination+" before the quoted deadline.";
                } else if (route.kind == oem::MISSION_ROUTE_SURFACE) {
                    candidate.missionType = "REGIONAL";
                    candidate.missionTitle = "Regional logistics";
                    candidate.missionBriefing = "A nearby facility needs "+item.cargoId+
                        ". Complete this regional surface transfer to "+destination+".";
                } else {
                    candidate.missionType = "REMOTE";
                    candidate.missionTitle = "Long-range logistics";
                    candidate.missionBriefing = "A remote facility has requested "+item.cargoId+
                        ". Deliver the shipment to "+destination+"; the route allowance is an estimate, not a flight plan.";
                }
                candidates.push_back(candidate);
            }
        }
        unsigned long generation=existing==missionBoards_.end()?1:existing->second.generation+1;
        char gen[32]; sprintf(gen,"%lu",generation);
        oem::MissionBoard generated;
        const oem::Result result=missionBoardStore_.generate(origin,originLocation.canonicalId,day,generation,
            sessionId_+"|"+originLocation.canonicalId+"|"+gen,candidates,generated);
        if(!result.ok) return false;
        std::map<std::string,oem::MissionBoard> updated=missionBoards_;
        updated[originLocation.canonicalId]=generated;
        const oem::Result saved=missionBoardStore_.save(missionBoardPath_,updated);
        if(!saved.ok) {
            char message[512]; sprintf(message,"OEM Mission Board: save failed (%s): %s",
                saved.code.c_str(),saved.message.c_str()); oapiWriteLog(message);
            return false;
        }
        missionBoards_.swap(updated);
        offers=generated.offers;
        return !offers.empty();
    }

    oem::Result acceptDeliveryOffer(const std::string& vesselId, std::size_t offerIndex) {
        if(!isFocusedVessel(vesselId))
            return oem::Result::failure("CONTRACT_FOCUS","Accept a mission from its assigned vessel.");
        const std::string origin=currentMarketLocation(vesselId);
        oem::EconomicLocation originLocation;
        if(origin.empty() || !resolveCurrentEconomicLocation(vesselId,origin,originLocation))
            return oem::Result::failure("CONTRACT_ORIGIN","Could not resolve the canonical origin location.");
        std::map<std::string,oem::MissionBoard>::iterator board=missionBoards_.find(originLocation.canonicalId);
        if(board==missionBoards_.end() || offerIndex>=board->second.offers.size())
            return oem::Result::failure("CONTRACT_OFFER_UNAVAILABLE","Selected mission is no longer on this location's board.");
        oem::DeliveryContract selected=board->second.offers[offerIndex];
        if(selected.originLocationId!=origin)
            return oem::Result::failure("CONTRACT_ORIGIN","Selected mission belongs to another location.");
        selected.vesselId=vesselId;
        if(selected.issuerId.empty()) selected.issuerId="issuer:"+originLocation.canonicalId;
        if (currentSimulationTime_ < 0 || selected.deadline < selected.acceptedAt)
            return oem::Result::failure("CONTRACT_TIME","Mission quote contains an invalid deadline window.");
        const oem::SimulationSeconds allowance = selected.deadline - selected.acceptedAt;
        if (allowance <= 0 || allowance > LLONG_MAX - currentSimulationTime_)
            return oem::Result::failure("CONTRACT_TIME","Mission deadline allowance is outside the supported range.");
        // The quote is generated at the start of the board day; the full quoted
        // allowance begins when the player actually accepts it.
        selected.acceptedAt = currentSimulationTime_;
        selected.deadline = currentSimulationTime_ + allowance;
        ensureCargoManifest(vesselId);
        if(!ensureCargoServiceInventory(origin))
            return oem::Result::failure("CONTRACT_ORIGIN_STOCK",
                "The origin cargo stock is not available for automatic mission loading.");
        std::map<std::string,oem::CargoManifest>::iterator manifest=cargoManifests_.find(vesselId);
        if(manifest==cargoManifests_.end())
            return oem::Result::failure("CONTRACT_MANIFEST",
                "The assigned vessel's OEM cargo manifest is unavailable.");
        oem::CargoDefinition cargoDefinition;
        if(!cargoCatalog_.find(selected.cargoId,cargoDefinition))
            return oem::Result::failure("CONTRACT_CARGO",
                "The mission cargo is missing from the configured cargo catalog.");
        const oem::Result accepted=economyJournalStore_.acceptContractWithCargo(
            economyJournalPath_,contractBook_,cargoServiceInventories_,
            manifest->second,selected,cargoDefinition.unitMassKg);
        if(!accepted.ok) return accepted;
        oem::MissionBoard updated=board->second;
        updated.offers.erase(updated.offers.begin()+offerIndex);
        std::map<std::string,oem::MissionBoard> candidate=missionBoards_;
        candidate[originLocation.canonicalId]=updated;
        const oem::Result saved=missionBoardStore_.save(missionBoardPath_,candidate);
        if(!saved.ok) {
            // Acceptance is already durable in the economy journal. Keep the
            // in-memory board consistent and report the persistence issue.
            board->second=updated;
            return oem::Result::failure("MISSION_BOARD_SAVE",
                "Mission accepted, but the remaining board could not be saved; it will be reconciled on restart.");
        }
        missionBoards_.swap(candidate);
        return oem::Result::success();
    }

    bool currentObjectiveOffers(const std::string& vesselId,
                                std::vector<oem::MissionObjectiveOffer>& offers) {
        offers.clear();
        oem::Credits companyBalance = 0;
        if (!economyReady_ || !objectiveBoardsReady_ || sessionId_.empty() ||
            currentSimulationTime_ <= 0 || vesselId.empty() || !isFocusedVessel(vesselId) ||
            !ledger_.getBalance("company", companyBalance))
            return false;
        std::size_t active = 0;
        const std::vector<oem::MissionObjectiveContract>& accepted = objectiveBook_.missions();
        for (std::size_t i = 0; i < accepted.size(); ++i)
            if (accepted[i].objective.vesselId == vesselId &&
                (accepted[i].status == oem::MISSION_OBJECTIVE_ACCEPTED ||
                 accepted[i].status == oem::MISSION_OBJECTIVE_COMPLETED))
                ++active;
        if (active >= 3) return false;
        const std::string origin = currentObjectiveLocation(vesselId);
        oem::EconomicLocation originLocation;
        if (origin.empty() || !resolveCurrentEconomicLocation(vesselId, origin, originLocation))
            return false;
        const long long day = (long long)currentSimulationTime_ / 86400;
        std::map<std::string, oem::MissionObjectiveBoard>::iterator existing =
            objectiveBoards_.find(originLocation.canonicalId);
        if (existing != objectiveBoards_.end() && existing->second.simulationDay == day) {
            oem::MissionObjectiveBoard pruned = existing->second;
            bool changed = false;
            for (std::size_t i = 0; i < pruned.offers.size();) {
                const oem::MissionObjectiveOffer& offer = pruned.offers[i];
                const oem::EconomicLocation* target = economicLocations_.find(offer.targetCanonicalId);
                const bool targetValid = target && target->storageId == offer.targetLocationId &&
                    ((offer.kind == oem::MISSION_OBJECTIVE_VISIT_SPACEPORT &&
                      target->kind == oem::ECONOMIC_LOCATION_SPACEPORT) ||
                     (offer.kind == oem::MISSION_OBJECTIVE_DOCK_STATION &&
                      target->kind == oem::ECONOMIC_LOCATION_STATION));
                if (objectiveBook_.find(offer.missionId) || !targetValid) {
                    pruned.offers.erase(pruned.offers.begin() + i);
                    changed = true;
                } else ++i;
            }
            if (changed) {
                std::map<std::string, oem::MissionObjectiveBoard> updated = objectiveBoards_;
                updated[originLocation.canonicalId] = pruned;
                const oem::Result saved = objectiveBoardStore_.save(objectiveBoardPath_, updated);
                if (!saved.ok) return false;
                objectiveBoards_.swap(updated);
            }
            offers = pruned.offers;
            return !offers.empty();
        }

        VECTOR3 originGlobal = _V(0,0,0);
        OBJHANDLE originBody = 0;
        double originLng = 0.0, originLat = 0.0, originRadius = 0.0;
        if (!resolveEconomicLocationPosition(originLocation, originGlobal, originBody,
                originLng, originLat, originRadius)) return false;

        std::vector<oem::MissionObjectiveCandidate> candidates;
        const std::vector<oem::EconomicLocation>& locations = economicLocations_.entries();
        for (std::size_t i = 0; i < locations.size(); ++i) {
            const oem::EconomicLocation& target = locations[i];
            if (target.canonicalId == originLocation.canonicalId) continue;
            if (target.kind != oem::ECONOMIC_LOCATION_SPACEPORT &&
                target.kind != oem::ECONOMIC_LOCATION_STATION) continue;
            VECTOR3 targetGlobal = _V(0,0,0);
            OBJHANDLE targetBody = 0;
            double targetLng = 0.0, targetLat = 0.0, targetRadius = 0.0;
            if (!resolveEconomicLocationPosition(target, targetGlobal, targetBody,
                    targetLng, targetLat, targetRadius)) continue;
            oem::MissionRouteDistance route;
            if (target.kind == oem::ECONOMIC_LOCATION_SPACEPORT &&
                originLocation.kind == oem::ECONOMIC_LOCATION_SPACEPORT &&
                targetBody == originBody && originRadius > 0.0) {
                route.metres = oem::surfaceDistanceMetres(originLat, originLng,
                    targetLat, targetLng, originRadius);
                route.kind = oem::MISSION_ROUTE_SURFACE;
            } else {
                const double dx = targetGlobal.x - originGlobal.x;
                const double dy = targetGlobal.y - originGlobal.y;
                const double dz = targetGlobal.z - originGlobal.z;
                route.metres = sqrt(dx*dx + dy*dy + dz*dz);
                route.kind = oem::MISSION_ROUTE_ESTIMATE;
            }
            route.valid = _finite(route.metres) && route.metres >= 0.0;
            oem::MissionEconomicsConfig config;
            if (route.kind == oem::MISSION_ROUTE_SURFACE)
                config.nominalCruiseSpeedMetresPerSecond = 250.0;
            const oem::MissionQuote quote = oem::quoteMission(500, 1, route, config);
            if (!quote.valid) continue;
            oem::MissionObjectiveCandidate candidate;
            candidate.issuerId = issuerForLocation(originLocation.canonicalId);
            candidate.kind = target.kind == oem::ECONOMIC_LOCATION_SPACEPORT
                ? oem::MISSION_OBJECTIVE_VISIT_SPACEPORT : oem::MISSION_OBJECTIVE_DOCK_STATION;
            candidate.targetLocationId = target.storageId;
            candidate.targetBodyId = target.celestialBody;
            candidate.targetCanonicalId = target.canonicalId;
            candidate.reward = quote.reward;
            candidate.allowanceSeconds = quote.allowanceSeconds;
            if (candidate.kind == oem::MISSION_OBJECTIVE_VISIT_SPACEPORT) {
                candidate.missionType = "VISIT";
                candidate.missionTitle = "Spaceport visit";
                candidate.missionBriefing = "Visit " + target.displayName + " on " +
                    target.celestialBody + ". Orbiter must record a landing at this exact base with ground contact.";
            } else {
                candidate.missionType = "DOCK";
                candidate.missionTitle = "Station rendezvous";
                candidate.missionBriefing = "Rendezvous with " + target.displayName +
                    ". Completion requires an actual Orbiter docking transition with this registered station.";
            }
            candidates.push_back(candidate);
        }

        const unsigned long generation = existing == objectiveBoards_.end()
            ? 1 : existing->second.generation + 1;
        char generationText[32]; sprintf(generationText, "%lu", generation);
        oem::MissionObjectiveBoard generated;
        const oem::Result result = objectiveBoardStore_.generate(origin, originLocation.canonicalId,
            day, generation, sessionId_ + "|" + originLocation.canonicalId + "|" +
            generationText + "|independent", candidates, generated);
        if (!result.ok) return false;
        std::map<std::string, oem::MissionObjectiveBoard> updated = objectiveBoards_;
        updated[originLocation.canonicalId] = generated;
        const oem::Result saved = objectiveBoardStore_.save(objectiveBoardPath_, updated);
        if (!saved.ok) {
            char message[512];
            sprintf(message, "OEM Independent Mission Board: save failed (%s): %s",
                saved.code.c_str(), saved.message.c_str());
            oapiWriteLog(message);
            return false;
        }
        objectiveBoards_.swap(updated);
        for (std::size_t i = 0; i < generated.offers.size(); ++i)
            if (!objectiveBook_.find(generated.offers[i].missionId))
                offers.push_back(generated.offers[i]);
        return !offers.empty();
    }

    oem::Result acceptObjectiveOffer(const std::string& vesselId, std::size_t offerIndex) {
        if (!economyReady_ || economyJournalPath_.empty())
            return oem::Result::failure("MISSION_ECONOMY_UNAVAILABLE", "The global economy journal is unavailable.");
        if (!isFocusedVessel(vesselId))
            return oem::Result::failure("MISSION_FOCUS", "Accept the objective from its assigned vessel.");
        std::size_t activeObjectives = 0;
        const std::vector<oem::MissionObjectiveContract>& activeMissions = objectiveBook_.missions();
        for (std::size_t i = 0; i < activeMissions.size(); ++i)
            if (activeMissions[i].objective.vesselId == vesselId &&
                (activeMissions[i].status == oem::MISSION_OBJECTIVE_ACCEPTED ||
                 activeMissions[i].status == oem::MISSION_OBJECTIVE_COMPLETED))
                ++activeObjectives;
        if (activeObjectives >= 3)
            return oem::Result::failure("MISSION_LIMIT", "This vessel already has three active objective missions.");
        const std::string origin = currentObjectiveLocation(vesselId);
        oem::EconomicLocation originLocation;
        if (origin.empty() || !resolveCurrentEconomicLocation(vesselId, origin, originLocation))
            return oem::Result::failure("MISSION_ORIGIN", "Could not resolve the current economic location.");
        std::map<std::string, oem::MissionObjectiveBoard>::iterator board =
            objectiveBoards_.find(originLocation.canonicalId);
        if (board == objectiveBoards_.end() || offerIndex >= board->second.offers.size())
            return oem::Result::failure("MISSION_OFFER_UNAVAILABLE", "Selected objective offer is no longer available.");
        const oem::MissionObjectiveOffer offer = board->second.offers[offerIndex];
        if (objectiveBook_.find(offer.missionId))
            return oem::Result::failure("MISSION_ALREADY_ACCEPTED", "This objective mission has already been accepted.");
        const oem::EconomicLocation* target = economicLocations_.find(offer.targetCanonicalId);
        if (!target || target->storageId != offer.targetLocationId ||
            (offer.kind == oem::MISSION_OBJECTIVE_VISIT_SPACEPORT &&
             target->kind != oem::ECONOMIC_LOCATION_SPACEPORT) ||
            (offer.kind == oem::MISSION_OBJECTIVE_DOCK_STATION &&
             target->kind != oem::ECONOMIC_LOCATION_STATION))
            return oem::Result::failure("MISSION_TARGET_UNAVAILABLE", "The objective target is no longer a registered location.");
        oem::Credits companyFunds = 0;
        if (!ledger_.getBalance("company", companyFunds))
            return oem::Result::failure("MISSION_ACCOUNT_UNAVAILABLE", "A company account is required before accepting paid missions.");
        if (currentSimulationTime_ <= 0 || offer.allowanceSeconds <= 0 ||
            offer.allowanceSeconds > LLONG_MAX - currentSimulationTime_)
            return oem::Result::failure("MISSION_TIME", "Objective mission deadline is outside the supported range.");
        oem::MissionObjective objective;
        objective.missionId = offer.missionId;
        objective.issuerId = offer.issuerId.empty() ? "issuer:" + originLocation.canonicalId : offer.issuerId;
        objective.vesselId = vesselId;
        objective.kind = offer.kind;
        objective.originLocationId = originLocation.canonicalId;
        objective.targetLocationId = offer.targetLocationId;
        objective.targetBodyId = offer.targetBodyId;
        objective.acceptedAt = currentSimulationTime_;
        objective.deadline = currentSimulationTime_ + offer.allowanceSeconds;
        const oem::Result accepted = economyJournalStore_.acceptMissionObjective(
            economyJournalPath_, objectiveBook_, objective, offer.reward);
        if (!accepted.ok) return accepted;
        {
            char diagnostic[768];
            sprintf(diagnostic,
                "OEM MISSION ACCEPTED: id=%s kind=%d vessel=%s targetBody=%s targetBase=%s acceptedAt=%lld deadline=%lld",
                objective.missionId.c_str(), (int)objective.kind, objective.vesselId.c_str(),
                objective.targetBodyId.c_str(), objective.targetLocationId.c_str(),
                (long long)objective.acceptedAt, (long long)objective.deadline);
            oapiWriteLog(diagnostic);
        }
        oem::MissionObjectiveBoard updated = board->second;
        for (std::size_t i = 0; i < updated.offers.size(); ++i)
            if (updated.offers[i].missionId == offer.missionId) {
                updated.offers.erase(updated.offers.begin() + i);
                break;
            }
        std::map<std::string, oem::MissionObjectiveBoard> candidate = objectiveBoards_;
        candidate[originLocation.canonicalId] = updated;
        const oem::Result saved = objectiveBoardStore_.save(objectiveBoardPath_, candidate);
        if (!saved.ok) {
            board->second = updated;
            return oem::Result::failure("OBJECTIVE_BOARD_SAVE",
                "Objective accepted durably, but the remaining board could not be saved; it will be reconciled on restart.");
        }
        objectiveBoards_.swap(candidate);
        return oem::Result::success();
    }

    oem::Result cancelMissionObjective(const std::string& missionId, const std::string& vesselId) {
        if (!economyReady_ || economyJournalPath_.empty())
            return oem::Result::failure("MISSION_ECONOMY_UNAVAILABLE", "The global economy journal is unavailable.");
        if (!isFocusedVessel(vesselId))
            return oem::Result::failure("MISSION_FOCUS", "Cancel the objective from its assigned vessel.");
        const oem::MissionObjectiveContract* mission = objectiveBook_.find(missionId);
        if (!mission) return oem::Result::failure("MISSION_NOT_FOUND", "Objective mission does not exist.");
        if (mission->objective.vesselId != vesselId)
            return oem::Result::failure("MISSION_VESSEL", "This objective belongs to another vessel.");
        return economyJournalStore_.cancelMissionObjective(
            economyJournalPath_, objectiveBook_, missionId);
    }

    oem::Result cancelDeliveryContract(const std::string& contractId, const std::string& vesselId) {
        if (!economyReady_ || economyJournalPath_.empty())
            return oem::Result::failure("CONTRACT_ECONOMY_UNAVAILABLE", "Economy journal is unavailable.");
        if (!isFocusedVessel(vesselId))
            return oem::Result::failure("CONTRACT_FOCUS", "Cancel the contract from its assigned vessel.");
        const oem::DeliveryContract *contract = contractBook_.find(contractId);
        if (!contract) return oem::Result::failure("CONTRACT_NOT_FOUND", "Contract does not exist.");
        if (contract->vesselId != vesselId)
            return oem::Result::failure("CONTRACT_VESSEL", "This contract belongs to another vessel.");
        return economyJournalStore_.cancelContract(economyJournalPath_, contractBook_, contractId);
    }

    oem::Result settleDeliveryContract(const std::string& contractId) {
        if (!economyReady_ || economyJournalPath_.empty())
            return oem::Result::failure("CONTRACT_ECONOMY_UNAVAILABLE", "Economy journal is unavailable.");
        const oem::DeliveryContract *contract = contractBook_.find(contractId);
        if (!contract)
            return oem::Result::failure("CONTRACT_NOT_FOUND", "Contract does not exist.");
        std::map<std::string, oem::CargoManifest>::iterator manifest =
            cargoManifests_.find(contract->vesselId);
        if (manifest == cargoManifests_.end())
            return oem::Result::failure("CONTRACT_CARGO_MISSING", "Contract vessel cargo manifest is unavailable.");
        const bool wasSettled = contract->status == oem::CONTRACT_SETTLED;
        const oem::Credits reward = contract->reward;
        const std::string vesselId = contract->vesselId;
        const oem::Result result = economyJournalStore_.settleContract(
            economyJournalPath_, contractBook_, contractId, "company", ledger_);
        if (result.ok && !wasSettled) {
            char notice[512];
            sprintf(notice, "CONTRACT COMPLETED\nDelivery received\n+ %lld CR",
                    (long long)reward);
            showCompletionNotice(notice);
            char log[512];
            sprintf(log, "OEM CONTRACT SETTLED: id=%s vessel=%s reward=%lld",
                    contractId.c_str(), vesselId.c_str(), (long long)reward);
            oapiWriteLog(log);
        }
        return result;
    }

    bool companyBalance(oem::Credits& balance) const {
        return economyReady_ && ledger_.getBalance("company", balance);
    }

    bool companyReputation(int& reputation) const {
        if (!economyReady_) return false;
        if (ledger_.getReputation("company", reputation)) return true;
        oem::Credits balance = 0;
        if (!ledger_.getBalance("company", balance)) return false;
        reputation = 50;
        return true;
    }

    std::string issuerForLocation(const std::string& canonicalLocationId) const {
        return missionIssuerRegistry_.issuerForLocation(canonicalLocationId);
    }

    bool issuerReputation(const std::string& issuerId, int& reputation) const {
        if (!economyReady_ || issuerId.empty()) return false;
        if (ledger_.getReputation(issuerId, reputation)) return true;
        reputation = 50;
        return true;
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

    std::string dockedEconomicStation(const std::string& vesselId) const {
        OBJHANDLE focus = oapiGetFocusObject();
        if (!focus || !oapiIsVessel(focus)) return std::string();
        VESSEL* vessel = oapiGetVesselInterface(focus);
        if (!vessel || !vessel->GetName() || vesselId != vessel->GetName())
            return std::string();
        for (UINT i = 0; i < vessel->DockCount(); ++i) {
            DOCKHANDLE dock = vessel->GetDockHandle(i);
            if (!dock) continue;
            OBJHANDLE other = vessel->GetDockStatus(dock);
            if (!other || !oapiIsVessel(other)) continue;
            char name[128] = {};
            oapiGetObjectName(other, name, sizeof(name));
            if (name[0] && economicStations_.isStation(name)) return std::string(name);
        }
        return std::string();
    }

    std::string currentObjectiveLocation(const std::string& vesselId) const {
        const std::string service = currentMarketLocation(vesselId);
        if (!service.empty()) return service;
        return dockedEconomicStation(vesselId);
    }

    std::string currentMarketLocation(const std::string& vesselId) const {
        const std::string docked = dockedCargoService(vesselId);
        if (!docked.empty()) return docked;
        OBJHANDLE focus = oapiGetFocusObject();
        if (!focus || !oapiIsVessel(focus)) return std::string();
        VESSEL *vessel = oapiGetVesselInterface(focus);
        if (!vessel || !vessel->GetName() || vesselId != vessel->GetName() ||
            !(vessel->GetFlightStatus() & 1u) || !vessel->GroundContact())
            return std::string();
        OBJHANDLE body = vessel->GetSurfaceRef();
        if (!body) return std::string();
        double lng=0.0, lat=0.0, radius=0.0;
        if (vessel->GetEquPos(lng, lat, radius) != body) return std::string();
        const double bodyRadius = oapiGetSize(body);
        if (!(bodyRadius > 0.0) || !_finite(bodyRadius)) return std::string();
        double nearest = -1.0;
        std::string baseId;
        const DWORD count = oapiGetBaseCount(body);
        for (DWORD i=0; i<count; ++i) {
            OBJHANDLE base = oapiGetBaseByIndex(body, (int)i);
            if (!base) continue;
            double blng=0.0, blat=0.0, brad=0.0;
            oapiGetBaseEquPos(base, &blng, &blat, &brad);
            const double d = oem::surfaceDistanceMetres(lat, lng, blat, blng, bodyRadius);
            if (d >= 0.0 && (nearest < 0.0 || d < nearest)) {
                char name[128] = {};
                oapiGetObjectName(base, name, sizeof(name));
                nearest = d;
                baseId = name;
            }
        }
        return nearest >= 0.0 && nearest <= 5000.0 ? baseId : std::string();
    }

    bool resolveEconomicLocationPosition(const oem::EconomicLocation& location,
            VECTOR3& globalPosition, OBJHANDLE& bodyHandle,
            double& longitude, double& latitude, double& bodyRadius) const {
        bodyHandle = 0;
        longitude = latitude = bodyRadius = 0.0;
        if (location.kind == oem::ECONOMIC_LOCATION_SPACEPORT) {
            char bodyName[256] = {};
            char baseName[256] = {};
            strncpy(bodyName, location.celestialBody.c_str(), sizeof(bodyName)-1);
            strncpy(baseName, location.storageId.c_str(), sizeof(baseName)-1);
            bodyHandle = oapiGetObjectByName(bodyName);
            if (!bodyHandle || oapiGetObjectType(bodyHandle) != OBJTP_PLANET) return false;
            OBJHANDLE base = oapiGetBaseByName(bodyHandle, baseName);
            if (!base) return false;
            double radius = 0.0;
            oapiGetBaseEquPos(base, &longitude, &latitude, &radius);
            bodyRadius = oapiGetSize(bodyHandle);
            if (!(bodyRadius > 0.0) || !_finite(bodyRadius) ||
                !_finite(longitude) || !_finite(latitude)) return false;
            oapiEquToGlobal(bodyHandle, longitude, latitude, radius, &globalPosition);
            return true;
        }
        if (location.kind == oem::ECONOMIC_LOCATION_STATION) {
            char stationName[256] = {};
            strncpy(stationName, location.storageId.c_str(), sizeof(stationName)-1);
            OBJHANDLE stationHandle = oapiGetObjectByName(stationName);
            if (!stationHandle || !oapiIsVessel(stationHandle)) return false;
            VESSEL* station = oapiGetVesselInterface(stationHandle);
            if (!station) return false;
            bodyHandle = station->GetGravityRef();
            station->GetGlobalPos(globalPosition);
            if (bodyHandle) {
                double radius = 0.0;
                OBJHANDLE coordinateBody = station->GetEquPos(longitude, latitude, radius);
                if (coordinateBody == bodyHandle) bodyRadius = oapiGetSize(bodyHandle);
            }
            return true;
        }
        return false;
    }

    bool resolveCurrentEconomicLocation(const std::string& vesselId,
            const std::string& storageId, oem::EconomicLocation& location) const {
        std::string docked = dockedCargoService(vesselId);
        if (docked.empty()) docked = dockedEconomicStation(vesselId);
        if (!docked.empty()) {
            const oem::EconomicLocation* found =
                economicLocations_.find(std::string("station:") + docked);
            if (!found) return false;
            location = *found;
            return location.storageId == storageId;
        }
        OBJHANDLE focus = oapiGetFocusObject();
        if (!focus || !oapiIsVessel(focus)) return false;
        VESSEL* vessel = oapiGetVesselInterface(focus);
        if (!vessel || !vessel->GetName() || vesselId != vessel->GetName()) return false;
        OBJHANDLE body = vessel->GetSurfaceRef();
        if (!body) return false;
        char bodyName[256] = {};
        oapiGetObjectName(body, bodyName, sizeof(bodyName));
        const oem::EconomicLocation* found = economicLocations_.find(
            std::string("surface:") + bodyName + "/" + storageId);
        if (!found || found->kind != oem::ECONOMIC_LOCATION_SPACEPORT) return false;
        location = *found;
        return true;
    }

    oem::Result performCargoExchangeTest(const std::string& vesselId,
                                         oem::CargoTransferDirection direction) {
        if (sessionId_.empty() || currentSimulationTime_ <= 0)
            return oem::Result::failure("CARGO_TEST_NOT_READY",
                "Wait until the simulation has advanced beyond its initialization frame.");
        const std::string serviceId = currentMarketLocation(vesselId);
        if (serviceId.empty())
            return oem::Result::failure("CARGO_TEST_NO_LOCATION",
                "Land within 5 km of a surface base or dock to a registered cargo service.");
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

    oem::Result performMarketTrade(const std::string& vesselId, oem::MarketSide side) {
        if (!economyReady_ || economyJournalPath_.empty())
            return oem::Result::failure("MARKET_UNAVAILABLE",
                "The global economy is not ready. Check accounts.cfg, cargo_catalog.cfg and the economy journal.");
        if (sessionId_.empty() || currentSimulationTime_ <= 0)
            return oem::Result::failure("MARKET_NOT_READY",
                "Wait until the simulation has advanced beyond its initialization frame.");
        const std::string serviceId = currentMarketLocation(vesselId);
        if (serviceId.empty())
            return oem::Result::failure("MARKET_NO_LOCATION",
                "Land within 5 km of a planetary spaceport or dock to a registered station.");
        ensureCargoManifest(vesselId);
        if (!ensureCargoServiceInventory(serviceId))
            return oem::Result::failure("MARKET_STOCK_UNAVAILABLE",
                "The registered station's explicit stock configuration is unavailable.");

        oem::Credits balance = 0;
        if (!ledger_.getBalance("company", balance))
            return oem::Result::failure("MARKET_ACCOUNT_MISSING",
                "accounts.cfg must define the company account ID 'company' before trading.");
        std::map<std::string, oem::CargoManifest>::iterator manifest = cargoManifests_.find(vesselId);
        std::map<std::string, oem::CargoServiceInventory>::iterator inventory =
            cargoServiceInventories_.find(serviceId);
        if (manifest == cargoManifests_.end() || inventory == cargoServiceInventories_.end())
            return oem::Result::failure("MARKET_PROJECTION_UNAVAILABLE",
                "The station inventory or vessel manifest is unavailable.");

        const std::vector<oem::CargoDefinition>& definitions = cargoCatalog_.entries();
        std::string selectedCargo;
        for (std::size_t i = 0; i < definitions.size(); ++i) {
            const std::string& cargoId = definitions[i].cargoId;
            if (side == oem::MARKET_BUY_FROM_STATION) {
                if (inventory->second.quantity(cargoId) > 0) {
                    selectedCargo = cargoId;
                    break;
                }
            } else if (side == oem::MARKET_SELL_TO_STATION) {
                if (manifest->second.quantity(cargoId) > 0) {
                    selectedCargo = cargoId;
                    break;
                }
            } else {
                return oem::Result::failure("MARKET_SIDE", "Unknown market operation.");
            }
        }
        if (selectedCargo.empty())
            return oem::Result::failure(side == oem::MARKET_BUY_FROM_STATION
                ? "MARKET_NO_STOCK" : "MARKET_NO_CARGO",
                side == oem::MARKET_BUY_FROM_STATION
                ? "No catalog item has available station stock."
                : "The vessel has no catalog cargo available to sell.");

        char operationId[256];
        sprintf(operationId, "OEMMARKET-%s-%lu", sessionId_.c_str(),
                ++cargoTransferSequence_);
        oem::MarketQuote quote;
        oem::MarketQuoteEngine quoteEngine;
        const oem::Result quoted = quoteEngine.createQuote(
            cargoCatalog_, inventory->second, std::string("QUOTE-") + operationId,
            selectedCargo, side, 1, currentSimulationTime_, quote);
        if (!quoted.ok) return quoted;

        const oem::Result committed = economyJournalStore_.applyTrade(
            economyJournalPath_, cargoCatalog_, quote, operationId, "company",
            ledger_, cargoServiceInventories_, manifest->second);
        if (!committed.ok) return committed;

        char message[512];
        sprintf(message, "OEM MARKET: %s cargo=%s quantity=1 total=%lld company_balance=%lld",
            side == oem::MARKET_BUY_FROM_STATION ? "BUY" : "SELL",
            selectedCargo.c_str(), (long long)quote.totalPrice,
            (long long)(balance + (side == oem::MARKET_BUY_FROM_STATION
                ? -quote.totalPrice : quote.totalPrice)));
        oapiWriteLog(message);
        return oem::Result::success();
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
        clearCompletionNotice();
        landingMonitors_.clear();
        flightMonitors_.clear();
        dockingMonitors_.clear();
        cargoManifests_.clear();
        cargoManifestLoadAttempted_.clear();
        contractBook_ = oem::DeliveryContractBook();
        objectiveBook_ = oem::MissionObjectiveBook();
        pendingContractOffers_.clear();
        contractOfferSequence_ = 0;
        missionBoards_.clear();
        missionBoardsReady_ = false;
        objectiveBoards_.clear();
        objectiveBoardsReady_ = false;
        if (!missionBoardPath_.empty()) {
            const DWORD boardAttributes = GetFileAttributesA(missionBoardPath_.c_str());
            if (boardAttributes == INVALID_FILE_ATTRIBUTES &&
                (GetLastError() == ERROR_FILE_NOT_FOUND || GetLastError() == ERROR_PATH_NOT_FOUND)) {
                missionBoardsReady_ = true;
            } else if (boardAttributes != INVALID_FILE_ATTRIBUTES) {
                const oem::Result boardLoad = missionBoardStore_.load(missionBoardPath_, missionBoards_);
                missionBoardsReady_ = boardLoad.ok;
                if (!boardLoad.ok) {
                    char boardError[512]; sprintf(boardError, "OEM Mission Board: load failed (%s): %s", boardLoad.code.c_str(), boardLoad.message.c_str());
                    oapiWriteLog(boardError);
                }
            }
        }
        if (!objectiveBoardPath_.empty()) {
            const DWORD objectiveAttributes = GetFileAttributesA(objectiveBoardPath_.c_str());
            if (objectiveAttributes == INVALID_FILE_ATTRIBUTES &&
                (GetLastError() == ERROR_FILE_NOT_FOUND || GetLastError() == ERROR_PATH_NOT_FOUND)) {
                objectiveBoardsReady_ = true;
            } else if (objectiveAttributes != INVALID_FILE_ATTRIBUTES) {
                const oem::Result objectiveLoad = objectiveBoardStore_.load(
                    objectiveBoardPath_, objectiveBoards_);
                objectiveBoardsReady_ = objectiveLoad.ok;
                if (!objectiveLoad.ok) {
                    char objectiveError[512];
                    sprintf(objectiveError, "OEM Independent Mission Board: load failed (%s): %s",
                        objectiveLoad.code.c_str(), objectiveLoad.message.c_str());
                    oapiWriteLog(objectiveError);
                }
            }
        }
        cargoServiceInventories_.clear();
        cargoServiceInventoryLoadAttempted_.clear();
        primaryResources_ = oem::PrimaryResourceBook();
        primaryResourcesReady_ = false;
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
        cargoServicesReady_ = false;
        const oem::Result servicesLoaded = cargoServicesPath_.empty()
            ? oem::Result::failure("CARGO_SERVICES_PATH", "Could not resolve cargo service configuration path.")
            : cargoServices_.load(cargoServicesPath_);
        cargoServicesReady_ = servicesLoaded.ok;
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
        economicStations_.clear();
        economicStationsReady_ = false;
        const oem::Result stationsLoaded = economicStationsPath_.empty()
            ? oem::Result::failure("ECONOMIC_STATIONS_PATH", "Could not resolve station registry path.")
            : economicStations_.load(economicStationsPath_);
        economicStationsReady_ = stationsLoaded.ok;
        if (!stationsLoaded.ok) {
            char message[512];
            sprintf(message, "OEM Economic Stations: load failed (%s): %s",
                    stationsLoaded.code.c_str(), stationsLoaded.message.c_str());
            oapiWriteLog(message);
        } else {
            char message[256];
            sprintf(message, "OEM Economic Stations: loaded %lu explicit stations.",
                    (unsigned long)economicStations_.size());
            oapiWriteLog(message);
        }
        // Build the economic-location index before journal recovery: replay may
        // need a planetary destination inventory even when the player is elsewhere.
        rebuildSpaceportRegistry();
        if (servicesLoaded.ok) {
            const std::set<std::string>& serviceIds = cargoServices_.vesselIds();
            for (std::set<std::string>::const_iterator it = serviceIds.begin(); it != serviceIds.end(); ++it)
                ensureCargoServiceInventory(*it);
            const std::vector<oem::EconomicLocation>& locations = economicLocations_.entries();
            for (std::size_t i = 0; i < locations.size(); ++i)
                ensureCargoServiceInventory(locations[i].storageId);
            // Restore opening stock for legacy surface locations already referenced
            // by the event journal before replaying the global economy journal.
            const std::vector<oem::SimulationEvent>& priorEvents = eventQueue_.events();
            for (std::size_t i = 0; i < priorEvents.size(); ++i)
                if (priorEvents[i].type == oem::SIM_EVENT_LANDING &&
                    priorEvents[i].locationKnown && !priorEvents[i].baseId.empty())
                    ensureCargoServiceInventory(priorEvents[i].baseId);
            initializeEconomyState();
        }
        oapiWriteLog((char*)"OEM Landing Bridge: simulation monitoring started.");
    }

    void clbkSimulationEnd() override {
        clearCompletionNotice();
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
        updateCompletionNotice(simSeconds);

        if (economyReady_) {
            reconcileObjectiveEvents();
            settleCompletedContracts();
            settleCompletedObjectives();
            const oem::SimulationSeconds primaryTime =
                (mjd > 0.0 && _finite(mjd))
                    ? (oem::SimulationSeconds)(mjd * 86400.0 + 0.5)
                    : simSeconds;
            updatePrimaryProduction(primaryTime);
            oem::MissionObjectiveBook objectiveDue = objectiveBook_;
            const std::vector<std::string> expiredObjectives = objectiveDue.expireDue(simSeconds);
            for (std::size_t i = 0; i < expiredObjectives.size(); ++i) {
                const oem::Result committed = economyJournalStore_.expireMissionObjective(
                    economyJournalPath_, objectiveBook_, expiredObjectives[i], ledger_, "company", simSeconds, missionFailurePolicy_);
                if (!committed.ok) {
                    char error[512];
                    sprintf(error, "OEM MISSION: expiry commit failed id=%s (%s): %s",
                        expiredObjectives[i].c_str(), committed.code.c_str(), committed.message.c_str());
                    oapiWriteLog(error);
                }
            }
            oem::DeliveryContractBook due = contractBook_;
            const std::vector<std::string> expired = due.expireDue(simSeconds);
            for (std::size_t i = 0; i < expired.size(); ++i) {
                const oem::Result committed = economyJournalStore_.expireContract(
                    economyJournalPath_, contractBook_, expired[i], ledger_, "company", simSeconds, missionFailurePolicy_);
                if (!committed.ok) {
                    char error[512];
                    sprintf(error, "OEM CONTRACT: expiry commit failed id=%s (%s): %s",
                        expired[i].c_str(), committed.code.c_str(), committed.message.c_str());
                    oapiWriteLog(error);
                }
            }
        }

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

        // Register any currently occupied surface base, including scenarios
        // that start with the vessel already parked (startup is not a landing event).
        if (landing.landed && landing.groundContact && landing.proximityKnown &&
            !landing.nearestBaseId.empty() &&
            landing.nearestBaseDistanceMetres >= 0.0 &&
            landing.nearestBaseDistanceMetres <= 5000.0 &&
            cargoServiceInventoryLoadAttempted_.find(landing.nearestBaseId) ==
                cargoServiceInventoryLoadAttempted_.end()) {
            if (!ensureCargoServiceInventory(landing.nearestBaseId)) {
                char stockMessage[512];
                sprintf(stockMessage,
                    "OEM SPACEPORT STOCK: initialization failed base=%s; market access will remain unavailable.",
                    landing.nearestBaseId.c_str());
                oapiWriteLog(stockMessage);
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
        const bool landingDetected = landingMonitors_[hVessel].observe(landing, landingEvent);
        static std::set<std::string> landingDiagnosticLogged;
        if (!landing.groundContact) landingDiagnosticLogged.erase(landing.vesselId);
        if (landing.groundContact && landing.landed && landing.proximityKnown &&
            landingDiagnosticLogged.insert(landing.vesselId).second) {
            char diagnostic[768];
            sprintf(diagnostic,
                "OEM LANDING SAMPLE: vessel=%s landed=%d contact=%d body=%s nearestBase=%s distance=%.1f m detected=%d simt=%lld",
                landing.vesselId.c_str(), landing.landed ? 1 : 0,
                landing.groundContact ? 1 : 0, landing.bodyId.c_str(),
                landing.nearestBaseId.c_str(), landing.nearestBaseDistanceMetres,
                landingDetected ? 1 : 0, (long long)landing.simulationTime);
            oapiWriteLog(diagnostic);
        }
        if (landingDetected) {
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
    void showCompletionNotice(const char *text) {
        if (!text || !*text) return;
        if (!completionNote_)
            completionNote_ = oapiCreateAnnotation(false, 1.25, _V(0.25, 1.0, 0.35));
        if (!completionNote_) {
            oapiWriteLog((char*)"OEM CONTRACT: could not create completion annotation.");
            return;
        }
        oapiAnnotationSetPos(completionNote_, 0.42, 0.40, 0.86, 0.60);
        oapiAnnotationSetSize(completionNote_, 1.25);
        oapiAnnotationSetColour(completionNote_, _V(0.25, 1.0, 0.35));
        char buffer[512];
        strncpy(buffer, text, sizeof(buffer) - 1);
        buffer[sizeof(buffer) - 1] = '\0';
        oapiAnnotationSetText(completionNote_, buffer);
        notificationUntil_ = currentSimulationTime_ + 7;
    }

    void updateCompletionNotice(oem::SimulationSeconds now) {
        if (completionNote_ && notificationUntil_ > 0 && now >= notificationUntil_)
            clearCompletionNotice();
    }

    void clearCompletionNotice() {
        if (completionNote_) {
            oapiDelAnnotation(completionNote_);
            completionNote_ = 0;
        }
        notificationUntil_ = 0;
    }

    void settleCompletedContracts() {
        std::vector<std::string> pending;
        const std::vector<oem::DeliveryContract>& contracts = contractBook_.contracts();
        for (std::size_t i = 0; i < contracts.size(); ++i)
            if (contracts[i].status == oem::CONTRACT_COMPLETED)
                pending.push_back(contracts[i].contractId);
        for (std::size_t i = 0; i < pending.size(); ++i) {
            const oem::Result settled = settleDeliveryContract(pending[i]);
            if (!settled.ok) {
                char error[512];
                sprintf(error, "OEM CONTRACT: automatic settlement retry failed id=%s (%s): %s",
                    pending[i].c_str(), settled.code.c_str(), settled.message.c_str());
                oapiWriteLog(error);
            }
        }
    }

    void reconcileObjectiveEvents() {
        if (!economyReady_) return;
        const std::vector<oem::SimulationEvent>& events = eventQueue_.events();
        std::vector<std::string> acceptedIds;
        const std::vector<oem::MissionObjectiveContract>& missions = objectiveBook_.missions();
        for (std::size_t i = 0; i < missions.size(); ++i)
            if (missions[i].status == oem::MISSION_OBJECTIVE_ACCEPTED)
                acceptedIds.push_back(missions[i].objective.missionId);
        for (std::size_t i = 0; i < acceptedIds.size(); ++i) {
            const oem::MissionObjectiveContract* mission = objectiveBook_.find(acceptedIds[i]);
            if (!mission) continue;
            std::map<std::string, oem::CargoManifest>::const_iterator manifest =
                cargoManifests_.find(mission->objective.vesselId);
            if (manifest == cargoManifests_.end()) continue;
            for (std::size_t n = events.size(); n > 0; --n) {
                const oem::SimulationEvent& event = events[n - 1];
                if (event.vesselId != mission->objective.vesselId ||
                    !oem::missionObjectiveMatchesEvent(mission->objective, event, manifest->second))
                    continue;
                oem::MissionObjectiveBook candidate = objectiveBook_;
                const oem::Result marked = candidate.restoreCompleted(
                    acceptedIds[i], event.eventId, event.simulationTime);
                if (!marked.ok) break;
                const oem::MissionObjectiveContract* completed = candidate.find(acceptedIds[i]);
                const oem::Result committed = completed
                    ? economyJournalStore_.completeMissionObjective(
                        economyJournalPath_, candidate, acceptedIds[i],
                        completed->completionEventId, completed->completedAt)
                    : oem::Result::failure("MISSION_OBJECTIVE_NOT_FOUND", "Completed objective disappeared.");
                if (committed.ok) {
                    objectiveBook_ = candidate;
                    char message[512];
                    sprintf(message, "OEM MISSION EVENT RECOVERED: id=%s event=%s",
                        acceptedIds[i].c_str(), event.eventId.c_str());
                    oapiWriteLog(message);
                } else {
                    char error[512];
                    sprintf(error, "OEM MISSION: event recovery commit failed id=%s (%s): %s",
                        acceptedIds[i].c_str(), committed.code.c_str(), committed.message.c_str());
                    oapiWriteLog(error);
                }
                break;
            }
        }
    }

    void settleCompletedObjectives() {
        std::vector<std::string> pending;
        const std::vector<oem::MissionObjectiveContract>& missions = objectiveBook_.missions();
        for (std::size_t i = 0; i < missions.size(); ++i)
            if (missions[i].status == oem::MISSION_OBJECTIVE_COMPLETED)
                pending.push_back(missions[i].objective.missionId);
        for (std::size_t i = 0; i < pending.size(); ++i) {
            const oem::MissionObjectiveContract* item = objectiveBook_.find(pending[i]);
            if (!item) continue;
            const oem::Credits reward = item->reward;
            const oem::Result settled = economyJournalStore_.settleMissionObjective(
                economyJournalPath_, objectiveBook_, pending[i], "company", ledger_);
            if (settled.ok) {
                char notice[512];
                sprintf(notice, "MISSION COMPLETED\nObjective verified\n+ %lld CR", (long long)reward);
                showCompletionNotice(notice);
                char log[512];
                sprintf(log, "OEM MISSION SETTLED: id=%s reward=%lld", pending[i].c_str(), (long long)reward);
                oapiWriteLog(log);
            } else {
                char error[512];
                sprintf(error, "OEM MISSION: automatic settlement retry failed id=%s (%s): %s",
                    pending[i].c_str(), settled.code.c_str(), settled.message.c_str());
                oapiWriteLog(error);
            }
        }
    }

    // Rebuild from Orbiter handles every scenario start; the saved registry is a derived snapshot.
    void rebuildSpaceportRegistry() {
        oem::SpaceportRegistry registry;
        const DWORD bodyCount = oapiGetGbodyCount();
        unsigned long baseCount = 0;
        unsigned long padCount = 0;
        const double gravitationalConstant = 6.67430e-11;
        for (DWORD bodyIndex = 0; bodyIndex < bodyCount; ++bodyIndex) {
            OBJHANDLE body = oapiGetGbodyByIndex((int)bodyIndex);
            if (!body) continue;
            char bodyName[128] = {};
            oapiGetObjectName(body, bodyName, sizeof(bodyName));
            if (bodyName[0] == '\\0') continue;
            const double bodyRadius = oapiGetSize(body);
            const double bodyMass = oapiGetMass(body);
            if (!(bodyRadius > 0.0) || !(bodyMass > 0.0) ||
                !_finite(bodyRadius) || !_finite(bodyMass)) continue;

            const DWORD count = oapiGetBaseCount(body);
            for (DWORD baseIndex = 0; baseIndex < count; ++baseIndex) {
                OBJHANDLE base = oapiGetBaseByIndex(body, (int)baseIndex);
                if (!base) continue;
                char baseName[128] = {};
                oapiGetObjectName(base, baseName, sizeof(baseName));
                if (baseName[0] == '\\0') continue;

                oem::SpaceportRecord record;
                record.bodyName = bodyName;
                record.baseName = baseName;
                record.id = record.bodyName + "/" + record.baseName;
                double baseRadius = 0.0;
                oapiGetBaseEquPos(base, &record.longitudeRad, &record.latitudeRad, &baseRadius);
                record.bodyRadiusMetres = bodyRadius;
                record.bodyMassKg = bodyMass;
                record.surfaceGravityMps2 = gravitationalConstant * bodyMass /
                    (bodyRadius * bodyRadius);
                record.escapeVelocityMps = sqrt(2.0 * gravitationalConstant * bodyMass / bodyRadius);
                if (!(baseRadius > 0.0) || !_finite(record.longitudeRad) ||
                    !_finite(record.latitudeRad)) continue;
                const oem::Result added = registry.add(record);
                if (!added.ok) {
                    char message[384];
                    sprintf(message, "OEM Spaceport Registry: skipped %s (%s).",
                            record.id.c_str(), added.code.c_str());
                    oapiWriteLog(message);
                    continue;
                }

                const DWORD pads = oapiGetBasePadCount(base);
                for (DWORD padIndex = 0; padIndex < pads; ++padIndex) {
                    oem::SpaceportPadRecord pad;
                    pad.index = (unsigned int)padIndex;
                    if (!oapiGetBasePadEquPos(base, padIndex, &pad.longitudeRad,
                                              &pad.latitudeRad, &pad.radiusMetres))
                        continue;
                    pad.hasIls = oapiGetBasePadNav(base, padIndex) != 0;
                    const oem::Result padAdded = registry.addPad(record.id, pad);
                    if (padAdded.ok) ++padCount;
                }
                ++baseCount;
            }
        }

        spaceportRegistry_ = registry;
        rebuildEconomicLocationRegistry(spaceportRegistry_);

        if (spaceportRegistryPath_.empty()) {
            oapiWriteLog((char*)"OEM Spaceport Registry: could not resolve output path.");
            return;
        }
        oem::SpaceportRegistryConfigStore store;
        const oem::Result saved = store.save(spaceportRegistryPath_, registry);
        if (!saved.ok) {
            char message[512];
            sprintf(message, "OEM Spaceport Registry: save failed (%s): %s",
                    saved.code.c_str(), saved.message.c_str());
            oapiWriteLog(message);
            return;
        }
        char message[256];
        sprintf(message, "OEM Spaceport Registry: saved %lu surface bases across %lu celestial bodies; %lu VTOL pads.",
                baseCount, (unsigned long)bodyCount, padCount);
        oapiWriteLog(message);
    }

    void rebuildEconomicLocationRegistry(const oem::SpaceportRegistry& spaceports) {
        economicLocations_.clear();
        unsigned long surfaces = 0;
        unsigned long stationCount = 0;
        for (std::size_t i = 0; i < spaceports.entries().size(); ++i) {
            const oem::SpaceportRecord& p = spaceports.entries()[i];
            const oem::Result added = economicLocations_.addSpaceport(
                p.bodyName, p.baseName, p.longitudeRad, p.latitudeRad);
            if (added.ok) ++surfaces;
            else {
                char message[384];
                sprintf(message, "OEM Economic Locations: skipped surface:%s (%s).",
                        p.id.c_str(), added.code.c_str());
                oapiWriteLog(message);
            }
        }
        const std::set<std::string>& registeredStationIds = economicStations_.vesselIds();
        for (std::set<std::string>::const_iterator it = registeredStationIds.begin(); it != registeredStationIds.end(); ++it) {
            const oem::Result added = economicLocations_.addStation(*it);
            if (added.ok) ++stationCount;
            else {
                char message[384];
                sprintf(message, "OEM Economic Locations: skipped station:%s (%s).",
                        it->c_str(), added.code.c_str());
                oapiWriteLog(message);
            }
        }
        char summary[256];
        sprintf(summary, "OEM Economic Locations: indexed %lu surface ports and %lu registered stations.",
                surfaces, stationCount);
        oapiWriteLog(summary);
    }

    bool ensureCargoServiceInventory(const std::string& serviceId) {
        if (serviceId.empty()) return false;
        if (cargoServiceInventories_.find(serviceId) != cargoServiceInventories_.end()) return true;
        if (cargoServiceInventoryLoadAttempted_.find(serviceId) != cargoServiceInventoryLoadAttempted_.end()) return false;
        cargoServiceInventoryLoadAttempted_.insert(serviceId);
        const std::string configPath = makeCargoStockConfigPath(module_, serviceId);
        if (configPath.empty()) {
            oapiWriteLog((char*)"OEM Cargo Stock: could not resolve stock configuration path.");
            return false;
        }
        oem::CargoServiceInventory candidate(serviceId);
        const DWORD attributes = GetFileAttributesA(configPath.c_str());
        if (attributes == INVALID_FILE_ATTRIBUTES) {
            const DWORD fileError = GetLastError();
            if (fileError != ERROR_FILE_NOT_FOUND && fileError != ERROR_PATH_NOT_FOUND) {
                oapiWriteLog((char*)"OEM Cargo Stock: could not inspect stock configuration path.");
                return false;
            }
            // Never invent a new opening baseline when the global journal already
            // contains transactions for this location; replay would be ambiguous.
            bool hasHistory = false;
            const oem::Result historyCheck = economyJournalStore_.hasLocationHistory(
                economyJournalPath_, serviceId, hasHistory);
            if (!historyCheck.ok || hasHistory) {
                char error[512];
                sprintf(error, "OEM Cargo Stock: missing opening stock with journal history service=%s (%s).",
                    serviceId.c_str(), historyCheck.ok ? "OPENING_STOCK_HISTORY_MISSING" : historyCheck.code.c_str());
                oapiWriteLog(error);
                return false;
            }
            // First encounter with a location: create a durable, explicitly empty
            // opening inventory. A configured primary-resource profile may seed it once.
            const oem::Result created = cargoStockConfigStore_.save(configPath, candidate);
            if (!created.ok) {
                char message[512];
                sprintf(message, "OEM Cargo Stock: could not create empty opening stock service=%s (%s): %s",
                        serviceId.c_str(), created.code.c_str(), created.message.c_str());
                oapiWriteLog(message);
                return false;
            }
            char message[512];
            sprintf(message, "OEM Cargo Stock: created empty opening inventory service=%s.", serviceId.c_str());
            oapiWriteLog(message);
        } else {
            const oem::Result configured = cargoStockConfigStore_.load(configPath, serviceId, candidate);
            if (!configured.ok) {
                char message[512];
                sprintf(message, "OEM Cargo Stock: load failed service=%s (%s): %s",
                        serviceId.c_str(), configured.code.c_str(), configured.message.c_str());
                oapiWriteLog(message);
                return false;
            }
        }
        cargoServiceInventories_.insert(std::make_pair(serviceId, candidate));
        char message[512];
        sprintf(message, "OEM Cargo Stock: opening state loaded service=%s lines=%lu.",
                serviceId.c_str(), (unsigned long)candidate.lines().size());
        oapiWriteLog(message);
        if (primaryResourcesReady_ && economyReady_ && !seedPrimaryOpeningStock(serviceId)) {
            cargoServiceInventories_.erase(serviceId);
            cargoServiceInventoryLoadAttempted_.erase(serviceId);
            return false;
        }
        return true;
    }

    bool seedPrimaryOpeningStock(const std::string& locationId) {
        std::map<std::string, oem::CargoServiceInventory>::iterator stock =
            cargoServiceInventories_.find(locationId);
        if (stock == cargoServiceInventories_.end()) return false;
        oem::StarterStockProfile profile;
        profile.profileId = "primary-opening:" + locationId;
        const std::vector<oem::PrimaryResourceSource>& sources = primaryResources_.sources();
        for (std::size_t i = 0; i < sources.size(); ++i) {
            const oem::PrimaryResourceSource& source = sources[i];
            if (source.locationId != locationId || source.openingStock <= 0) continue;
            oem::StarterStockLine line;
            line.cargoId = source.cargoId;
            line.quantity = source.openingStock;
            line.unitMassKg = source.unitMassKg;
            profile.lines.push_back(line);
        }
        if (profile.lines.empty()) return true;

        bool hasHistory = false;
        const oem::Result history = economyJournalStore_.hasLocationHistory(
            economyJournalPath_, locationId, hasHistory);
        if (!history.ok) {
            char error[512];
            sprintf(error, "OEM Primary Resources: could not inspect history at %s (%s): %s",
                locationId.c_str(), history.code.c_str(), history.message.c_str());
            oapiWriteLog(error);
            return false;
        }

        // Existing inventory or journal history is authoritative. Consume the
        // one-time opening quantity so it cannot be applied to a later empty state.
        if (!stock->second.lines().empty() || !stock->second.history().empty() || hasHistory) {
            oem::PrimaryResourceBook updated = primaryResources_;
            bool consumed = false;
            const oem::Result cleared = updated.consumeOpeningStock(locationId, consumed);
            if (!cleared.ok) return false;
            if (consumed) {
                const oem::Result saved = primaryResourceConfigStore_.save(
                    primaryResourcesPath_, updated);
                if (!saved.ok) {
                    char warning[512];
                    sprintf(warning, "OEM Primary Resources: could not clear consumed opening stock at %s (%s): %s",
                        locationId.c_str(), saved.code.c_str(), saved.message.c_str());
                    oapiWriteLog(warning);
                }
                primaryResources_ = updated;
            }
            return true;
        }

        oem::CargoServiceInventory candidate(stock->second);
        const oem::Result seeded = oem::seedStarterStock(profile, candidate);
        if (!seeded.ok) {
            char error[512];
            sprintf(error, "OEM Primary Resources: starter stock rejected at %s (%s): %s",
                locationId.c_str(), seeded.code.c_str(), seeded.message.c_str());
            oapiWriteLog(error);
            return false;
        }
        const std::string path = makeCargoStockConfigPath(module_, locationId);
        if (path.empty()) return false;
        const oem::Result savedStock = cargoStockConfigStore_.save(path, candidate);
        if (!savedStock.ok) {
            char error[512];
            sprintf(error, "OEM Primary Resources: could not save starter stock at %s (%s): %s",
                locationId.c_str(), savedStock.code.c_str(), savedStock.message.c_str());
            oapiWriteLog(error);
            return false;
        }

        oem::PrimaryResourceBook updated = primaryResources_;
        bool consumed = false;
        const oem::Result cleared = updated.consumeOpeningStock(locationId, consumed);
        if (!cleared.ok) return false;
        if (consumed) {
            const oem::Result savedConfig = primaryResourceConfigStore_.save(
                primaryResourcesPath_, updated);
            if (!savedConfig.ok) {
                char warning[512];
                sprintf(warning, "OEM Primary Resources: opening stock was applied at %s but its one-time marker could not be saved (%s): %s",
                    locationId.c_str(), savedConfig.code.c_str(), savedConfig.message.c_str());
                oapiWriteLog(warning);
            }
            primaryResources_ = updated;
        }
        stock->second = candidate;
        char message[512];
        sprintf(message, "OEM Primary Resources: seeded opening stock at %s (%lu cargo types).",
            locationId.c_str(), (unsigned long)profile.lines.size());
        oapiWriteLog(message);
        return true;
    }

    void updatePrimaryProduction(oem::SimulationSeconds now) {
        if (!economyReady_ || !primaryResourcesReady_ || primaryResources_.sources().empty()) return;
        bool clockInitialized = false;
        const oem::Result clockResult = primaryResources_.initializeUnsetTimes(now, clockInitialized);
        if (!clockResult.ok) {
            oapiWriteLog((char*)"OEM Primary Resources: invalid simulation clock; production paused.");
            return;
        }
        if (clockInitialized) {
            const oem::Result savedClock = primaryResourceConfigStore_.save(primaryResourcesPath_, primaryResources_);
            if (!savedClock.ok) {
                char error[512];
                sprintf(error, "OEM Primary Resources: could not persist initial simulation clock (%s): %s",
                    savedClock.code.c_str(), savedClock.message.c_str());
                oapiWriteLog(error);
                return;
            }
        }
        std::vector<oem::PrimaryResourceIntent> intents;
        const oem::Result planned = primaryResources_.planDue(now, cargoServiceInventories_, intents);
        if (!planned.ok) {
            char error[512];
            sprintf(error, "OEM Primary Resources: scheduling failed (%s): %s",
                planned.code.c_str(), planned.message.c_str());
            oapiWriteLog(error);
            return;
        }
        for (std::size_t i = 0; i < intents.size(); ++i) {
            const oem::PrimaryResourceIntent& intent = intents[i];
            const oem::Result committed = economyJournalStore_.applyPrimaryProduction(
                economyJournalPath_, intent.operationId, intent.sourceId, intent.locationId,
                intent.cargoId, intent.quantity, intent.unitMassKg, intent.simulationTime,
                cargoServiceInventories_);
            if (!committed.ok) {
                char error[512];
                sprintf(error, "OEM Primary Resources: production commit failed source=%s (%s): %s",
                    intent.sourceId.c_str(), committed.code.c_str(), committed.message.c_str());
                oapiWriteLog(error);
                continue;
            }
            oem::PrimaryResourceBook candidate = primaryResources_;
            const oem::Result marked = candidate.markCommitted(intent.sourceId, intent.simulationTime);
            if (!marked.ok) {
                char error[512];
                sprintf(error, "OEM Primary Resources: scheduler update failed source=%s (%s): %s",
                    intent.sourceId.c_str(), marked.code.c_str(), marked.message.c_str());
                oapiWriteLog(error);
                return;
            }
            primaryResources_ = candidate;
            const oem::Result saved = primaryResourceConfigStore_.save(primaryResourcesPath_, primaryResources_);
            if (!saved.ok) {
                char error[512];
                sprintf(error, "OEM Primary Resources: schedule save failed source=%s (%s): %s",
                    intent.sourceId.c_str(), saved.code.c_str(), saved.message.c_str());
                oapiWriteLog(error);
            }
            char message[512];
            sprintf(message, "OEM PRIMARY PRODUCTION: source=%s location=%s cargo=%s quantity=%lld simt=%lld",
                intent.sourceId.c_str(), intent.locationId.c_str(), intent.cargoId.c_str(),
                intent.quantity, (long long)intent.simulationTime);
            oapiWriteLog(message);
        }
    }

    bool recoverPropellantTransactions(oem::Ledger& recoveredLedger) {
        if (propellantTransactionJournalPath_.empty()) return false;
        std::map<std::string, oem::PropellantTransaction> transactions;
        oem::Result loaded = propellantTransactionJournal_.load(
            propellantTransactionJournalPath_, transactions);
        if (!loaded.ok) {
            char message[512];
            sprintf(message, "OEM Propellant: transaction journal load failed (%s): %s",
                loaded.code.c_str(), loaded.message.c_str());
            oapiWriteLog(message);
            return false;
        }
        for (std::map<std::string, oem::PropellantTransaction>::const_iterator it =
                 transactions.begin(); it != transactions.end(); ++it) {
            const oem::PropellantTransaction& tx = it->second;
            const std::string chargeId = "propellant-charge:" + tx.operationId;
            if (tx.state == oem::PROPELLANT_TX_ABORTED) continue;
            if (tx.state == oem::PROPELLANT_TX_COMMITTED) {
                oem::Result charge = economyJournalStore_.chargePropellant(
                    economyJournalPath_, tx.operationId, "company",
                    tx.quote.totalCostCredits, tx.simulationTime, recoveredLedger);
                if (!charge.ok) return false;
                continue;
            }
            char vesselName[128] = {};
            strncpy(vesselName, tx.vesselId.c_str(), sizeof(vesselName) - 1);
            OBJHANDLE vesselHandle = oapiGetVesselByName(vesselName);
            if (!vesselHandle || !oapiIsVessel(vesselHandle)) return false;
            VESSEL* vessel = oapiGetVesselInterface(vesselHandle);
            if (!vessel) return false;
            std::vector<oem::PropellantResourceState> observed;
            const DWORD count = vessel->GetPropellantCount();
            observed.reserve(count);
            for (DWORD n = 0; n < count; ++n) {
                PROPELLANT_HANDLE resource = vessel->GetPropellantHandleByIndex(n);
                if (!resource) return false;
                const double mass = vessel->GetPropellantMass(resource);
                const double capacity = vessel->GetPropellantMaxMass(resource);
                if (!_finite(mass) || !_finite(capacity) || mass < 0.0 ||
                    capacity < 0.0 || mass > capacity) return false;
                observed.push_back(oem::PropellantResourceState(mass, capacity));
            }
            std::vector<double> rollback;
            const oem::PropellantRecoveryAction action =
                oem::classifyPropellantRecovery(tx, observed, rollback);
            if (action == oem::PROPELLANT_RECOVERY_FINALIZE_CHARGE) {
                oem::Result charge = economyJournalStore_.chargePropellant(
                    economyJournalPath_, tx.operationId, "company",
                    tx.quote.totalCostCredits, tx.simulationTime, recoveredLedger);
                if (!charge.ok) return false;
                oem::Result terminal = propellantTransactionJournal_.setTerminalState(
                    propellantTransactionJournalPath_, tx.operationId, oem::PROPELLANT_TX_COMMITTED);
                if (!terminal.ok) return false;
            } else if (action == oem::PROPELLANT_RECOVERY_ABORT_UNCHANGED) {
                if (recoveredLedger.hasOperation(chargeId)) return false;
                oem::Result terminal = propellantTransactionJournal_.setTerminalState(
                    propellantTransactionJournalPath_, tx.operationId, oem::PROPELLANT_TX_ABORTED);
                if (!terminal.ok) return false;
            } else if (action == oem::PROPELLANT_RECOVERY_ROLLBACK_PARTIAL) {
                if (recoveredLedger.hasOperation(chargeId)) return false;
                for (DWORD n = 0; n < count; ++n) {
                    PROPELLANT_HANDLE resource = vessel->GetPropellantHandleByIndex(n);
                    if (!resource || n >= rollback.size()) return false;
                    vessel->SetPropellantMass(resource, rollback[n]);
                }
                bool restored = true;
                for (DWORD n = 0; n < count; ++n) {
                    PROPELLANT_HANDLE resource = vessel->GetPropellantHandleByIndex(n);
                    if (!resource) { restored = false; break; }
                    const double mass = vessel->GetPropellantMass(resource);
                    const double target = rollback[n];
                    const double tolerance = 1.0e-9 * (fabs(target) > 1.0 ? fabs(target) : 1.0);
                    if (!_finite(mass) || fabs(mass - target) > tolerance) restored = false;
                }
                if (!restored) return false;
                oem::Result terminal = propellantTransactionJournal_.setTerminalState(
                    propellantTransactionJournalPath_, tx.operationId, oem::PROPELLANT_TX_ABORTED);
                if (!terminal.ok) return false;
            } else {
                char message[384];
                sprintf(message, "OEM Propellant: recovery conflict for operation %s; no automatic mutation performed.",
                    tx.operationId.c_str());
                oapiWriteLog(message);
                return false;
            }
        }
        return true;
    }

    bool initializeEconomyState() {
        if (economyJournalPath_.empty() || accountsPath_.empty() || cargoCatalogPath_.empty() || primaryResourcesPath_.empty() || missionFailurePolicyPath_.empty() || missionIssuerRegistryPath_.empty()) {
            oapiWriteLog((char*)"OEM Economy: could not resolve economy configuration paths.");
            return false;
        }

        const oem::Result failurePolicy = missionFailurePolicyStore_.loadOrCreate(
            missionFailurePolicyPath_, missionFailurePolicy_);
        if (!failurePolicy.ok) {
            char message[512];
            sprintf(message, "OEM Economy: mission failure policy failed (%s): %s",
                    failurePolicy.code.c_str(), failurePolicy.message.c_str());
            oapiWriteLog(message);
            return false;
        }

        const oem::Result issuerRegistry = missionIssuerRegistry_.loadOrCreate(missionIssuerRegistryPath_);
        if (!issuerRegistry.ok) {
            char message[512];
            sprintf(message, "OEM Economy: mission issuer registry failed (%s): %s",
                    issuerRegistry.code.c_str(), issuerRegistry.message.c_str());
            oapiWriteLog(message);
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

        if (primaryResourcesPath_.empty()) {
            oapiWriteLog((char*)"OEM Primary Resources: could not resolve primary_resources.cfg.");
            return false;
        }
        DWORD primaryAttributes = GetFileAttributesA(primaryResourcesPath_.c_str());
        if (primaryAttributes == INVALID_FILE_ATTRIBUTES) {
            const DWORD fileError = GetLastError();
            if (fileError != ERROR_FILE_NOT_FOUND && fileError != ERROR_PATH_NOT_FOUND) {
                oapiWriteLog((char*)"OEM Primary Resources: could not inspect primary_resources.cfg.");
                return false;
            }
            primaryResources_ = oem::PrimaryResourceBook();
            const oem::Result created = primaryResourceConfigStore_.save(primaryResourcesPath_, primaryResources_);
            if (!created.ok) {
                char error[512];
                sprintf(error, "OEM Primary Resources: could not create config (%s): %s",
                    created.code.c_str(), created.message.c_str());
                oapiWriteLog(error);
                return false;
            }
            oapiWriteLog((char*)"OEM Primary Resources: created empty primary_resources.cfg; no sources are enabled.");
        } else {
            const oem::Result loaded = primaryResourceConfigStore_.load(primaryResourcesPath_, primaryResources_);
            if (!loaded.ok) {
                char error[512];
                sprintf(error, "OEM Primary Resources: config load failed (%s): %s",
                    loaded.code.c_str(), loaded.message.c_str());
                oapiWriteLog(error);
                return false;
            }
        }
        primaryResourcesReady_ = true;
        char sourceSummary[256];
        sprintf(sourceSummary, "OEM Primary Resources: loaded %lu configured sources.",
            (unsigned long)primaryResources_.sources().size());
        oapiWriteLog(sourceSummary);
        const std::vector<oem::PrimaryResourceSource>& sources = primaryResources_.sources();
        for (std::size_t i = 0; i < sources.size(); ++i) {
            const oem::PrimaryResourceSource& source = sources[i];
            oem::CargoDefinition definition;
            if (!openingCatalog.find(source.cargoId, definition) ||
                fabs(definition.unitMassKg - source.unitMassKg) > 1.0e-9) {
                char error[512];
                sprintf(error, "OEM Primary Resources: source %s has an unknown cargo or unit-mass mismatch.",
                    source.sourceId.c_str());
                oapiWriteLog(error);
                return false;
            }
            const oem::EconomicLocation* stationLocation =
                economicLocations_.find("station:" + source.locationId);
            const oem::EconomicLocation* surfaceLocation = 0;
            std::size_t surfaceMatches = 0;
            const std::vector<oem::EconomicLocation>& knownLocations = economicLocations_.entries();
            for (std::size_t locationIndex = 0; locationIndex < knownLocations.size(); ++locationIndex) {
                if (knownLocations[locationIndex].kind == oem::ECONOMIC_LOCATION_SPACEPORT &&
                    knownLocations[locationIndex].storageId == source.locationId) {
                    surfaceLocation = &knownLocations[locationIndex];
                    ++surfaceMatches;
                }
            }
            if (surfaceMatches > 1 || (stationLocation && surfaceMatches != 0)) {
                char error[512];
                sprintf(error, "OEM Primary Resources: source %s uses ambiguous storage ID %s.",
                    source.sourceId.c_str(), source.locationId.c_str());
                oapiWriteLog(error);
                return false;
            }
            const oem::EconomicLocation* location =
                stationLocation ? stationLocation : (surfaceMatches == 1 ? surfaceLocation : 0);
            if (!location) {
                char error[512];
                sprintf(error, "OEM Primary Resources: source %s references an unknown or ambiguous location %s.",
                    source.sourceId.c_str(), source.locationId.c_str());
                oapiWriteLog(error);
                return false;
            }
            if (!ensureCargoServiceInventory(source.locationId)) return false;
        }
        std::set<std::string> sourceLocations;
        for (std::size_t i = 0; i < sources.size(); ++i)
            sourceLocations.insert(sources[i].locationId);
        // Only empty locations with no journal history can consume configured
        // opening stock. The source quantity is then cleared and persisted.
        for (std::set<std::string>::const_iterator it = sourceLocations.begin();
             it != sourceLocations.end(); ++it)
            if (!seedPrimaryOpeningStock(*it)) return false;

        // Recovery must start from the opening inventory of every registered
        // cargo service. Loading only the currently encountered location makes
        // valid older journal entries fail when their service is not yet docked.
        const std::set<std::string>& registeredServices = cargoServices_.vesselIds();
        for (std::set<std::string>::const_iterator service = registeredServices.begin();
             service != registeredServices.end(); ++service) {
            if (!ensureCargoServiceInventory(*service)) {
                char message[512];
                sprintf(message, "OEM Economy: could not load opening inventory for registered service=%s.",
                        service->c_str());
                oapiWriteLog(message);
                return false;
            }
        }

        std::map<std::string, oem::CargoManifest> openingManifests;
        const oem::Result recovered = economyJournalStore_.load(
            economyJournalPath_, openingCatalog, openingLedger,
            cargoServiceInventories_, openingManifests, &contractBook_, &objectiveBook_);
        if (!recovered.ok) {
            char message[512];
            sprintf(message, "OEM Economy: global journal recovery failed (%s): %s",
                    recovered.code.c_str(), recovered.message.c_str());
            oapiWriteLog(message);
            return false;
        }

        const oem::Result productionRecovery =
            economyJournalStore_.recoverPrimaryProductionTimes(economyJournalPath_, primaryResources_);
        if (!productionRecovery.ok) {
            char error[512];
            sprintf(error, "OEM Primary Resources: journal recovery failed (%s): %s",
                productionRecovery.code.c_str(), productionRecovery.message.c_str());
            oapiWriteLog(error);
            return false;
        }
        const oem::Result productionConfigSaved =
            primaryResourceConfigStore_.save(primaryResourcesPath_, primaryResources_);
        if (!productionConfigSaved.ok) {
            char error[512];
            sprintf(error, "OEM Primary Resources: could not persist recovered schedule (%s): %s",
                productionConfigSaved.code.c_str(), productionConfigSaved.message.c_str());
            oapiWriteLog(error);
            return false;
        }

        propellantRecoveryReady_ = recoverPropellantTransactions(openingLedger);
        if (!propellantRecoveryReady_)
            oapiWriteLog((char*)"OEM Propellant: unresolved transaction recovery; new refuelling operations are disabled.");
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

        // Contract completion is derived from the same verified Orbiter event
        // and the vessel's durable OEM cargo manifest; no MFD button can fake it.
        if (economyReady_) {
            std::map<std::string, oem::CargoManifest>::const_iterator manifest =
                cargoManifests_.find(event.vesselId);
            if (manifest != cargoManifests_.end()) {
                oem::DeliveryContractBook observed = contractBook_;
                observed.observe(event, manifest->second);
                const std::vector<oem::DeliveryContract>& updated = observed.contracts();
                for (std::size_t i = 0; i < updated.size(); ++i) {
                    const oem::DeliveryContract *before = contractBook_.find(updated[i].contractId);
                    if (!before || before->status != oem::CONTRACT_ACCEPTED) continue;
                    oem::Result committed = oem::Result::success();
                    if (updated[i].status == oem::CONTRACT_COMPLETED) {
                        std::map<std::string, oem::CargoManifest>::iterator arrivalManifest =
                            cargoManifests_.find(updated[i].vesselId);
                        if (arrivalManifest == cargoManifests_.end()) {
                            committed = oem::Result::failure("CONTRACT_CARGO_MISSING",
                                "Contract vessel manifest is unavailable at destination.");
                        } else {
                            committed = economyJournalStore_.completeContract(
                                economyJournalPath_, contractBook_, updated[i].contractId,
                                updated[i].completionEventId, updated[i].completedAt,
                                cargoServiceInventories_, arrivalManifest->second);
                            if (committed.ok)
                                committed = settleDeliveryContract(updated[i].contractId);
                        }
                    }
                    else if (updated[i].status == oem::CONTRACT_EXPIRED)
                        committed = economyJournalStore_.expireContract(
                            economyJournalPath_, contractBook_, updated[i].contractId, ledger_, "company", event.simulationTime, missionFailurePolicy_);
                    if (!committed.ok) {
                        char contractError[512];
                        sprintf(contractError, "OEM CONTRACT: journal commit failed id=%s (%s): %s",
                            updated[i].contractId.c_str(), committed.code.c_str(),
                            committed.message.c_str());
                        oapiWriteLog(contractError);
                    }
                }
            }
        }

        if (economyReady_) {
            std::map<std::string, oem::CargoManifest>::const_iterator manifest =
                cargoManifests_.find(event.vesselId);
            if (manifest != cargoManifests_.end()) {
                std::vector<std::string> acceptedIds;
                const std::vector<oem::MissionObjectiveContract>& missions = objectiveBook_.missions();
                for (std::size_t i = 0; i < missions.size(); ++i)
                    if (missions[i].status == oem::MISSION_OBJECTIVE_ACCEPTED &&
                        missions[i].objective.vesselId == event.vesselId)
                        acceptedIds.push_back(missions[i].objective.missionId);
                for (std::size_t i = 0; i < acceptedIds.size(); ++i) {
                    const oem::MissionObjectiveContract* current = objectiveBook_.find(acceptedIds[i]);
                    if (!current || current->status != oem::MISSION_OBJECTIVE_ACCEPTED) continue;
                    const bool objectiveMatch =
                        oem::missionObjectiveMatchesEvent(current->objective, event, manifest->second);
                    if (event.type == oem::SIM_EVENT_LANDING &&
                        current->objective.kind == oem::MISSION_OBJECTIVE_VISIT_SPACEPORT) {
                        char diagnostic[1024];
                        sprintf(diagnostic,
                            "OEM MISSION LANDING CHECK: id=%s vessel=%s/%s targetBody=%s eventBody=%s targetBase=%s eventBase=%s locationKnown=%d eventTime=%lld acceptedAt=%lld deadline=%lld match=%d",
                            current->objective.missionId.c_str(), current->objective.vesselId.c_str(),
                            event.vesselId.c_str(), current->objective.targetBodyId.c_str(),
                            event.bodyId.c_str(), current->objective.targetLocationId.c_str(),
                            event.baseId.c_str(), event.locationKnown ? 1 : 0,
                            (long long)event.simulationTime,
                            (long long)current->objective.acceptedAt,
                            (long long)current->objective.deadline, objectiveMatch ? 1 : 0);
                        oapiWriteLog(diagnostic);
                    }
                    if (objectiveMatch) {
                        oem::MissionObjectiveBook candidate = objectiveBook_;
                        const oem::Result marked = candidate.restoreCompleted(
                            acceptedIds[i], event.eventId, event.simulationTime);
                        if (!marked.ok) continue;
                        const oem::MissionObjectiveContract* completed = candidate.find(acceptedIds[i]);
                        const oem::Result committed = completed
                            ? economyJournalStore_.completeMissionObjective(
                                economyJournalPath_, candidate, acceptedIds[i],
                                completed->completionEventId, completed->completedAt)
                            : oem::Result::failure("MISSION_OBJECTIVE_NOT_FOUND", "Completed objective disappeared.");
                        if (committed.ok) {
                            objectiveBook_ = candidate;
                            char missionMessage[512];
                            sprintf(missionMessage,
                                "OEM MISSION OBJECTIVE COMPLETED: id=%s event=%s vessel=%s",
                                acceptedIds[i].c_str(), event.eventId.c_str(), event.vesselId.c_str());
                            oapiWriteLog(missionMessage);
                        } else {
                            char missionError[512];
                            sprintf(missionError, "OEM MISSION: completion commit failed id=%s (%s): %s",
                                acceptedIds[i].c_str(), committed.code.c_str(), committed.message.c_str());
                            oapiWriteLog(missionError);
                        }
                    } else if (current->objective.deadline > 0 &&
                               event.simulationTime > current->objective.deadline) {
                        const oem::Result expired = economyJournalStore_.expireMissionObjective(
                            economyJournalPath_, objectiveBook_, acceptedIds[i], ledger_, "company", event.simulationTime, missionFailurePolicy_);
                        if (!expired.ok) {
                            char missionError[512];
                            sprintf(missionError, "OEM MISSION: expiry commit failed id=%s (%s): %s",
                                acceptedIds[i].c_str(), expired.code.c_str(), expired.message.c_str());
                            oapiWriteLog(missionError);
                        }
                    }
                }
            }
        }

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
    oem::PrimaryResourceConfigStore primaryResourceConfigStore_;
    oem::PrimaryResourceBook primaryResources_;
    std::string primaryResourcesPath_;
    bool primaryResourcesReady_;
    oem::PropellantTransactionJournal propellantTransactionJournal_;
    std::string propellantTransactionJournalPath_;
    std::string propellantPricePath_;
    bool propellantRecoveryReady_;
    unsigned long propellantOperationSequence_;
    oem::DeliveryContractBook contractBook_;
    oem::MissionObjectiveBook objectiveBook_;
    std::map<std::string, oem::DeliveryContract> pendingContractOffers_;
    unsigned long contractOfferSequence_;
    long long contractOfferDay_;
    oem::MissionBoardStore missionBoardStore_;
    std::map<std::string, oem::MissionBoard> missionBoards_;
    std::string missionBoardPath_;
    bool missionBoardsReady_;
    oem::MissionObjectiveBoardStore objectiveBoardStore_;
    std::map<std::string, oem::MissionObjectiveBoard> objectiveBoards_;
    std::string objectiveBoardPath_;
    bool objectiveBoardsReady_;
    std::string economyJournalPath_;
    oem::MissionFailurePolicyStore missionFailurePolicyStore_;
    oem::MissionFailurePolicy missionFailurePolicy_;
    std::string missionFailurePolicyPath_;
    oem::MissionIssuerRegistry missionIssuerRegistry_;
    std::string missionIssuerRegistryPath_;
    oem::LedgerConfigStore ledgerConfigStore_;
    oem::CargoCatalogConfigStore cargoCatalogConfigStore_;
    std::string accountsPath_;
    std::string cargoCatalogPath_;
    std::string spaceportRegistryPath_;
    oem::EconomicStationRegistry economicStations_;
    std::string economicStationsPath_;
    oem::SpaceportRegistry spaceportRegistry_;
    oem::EconomicLocationRegistry economicLocations_;
    bool economicStationsReady_;
    bool cargoServicesReady_;
    oem::Ledger ledger_;
    oem::CargoCatalog cargoCatalog_;
    bool economyReady_;
    bool journalReady_;
    std::string sessionId_;
    oem::SimulationSeconds currentSimulationTime_;
    unsigned long cargoTransferSequence_;
    OBJHANDLE lastFocus_;
    NOTEHANDLE completionNote_;
    oem::SimulationSeconds notificationUntil_;
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

// Read-only view of Orbiter's real propellant resources. No resource is modified here.
class OEMPropellantMonitorMFD : public MFD {
public:
    OEMPropellantMonitorMFD(DWORD w, DWORD h, VESSEL *vessel)
        : MFD(w, h, vessel),
          vesselId_(vessel && vessel->GetName() ? vessel->GetName() : "") {}

    void Update(HDC hDC) override {
        Title(hDC, "OEM PROPELLANT MONITOR");
        SelectDefaultFont(hDC, 0);
        int y = (int)ch + 3;
        char line[256];
        sprintf(line, "VESSEL: %.36s", vesselId_.c_str());
        DrawLine(hDC, y, line);
        DrawLine(hDC, y, "READ ONLY - ORBITER RESOURCE DATA");
        if (!g_bridge) {
            DrawLine(hDC, y, "OEM bridge unavailable.");
            return;
        }
        std::vector<oem::PropellantResourceState> resources;
        const oem::Result result = g_bridge->readPropellantSnapshot(vesselId_, resources);
        if (!result.ok) {
            sprintf(line, "%.30s: %.150s", result.code.c_str(), result.message.c_str());
            DrawLine(hDC, y, line);
            return;
        }
        double current = 0.0, capacity = 0.0;
        for (std::size_t i = 0; i < resources.size(); ++i) {
            current += resources[i].currentMassKg;
            capacity += resources[i].capacityKg;
        }
        sprintf(line, "RESOURCES: %lu", (unsigned long)resources.size());
        DrawLine(hDC, y, line);
        sprintf(line, "TOTAL MASS: %.3f kg", current);
        DrawLine(hDC, y, line);
        sprintf(line, "TOTAL CAPACITY: %.3f kg", capacity);
        DrawLine(hDC, y, line);
        sprintf(line, "FREE CAPACITY: %.3f kg", capacity - current);
        DrawLine(hDC, y, line);
        DrawLine(hDC, y, "------------------------------");
        if (resources.empty()) {
            DrawLine(hDC, y, "NO PROPELLANT RESOURCES");
            DrawLine(hDC, y, "Totals are zero; nothing to refuel.");
            return;
        }
        const int available = ((int)H - y - (int)ch) / (int)ch;
        for (std::size_t i = 0; i < resources.size() && (int)i < available; ++i) {
            sprintf(line, "TANK %lu: %.3f / %.3f kg",
                (unsigned long)(i + 1),
                resources[i].currentMassKg, resources[i].capacityKg);
            DrawLine(hDC, y, line);
        }
    }

    static int MsgProc(UINT msg, UINT mfd, WPARAM wparam, LPARAM lparam) {
        if (msg == OAPI_MSG_MFD_OPENEDEX && g_bridge) {
            MFDMODEOPENSPEC *spec = (MFDMODEOPENSPEC*)wparam;
            return (int)(new OEMPropellantMonitorMFD(
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

class OEMPropellantRefuelMFD : public MFD {
public:
    OEMPropellantRefuelMFD(DWORD w, DWORD h, VESSEL *vessel)
        : MFD(w, h, vessel),
          vesselId_(vessel && vessel->GetName() ? vessel->GetName() : ""),
          status_("B/F: select purchase   Y: confirm   X: cancel"),
          pendingAction_(0) {}

    void Update(HDC hDC) override {
        Title(hDC, "OEM REFUELLING");
        SelectDefaultFont(hDC, 0);
        int y = (int)ch + 3;
        if (!g_bridge) {
            DrawLine(hDC, y, "OEM bridge unavailable.");
            return;
        }
        oem::PropellantPriceContext context;
        std::string location;
        oem::Result result = g_bridge->resolvePropellantContext(vesselId_, context, location);
        if (!result.ok) {
            DrawLine(hDC, y, "NO ELIGIBLE REFUELLING LOCATION");
            DrawLine(hDC, y, result.message.c_str());
            DrawLine(hDC, y, status_.c_str());
            return;
        }
        char line[256];
        sprintf(line, "LOCATION: %.36s", location.c_str());
        DrawLine(hDC, y, line);
        sprintf(line, "BODY: %.40s", context.celestialBody.c_str());
        DrawLine(hDC, y, line);
        double unitPrice = 0.0;
        result = g_bridge->resolvePropellantUnitPrice(context, unitPrice);
        if (!result.ok) {
            DrawLine(hDC, y, "PRICE CONFIGURATION ERROR");
            DrawLine(hDC, y, result.message.c_str());
            DrawLine(hDC, y, status_.c_str());
            return;
        }
        sprintf(line, "PRICE: %.3f CR / KG", unitPrice);
        DrawLine(hDC, y, line);
        std::vector<oem::PropellantResourceState> resources;
        result = g_bridge->readPropellantSnapshot(vesselId_, resources);
        if (!result.ok) {
            DrawLine(hDC, y, result.message.c_str());
            DrawLine(hDC, y, status_.c_str());
            return;
        }
        oem::PropellantTotals totals;
        result = oem::inspectPropellantResources(resources, totals);
        if (!result.ok) {
            DrawLine(hDC, y, result.message.c_str());
            return;
        }
        sprintf(line, "FUEL: %.3f / %.3f KG", totals.currentMassKg, totals.capacityKg);
        DrawLine(hDC, y, line);
        sprintf(line, "FREE CAPACITY: %.3f KG", totals.freeCapacityKg);
        DrawLine(hDC, y, line);
        oem::Credits balance = 0;
        if (g_bridge->companyBalance(balance)) {
            sprintf(line, "COMPANY BALANCE: %lld CR", (long long)balance);
            DrawLine(hDC, y, line);
        } else {
            DrawLine(hDC, y, "COMPANY ACCOUNT UNAVAILABLE");
        }
        const double partialMass = totals.freeCapacityKg < 100.0 ? totals.freeCapacityKg : 100.0;
        const long double partialCost = ceil((long double)partialMass * (long double)unitPrice);
        const long double fullCost = ceil((long double)totals.freeCapacityKg * (long double)unitPrice);
        if (partialMass > 0.0 && partialCost <= (long double)LLONG_MAX) {
            sprintf(line, "B: %.3f KG  /  %lld CR", partialMass, (long long)partialCost);
            DrawLine(hDC, y, line);
        } else DrawLine(hDC, y, "B: PARTIAL QUOTE OVERFLOW");
        if (totals.freeCapacityKg > 0.0 && fullCost <= (long double)LLONG_MAX) {
            sprintf(line, "F: %.3f KG  /  %lld CR", totals.freeCapacityKg, (long long)fullCost);
            DrawLine(hDC, y, line);
        } else DrawLine(hDC, y, "F: FULL QUOTE OVERFLOW");
        DrawLine(hDC, y, "Y: CONFIRM SELECTED PURCHASE   X: CANCEL");
        DrawLine(hDC, y, status_.c_str());
    }

    bool ConsumeKeyBuffered(DWORD key) override {
        if (!g_bridge) return false;
        if (key == OAPI_KEY_B) {
            pendingAction_ = 1;
            status_ = "Selected: up to 100 kg. Press Y to confirm or X to cancel.";
            return true;
        }
        if (key == OAPI_KEY_F) {
            pendingAction_ = 2;
            status_ = "Selected: fill all tanks. Press Y to confirm or X to cancel.";
            return true;
        }
        if (key == OAPI_KEY_X) {
            pendingAction_ = 0;
            status_ = "Purchase cancelled. Select B or F to prepare another quote.";
            return true;
        }
        if (key == OAPI_KEY_Y) {
            if (pendingAction_ == 0) {
                status_ = "No purchase selected. Press B or F first.";
                return true;
            }
            const bool fillAll = pendingAction_ == 2;
            pendingAction_ = 0;
            const oem::Result result =
                g_bridge->performPropellantPurchase(vesselId_, fillAll);
            status_ = result.ok ? "Purchase and physical fill committed."
                : result.code + ": " + result.message;
            return true;
        }
        return false;
    }

    bool ConsumeButton(int bt, int event) override {
        if (!(event & PANEL_MOUSE_LBDOWN)) return false;
        if (bt == 0) return ConsumeKeyBuffered(OAPI_KEY_B);
        if (bt == 1) return ConsumeKeyBuffered(OAPI_KEY_F);
        return false;
    }

    char *ButtonLabel(int bt) override {
        static char *labels[2] = { (char*)"100 KG", (char*)"FILL" };
        return bt >= 0 && bt < 2 ? labels[bt] : 0;
    }

    int ButtonMenu(const MFDBUTTONMENU **menu) const override {
        static const MFDBUTTONMENU items[2] = {
            { "Buy up to 100 kg of fuel", 0, 'B' },
            { "Fill all available tank capacity", 0, 'F' }
        };
        if (menu) *menu = items;
        return 2;
    }

    static int MsgProc(UINT msg, UINT mfd, WPARAM wparam, LPARAM lparam) {
        if (msg == OAPI_MSG_MFD_OPENEDEX && g_bridge) {
            MFDMODEOPENSPEC *spec = (MFDMODEOPENSPEC*)wparam;
            return (int)(new OEMPropellantRefuelMFD(
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
    int pendingAction_;
};

// Market operations are committed through EconomyJournalStore; the MFD never changes Orbiter physical cargo.
class OEMMarketMFD : public MFD {
public:
    OEMMarketMFD(DWORD w, DWORD h, VESSEL *vessel)
        : MFD(w, h, vessel),
          vesselId_(vessel && vessel->GetName() ? vessel->GetName() : ""),
          status_("Dock to a registered service to trade.") {}

    void Update(HDC hDC) override {
        Title(hDC, "OEM LOCATION MARKET");
        SelectDefaultFont(hDC, 0);
        int y = (int)ch + 3;
        DrawLine(hDC, y, "OEM ACCOUNTING ONLY - NO PHYSICAL CARGO");
        if (!g_bridge) {
            DrawLine(hDC, y, "OEM bridge unavailable.");
            return;
        }
        const std::string serviceId = g_bridge->currentMarketLocation(vesselId_);
        if (serviceId.empty()) {
            DrawLine(hDC, y, "Land at a base or dock to a registered station.");
        } else {
            char line[256];
            sprintf(line, "MARKET: %.40s", serviceId.c_str());
            DrawLine(hDC, y, line);
        }
        oem::Credits balance = 0;
        if (g_bridge->companyBalance(balance)) {
            char line[128];
            sprintf(line, "COMPANY BALANCE: %lld cr", (long long)balance);
            DrawLine(hDC, y, line);
        } else {
            DrawLine(hDC, y, "COMPANY ACCOUNT: missing (accounts.cfg)");
        }
        const oem::CargoCatalog *catalog = g_bridge->marketCatalog();
        const oem::CargoServiceInventory *stock = serviceId.empty()
            ? 0 : g_bridge->cargoServiceInventory(serviceId);
        const oem::CargoManifest *manifest = g_bridge->cargoManifest(vesselId_);
        if (!catalog) {
            DrawLine(hDC, y, "Market configuration is not ready.");
        } else if (catalog->entries().empty()) {
            DrawLine(hDC, y, "Cargo catalog is empty.");
        } else {
            DrawLine(hDC, y, "CARGO / STOCK / SHIP / BUY / SELL");
            const std::vector<oem::CargoDefinition>& items = catalog->entries();
            for (std::size_t i = 0; i < items.size() && i < 4; ++i) {
                const long long stationQty = stock ? stock->quantity(items[i].cargoId) : 0;
                const long long vesselQty = manifest ? manifest->quantity(items[i].cargoId) : 0;
                char line[256];
                sprintf(line, "%.10s %lld/%lld %lld/%lld",
                    items[i].cargoId.c_str(), stationQty, (long long)items[i].buyPrice,
                    vesselQty, (long long)items[i].sellPrice);
                DrawLine(hDC, y, line);
            }
        }
        DrawLine(hDC, y, "B: BUY 1 UNIT     S: SELL 1 UNIT");
        DrawLine(hDC, y, status_.c_str());
    }

    bool ConsumeKeyBuffered(DWORD key) override {
        if (!g_bridge) return false;
        oem::Result result;
        if (key == OAPI_KEY_B) {
            result = g_bridge->performMarketTrade(vesselId_, oem::MARKET_BUY_FROM_STATION);
        } else if (key == OAPI_KEY_S) {
            result = g_bridge->performMarketTrade(vesselId_, oem::MARKET_SELL_TO_STATION);
        } else {
            return false;
        }
        status_ = result.ok ? "Trade durably committed." : result.code + ": " + result.message;
        return true;
    }

    bool ConsumeButton(int bt, int event) override {
        if (!(event & PANEL_MOUSE_LBDOWN)) return false;
        if (bt == 0) return ConsumeKeyBuffered(OAPI_KEY_B);
        if (bt == 1) return ConsumeKeyBuffered(OAPI_KEY_S);
        return false;
    }

    char *ButtonLabel(int bt) override {
        static char *labels[2] = { (char*)"BUY", (char*)"SELL" };
        return bt >= 0 && bt < 2 ? labels[bt] : 0;
    }

    int ButtonMenu(const MFDBUTTONMENU **menu) const override {
        static const MFDBUTTONMENU items[2] = {
            { "Buy one unit at current location", 0, 'B' },
            { "Sell one unit at current location", 0, 'S' }
        };
        if (menu) *menu = items;
        return 2;
    }

    static int MsgProc(UINT msg, UINT mfd, WPARAM wparam, LPARAM lparam) {
        if (msg == OAPI_MSG_MFD_OPENEDEX && g_bridge) {
            MFDMODEOPENSPEC *spec = (MFDMODEOPENSPEC*)wparam;
            return (int)(new OEMMarketMFD(spec->w, spec->h, (VESSEL*)lparam));
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

class OEMMissionBoardMFD : public MFD {
public:
    OEMMissionBoardMFD(DWORD w, DWORD h, VESSEL *vessel)
        : MFD(w,h,vessel), vesselId_(vessel&&vessel->GetName()?vessel->GetName():""),
          selectedOffer_(0) {}

    void Update(HDC hDC) override {
        Title(hDC,"OEM MISSION BOARD");
        SelectDefaultFont(hDC,0);
        int y=(int)ch+3;
        if(!g_bridge){DrawLine(hDC,y,"OEM bridge unavailable.");return;}

        const oem::SimulationSeconds now=g_bridge->missionSimulationTime();
        const oem::DeliveryContract* delivery=activeDelivery();
        const oem::MissionObjectiveContract* objective=activeObjective();
        const bool showDelivery=delivery&&(!objective||delivery->acceptedAt>=objective->objective.acceptedAt);
        if(delivery||objective) {
            DrawLine(hDC,y,"CURRENT MISSION");
            if(showDelivery) {
                DrawWrappedText(hDC,y,delivery->missionTitle.empty()?"Cargo delivery":delivery->missionTitle,35);
                DrawWrappedText(hDC,y,delivery->missionBriefing.empty()?
                    "Deliver the contracted cargo to the destination.":delivery->missionBriefing,35);
                DrawRemaining(hDC,y,delivery->deadline,now);
            } else {
                const oem::MissionObjective& m=objective->objective;
                DrawWrappedText(hDC,y,m.kind==oem::MISSION_OBJECTIVE_VISIT_SPACEPORT?"Spaceport visit":"Station rendezvous",35);
                std::string briefing;
                if(m.kind==oem::MISSION_OBJECTIVE_VISIT_SPACEPORT)
                    briefing="Land at "+m.targetLocationId+" and establish ground contact. The landing must be recorded at this exact base.";
                else
                    briefing="Rendezvous with "+m.targetLocationId+". Completion requires an actual docking transition with this registered station.";
                DrawWrappedText(hDC,y,briefing,35);
                DrawRemaining(hDC,y,m.deadline,now);
            }
            DrawLine(hDC,y,"C: CANCEL CURRENT MISSION");
            return;
        }

        std::vector<oem::DeliveryContract> cargoOffers;
        std::vector<oem::MissionObjectiveOffer> objectiveOffers;
        g_bridge->currentMissionOffers(vesselId_,cargoOffers);
        g_bridge->currentObjectiveOffers(vesselId_,objectiveOffers);
        struct Entry { bool delivery; std::size_t index; std::string title; long long reward; };
        std::vector<Entry> entries;
        for(std::size_t i=0;i<cargoOffers.size()&&entries.size()<6;++i) {
            Entry e; e.delivery=true;e.index=i;e.title=cargoOffers[i].missionTitle.empty()?"Cargo delivery":cargoOffers[i].missionTitle;
            e.reward=cargoOffers[i].reward;entries.push_back(e);
        }
        for(std::size_t i=0;i<objectiveOffers.size()&&entries.size()<6;++i) {
            Entry e;e.delivery=false;e.index=i;e.title=objectiveOffers[i].missionTitle;
            e.reward=objectiveOffers[i].reward;entries.push_back(e);
        }
        if(selectedOffer_>=entries.size()) selectedOffer_=entries.empty()?0:entries.size()-1;
        if(entries.empty()) {
            DrawLine(hDC,y,"NO MISSIONS AVAILABLE HERE");
            DrawLine(hDC,y,"Check back after the next");
            DrawLine(hDC,y,"daily mission board refresh.");
            return;
        }
        DrawLine(hDC,y,"AVAILABLE MISSIONS (MAX 6)");
        for(std::size_t i=0;i<entries.size();++i) {
            char line[128];
            sprintf(line,"%lu %s %lld CR",(unsigned long)(i+1),
                entries[i].delivery?"DELIVERY":"OBJECTIVE",entries[i].reward);
            const COLORREF old=GetTextColor(hDC);
            if(i==selectedOffer_) SetTextColor(hDC,RGB(255,220,80));
            std::string marked=(i==selectedOffer_?"> ":"  ")+entries[i].title;
            DrawWrappedText(hDC,y,marked,35);
            SetTextColor(hDC,old);
            DrawLine(hDC,y,line);
            if(entries[i].delivery) {
                const oem::DeliveryContract& o=cargoOffers[entries[i].index];
                DrawWrappedText(hDC,y,"TO: "+o.destinationLocationId,35);
            } else {
                const oem::MissionObjectiveOffer& o=objectiveOffers[entries[i].index];
                DrawWrappedText(hDC,y,"TO: "+o.targetLocationId,35);
            }
        }
        DrawLine(hDC,y,"-------------------");
        DrawLine(hDC,y,"BRIEFING");
        if(entries[selectedOffer_].delivery)
            DrawWrappedText(hDC,y,cargoOffers[entries[selectedOffer_].index].missionBriefing,35);
        else
            DrawWrappedText(hDC,y,objectiveOffers[entries[selectedOffer_].index].missionBriefing,35);
        DrawLine(hDC,y,"UP/DOWN SELECT  A ACCEPT");
    }

    bool ConsumeKeyBuffered(DWORD key) override {
        if(!g_bridge) return false;
        if(key==OAPI_KEY_UP||key==OAPI_KEY_DOWN) {
            std::vector<oem::DeliveryContract> cargo;
            std::vector<oem::MissionObjectiveOffer> objectives;
            g_bridge->currentMissionOffers(vesselId_,cargo);
            g_bridge->currentObjectiveOffers(vesselId_,objectives);
            const std::size_t available=cargo.size()+objectives.size();
            const std::size_t count=available<6?available:6;
            if(count) {
                if(key==OAPI_KEY_UP&&selectedOffer_>0)--selectedOffer_;
                if(key==OAPI_KEY_DOWN&&selectedOffer_+1<count)++selectedOffer_;
            } else selectedOffer_=0;
            return true;
        }
        if(key==OAPI_KEY_A) {
            std::vector<oem::DeliveryContract> cargo;
            std::vector<oem::MissionObjectiveOffer> objectives;
            g_bridge->currentMissionOffers(vesselId_,cargo);
            g_bridge->currentObjectiveOffers(vesselId_,objectives);
            const std::size_t cargoCount=cargo.size()<6?cargo.size():6;
            const std::size_t available=cargo.size()+objectives.size();
            const std::size_t total=available<6?available:6;
            if(selectedOffer_>=total)return true;
            oem::Result result;
            if(selectedOffer_<cargoCount)
                result=g_bridge->acceptDeliveryOffer(vesselId_,selectedOffer_);
            else
                result=g_bridge->acceptObjectiveOffer(vesselId_,selectedOffer_-cargoCount);
            lastResult_=result.ok?"MISSION ACCEPTED":result.code;
            return true;
        }
        if(key==OAPI_KEY_C) {
            const oem::DeliveryContract* d=activeDelivery();
            const oem::MissionObjectiveContract* o=activeObjective();
            if(d&&(!o||d->acceptedAt>=o->objective.acceptedAt))
                g_bridge->cancelDeliveryContract(d->contractId,vesselId_);
            else if(o) g_bridge->cancelMissionObjective(o->objective.missionId,vesselId_);
            return true;
        }
        return false;
    }

    bool ConsumeButton(int bt,int event) override {
        if(!(event&PANEL_MOUSE_LBDOWN))return false;
        if(bt==0)return ConsumeKeyBuffered(OAPI_KEY_UP);
        if(bt==1)return ConsumeKeyBuffered(OAPI_KEY_DOWN);
        if(bt==2)return ConsumeKeyBuffered(OAPI_KEY_A);
        if(bt==3)return ConsumeKeyBuffered(OAPI_KEY_C);
        return false;
    }
    char* ButtonLabel(int bt) override {
        static char* labels[4]={(char*)"UP",(char*)"DOWN",(char*)"ACCEPT",(char*)"CANCEL"};
        return bt>=0&&bt<4?labels[bt]:0;
    }
    int ButtonMenu(const MFDBUTTONMENU** menu) const override {
        static const MFDBUTTONMENU items[4]={
            {"Select previous mission",0,0},{"Select next mission",0,0},
            {"Accept selected mission",0,'A'},{"Cancel active mission",0,'C'}};
        if(menu)*menu=items;return 4;
    }
    static int MsgProc(UINT msg,UINT mfd,WPARAM wparam,LPARAM lparam) {
        if(msg==OAPI_MSG_MFD_OPENEDEX&&g_bridge) {
            MFDMODEOPENSPEC* spec=(MFDMODEOPENSPEC*)wparam;
            return (int)(new OEMMissionBoardMFD(spec->w,spec->h,(VESSEL*)lparam));
        }
        return 0;
    }
private:
    const oem::DeliveryContract* activeDelivery() const {
        if(!g_bridge)return 0;
        const std::vector<oem::DeliveryContract>& all=g_bridge->deliveryContractBook().contracts();
        for(std::size_t i=all.size();i>0;--i)
            if(all[i-1].vesselId==vesselId_&&all[i-1].status==oem::CONTRACT_ACCEPTED)return &all[i-1];
        return 0;
    }
    const oem::MissionObjectiveContract* activeObjective() const {
        if(!g_bridge)return 0;
        const std::vector<oem::MissionObjectiveContract>& all=g_bridge->missionObjectiveBook().missions();
        for(std::size_t i=all.size();i>0;--i)
            if(all[i-1].objective.vesselId==vesselId_&&all[i-1].status==oem::MISSION_OBJECTIVE_ACCEPTED)return &all[i-1];
        return 0;
    }
    void DrawLine(HDC hDC,int& y,const char* text) {
        std::string line=text?text:""; if(line.size()>35)line.resize(35);
        TextOutA(hDC,4,y,line.c_str(),(int)line.size());y+=(int)ch;
    }
    void DrawWrappedText(HDC hDC,int& y,const std::string& text,std::size_t width) {
        std::size_t pos=0;
        while(pos<text.size()) {
            while(pos<text.size()&&(text[pos]==' '||text[pos]=='\t'||text[pos]=='\r'||text[pos]=='\n'))++pos;
            if(pos>=text.size())break;
            std::string line;
            while(pos<text.size()&&text[pos]!='\r'&&text[pos]!='\n') {
                while(pos<text.size()&&(text[pos]==' '||text[pos]=='\t'))++pos;
                if(pos>=text.size()||text[pos]=='\r'||text[pos]=='\n')break;
                std::size_t begin=pos;
                while(pos<text.size()&&text[pos]!=' '&&text[pos]!='\t'&&text[pos]!='\r'&&text[pos]!='\n')++pos;
                std::string word=text.substr(begin,pos-begin);
                if(!line.empty()&&line.size()+1+word.size()>width){DrawLine(hDC,y,line.c_str());line.clear();}
                while(word.size()>width) {
                    if(!line.empty()){DrawLine(hDC,y,line.c_str());line.clear();}
                    DrawLine(hDC,y,word.substr(0,width).c_str());word.erase(0,width);
                }
                if(!word.empty()){if(!line.empty())line+=" ";line+=word;}
            }
            if(!line.empty())DrawLine(hDC,y,line.c_str());
            while(pos<text.size()&&(text[pos]=='\r'||text[pos]=='\n'))++pos;
        }
    }
    void DrawRemaining(HDC hDC,int& y,oem::SimulationSeconds deadline,oem::SimulationSeconds now) {
        const oem::SimulationSeconds left=deadline>now?deadline-now:0;
        char line[64];
        if(left>=86400)sprintf(line,"TIME LEFT: %lldD %lldH",(long long)(left/86400),(long long)((left%86400)/3600));
        else sprintf(line,"TIME LEFT: %lldH %lldM",(long long)(left/3600),(long long)((left%3600)/60));
        DrawLine(hDC,y,line);
    }
    std::string vesselId_;
    std::string lastResult_;
    std::size_t selectedOffer_;
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


class OEMEconomicAdminMFD : public MFD {
public:
    OEMEconomicAdminMFD(DWORD w, DWORD h, VESSEL *vessel)
        : MFD(w, h, vessel),
          vesselId_(vessel && vessel->GetName() ? vessel->GetName() : ""),
          status_("Changes are saved automatically by OEM.") {}

    void Update(HDC hDC) override {
        Title(hDC, "OEM ECONOMIC ADMIN");
        SelectDefaultFont(hDC, 0);
        int y = (int)ch + 3;
        char line[256];
        sprintf(line, "FOCUSED VESSEL: %.38s", vesselId_.c_str());
        DrawLine(hDC, y, line);
        if (!g_bridge) {
            DrawLine(hDC, y, "OEM bridge unavailable.");
            return;
        }
        DrawLine(hDC, y, g_bridge->isEconomicStation(vesselId_)
            ? "IDENTITY: REGISTERED ORBITAL STATION"
            : "IDENTITY: ORDINARY VESSEL");
        DrawLine(hDC, y, g_bridge->isCargoService(vesselId_)
            ? "SERVICE: CARGO / MARKET ENABLED"
            : "SERVICE: CARGO / MARKET DISABLED");
        DrawLine(hDC, y, "Station identity and cargo services");
        DrawLine(hDC, y, "are independent permissions.");
        DrawLine(hDC, y, "STN+: register as station");
        DrawLine(hDC, y, "STN-: remove station status");
        DrawLine(hDC, y, "SVC+: enable cargo service");
        DrawLine(hDC, y, "SVC-: disable cargo service");
        DrawLine(hDC, y, status_.c_str());
    }

    bool ConsumeKeyBuffered(DWORD key) override {
        if (!g_bridge) return false;
        oem::Result result;
        if (key == OAPI_KEY_R)
            result = g_bridge->setEconomicStationRegistered(vesselId_, true);
        else if (key == OAPI_KEY_X)
            result = g_bridge->setEconomicStationRegistered(vesselId_, false);
        else if (key == OAPI_KEY_T)
            result = g_bridge->setCargoServiceRegistered(vesselId_, true);
        else if (key == OAPI_KEY_D)
            result = g_bridge->setCargoServiceRegistered(vesselId_, false);
        else
            return false;
        status_ = result.ok ? "Saved. Registry updated." : result.code + ": " + result.message;
        return true;
    }

    bool ConsumeButton(int bt, int event) override {
        if (!(event & PANEL_MOUSE_LBDOWN)) return false;
        static const DWORD keys[4] = { OAPI_KEY_R, OAPI_KEY_X, OAPI_KEY_T, OAPI_KEY_D };
        return bt >= 0 && bt < 4 ? ConsumeKeyBuffered(keys[bt]) : false;
    }

    char *ButtonLabel(int bt) override {
        static char *labels[4] = {
            (char*)"STN+", (char*)"STN-", (char*)"SVC+", (char*)"SVC-"
        };
        return bt >= 0 && bt < 4 ? labels[bt] : 0;
    }

    int ButtonMenu(const MFDBUTTONMENU **menu) const override {
        static const MFDBUTTONMENU items[4] = {
            { "Register focused vessel as an orbital station", 0, 'R' },
            { "Remove orbital station identity", 0, 'X' },
            { "Enable cargo and market services", 0, 'T' },
            { "Disable cargo and market services", 0, 'D' }
        };
        if (menu) *menu = items;
        return 4;
    }

    static int MsgProc(UINT msg, UINT mfd, WPARAM wparam, LPARAM lparam) {
        if (msg == OAPI_MSG_MFD_OPENEDEX && g_bridge) {
            MFDMODEOPENSPEC *spec = (MFDMODEOPENSPEC*)wparam;
            return (int)(new OEMEconomicAdminMFD(
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

    spec.name = "OEM Location Market";
    spec.key = OAPI_KEY_M;
    spec.msgproc = OEMMarketMFD::MsgProc;
    g_marketMode = oapiRegisterMFDMode(spec);

    spec.name = "OEM Mission Board";
    spec.key = OAPI_KEY_D;
    spec.msgproc = OEMMissionBoardMFD::MsgProc;
    g_missionBoardMode = oapiRegisterMFDMode(spec);

    spec.name = "OEM Economic Admin";
    spec.key = OAPI_KEY_A;
    spec.msgproc = OEMEconomicAdminMFD::MsgProc;
    g_economicAdminMode = oapiRegisterMFDMode(spec);

    spec.name = "OEM Propellant Monitor";
    spec.key = OAPI_KEY_P;
    spec.msgproc = OEMPropellantMonitorMFD::MsgProc;
    g_propellantMonitorMode = oapiRegisterMFDMode(spec);

    spec.name = "OEM Refuelling";
    spec.key = OAPI_KEY_U;
    spec.msgproc = OEMPropellantRefuelMFD::MsgProc;
    g_propellantRefuelMode = oapiRegisterMFDMode(spec);
}

DLLCLBK void ExitModule(HINSTANCE hDLL) {
    if (g_flightLogMode >= 0) oapiUnregisterMFDMode(g_flightLogMode);
    if (g_cargoManifestMode >= 0) oapiUnregisterMFDMode(g_cargoManifestMode);
    if (g_marketMode >= 0) oapiUnregisterMFDMode(g_marketMode);
    if (g_missionBoardMode >= 0) oapiUnregisterMFDMode(g_missionBoardMode);
    if (g_economicAdminMode >= 0) oapiUnregisterMFDMode(g_economicAdminMode);
    if (g_propellantMonitorMode >= 0) oapiUnregisterMFDMode(g_propellantMonitorMode);
    if (g_propellantRefuelMode >= 0) oapiUnregisterMFDMode(g_propellantRefuelMode);
    g_flightLogMode = -1;
    g_cargoManifestMode = -1;
    g_marketMode = -1;
    g_missionBoardMode = -1;
    g_economicAdminMode = -1;
    g_propellantMonitorMode = -1;
    g_propellantRefuelMode = -1;
    g_bridge = 0;
    // Orbiter owns and destroys registered oapi::Module instances.
}
