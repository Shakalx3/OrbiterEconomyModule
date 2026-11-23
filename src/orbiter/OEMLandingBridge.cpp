#define STRICT
#define ORBITER_MODULE
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <map>
#include <set>
#include <vector>
#include <string>
#include <math.h>
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
#include "../core/LedgerConfig.h"
#include "../core/CargoCatalogConfig.h"
#include "../core/EconomicLocationRegistry.h"
#include "../core/EconomicStationRegistry.h"
#include "../core/PropellantPlan.h"
#include "../core/PropellantQuote.h"
#include "../core/PropellantTransaction.h"
#include "../core/PropellantTransactionJournal.h"

namespace {


class OEMLandingBridge;
OEMLandingBridge *g_bridge = 0;
int g_flightLogMode = -1;
int g_cargoManifestMode = -1;
int g_cargoExchangeTestMode = -1;
int g_marketMode = -1;
int g_deliveryContractsMode = -1;
int g_economicAdminMode = -1;
int g_propellantMonitorMode = -1;

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

class OEMLandingBridge : public oapi::Module {
public:
    explicit OEMLandingBridge(HINSTANCE hDLL)
        : oapi::Module(hDLL), module_(hDLL), landingMonitors_(), flightMonitors_(),
          dockingMonitors_(), eventQueue_(), journalStore_(), journalPath_(makeJournalPath(hDLL)),
          cargoServices_(), cargoServicesPath_(makeCargoServicesPath(hDLL)),
          cargoTransferGate_(), cargoManifests_(), cargoManifestLoadAttempted_(),
          cargoStockConfigStore_(), cargoServiceInventories_(), cargoServiceInventoryLoadAttempted_(),
          economyJournalStore_(), contractBook_(), pendingContractOffers_(), contractOfferSequence_(0), economyJournalPath_(makeEconomyDataFilePath(hDLL, "economy.journal")),
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

    const oem::CargoCatalog* marketCatalog() const {
        return economyReady_ ? &cargoCatalog_ : 0;
    }

    const oem::DeliveryContractBook& deliveryContractBook() const {
        return contractBook_;
    }

    bool currentDeliveryOffer(const std::string& vesselId, oem::DeliveryContract& offer) {
        if (!economyReady_ || sessionId_.empty() || currentSimulationTime_ <= 0 ||
            vesselId.empty() || !isFocusedVessel(vesselId)) return false;
        const std::vector<oem::DeliveryContract>& accepted =
            contractBook_.contracts();
        for (std::size_t i = 0; i < accepted.size(); ++i) {
            if (accepted[i].vesselId == vesselId &&
                accepted[i].status != oem::CONTRACT_SETTLED &&
                accepted[i].status != oem::CONTRACT_EXPIRED)
                return false;
        }
        std::map<std::string, oem::DeliveryContract>::iterator pending =
            pendingContractOffers_.find(vesselId);
        const std::string origin = currentMarketLocation(vesselId);
        if (origin.empty()) return false;
        if (pending != pendingContractOffers_.end()) {
            if (pending->second.originLocationId != origin) return false;
            offer = pending->second;
            return true;
        }
        std::map<std::string, oem::CargoServiceInventory>::const_iterator stock =
            cargoServiceInventories_.find(origin);
        if (stock == cargoServiceInventories_.end()) return false;
        std::string destination;
        const std::set<std::string>& services = cargoServices_.vesselIds();
        for (std::set<std::string>::const_iterator it = services.begin(); it != services.end(); ++it) {
            if (*it != origin) { destination = *it; break; }
        }
        if (destination.empty()) return false;
        oem::Credits availableFunds = 0;
        if (!ledger_.getBalance("company", availableFunds)) return false;
        const std::vector<oem::CargoDefinition>& items = cargoCatalog_.entries();
        const oem::CargoDefinition *selected = 0;
        for (std::size_t i = 0; i < items.size(); ++i) {
            if (stock->second.quantity(items[i].cargoId) > 0 &&
                items[i].buyPrice > 0 && items[i].buyPrice <= availableFunds) {
                selected = &items[i];
                break;
            }
        }
        if (!selected) return false;
        oem::DeliveryContract generated;
        char id[256];
        sprintf(id, "OEMDELIVERY-%s-%lu", sessionId_.c_str(), ++contractOfferSequence_);
        generated.contractId = id;
        generated.vesselId = vesselId;
        generated.cargoId = selected->cargoId;
        generated.quantity = 1;
        generated.originLocationId = origin;
        generated.destinationLocationId = destination;
        generated.reward = selected->buyPrice > 0 ? selected->buyPrice : 1;
        generated.acceptedAt = currentSimulationTime_;
        generated.deadline = currentSimulationTime_ + 7 * 86400;
        pendingContractOffers_[vesselId] = generated;
        offer = generated;
        return true;
    }

    oem::Result acceptDeliveryOffer(const std::string& vesselId) {
        if (!isFocusedVessel(vesselId))
            return oem::Result::failure("CONTRACT_FOCUS", "Accept the offer from its assigned vessel.");
        std::map<std::string, oem::DeliveryContract>::const_iterator it =
            pendingContractOffers_.find(vesselId);
        if (it == pendingContractOffers_.end() ||
            currentMarketLocation(vesselId) != it->second.originLocationId)
            return oem::Result::failure("CONTRACT_OFFER_UNAVAILABLE", "No delivery offer is available at this location.");
        const oem::Result saved = economyJournalStore_.acceptContract(
            economyJournalPath_, contractBook_, it->second);
        if (saved.ok) pendingContractOffers_.erase(it);
        return saved;
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
        pendingContractOffers_.clear();
        contractOfferSequence_ = 0;
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
        if (servicesLoaded.ok) {
            const std::set<std::string>& serviceIds = cargoServices_.vesselIds();
            for (std::set<std::string>::const_iterator it = serviceIds.begin(); it != serviceIds.end(); ++it)
                ensureCargoServiceInventory(*it);
            // Restore explicit opening stock for previously visited surface bases
            // before replaying economy.journal, just as for registered stations.
            const std::vector<oem::SimulationEvent>& priorEvents = eventQueue_.events();
            for (std::size_t i = 0; i < priorEvents.size(); ++i)
                if (priorEvents[i].type == oem::SIM_EVENT_LANDING &&
                    priorEvents[i].locationKnown && !priorEvents[i].baseId.empty())
                    ensureCargoServiceInventory(priorEvents[i].baseId);
            initializeEconomyState();
        }
        rebuildSpaceportRegistry();
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
            settleCompletedContracts();
            oem::DeliveryContractBook due = contractBook_;
            const std::vector<std::string> expired = due.expireDue(simSeconds);
            for (std::size_t i = 0; i < expired.size(); ++i) {
                const oem::Result committed = economyJournalStore_.expireContract(
                    economyJournalPath_, contractBook_, expired[i]);
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
            // First encounter with a location: create a durable, explicitly empty
            // opening inventory. Never synthesize cargo or overwrite an existing file.
            const oem::Result created = cargoStockConfigStore_.save(configPath, candidate);
            if (!created.ok) {
                char message[512];
                sprintf(message, "OEM Cargo Stock: could not create empty opening stock service=%s (%s): %s",
                        serviceId.c_str(), created.code.c_str(), created.message.c_str());
                oapiWriteLog(message);
                return false;
            }
            char message[512];
            sprintf(message, "OEM Cargo Stock: created empty opening inventory service=%s.",
                    serviceId.c_str());
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
        return true;
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
            cargoServiceInventories_, openingManifests, &contractBook_);
        if (!recovered.ok) {
            char message[512];
            sprintf(message, "OEM Economy: global journal recovery failed (%s): %s",
                    recovered.code.c_str(), recovered.message.c_str());
            oapiWriteLog(message);
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
                            economyJournalPath_, contractBook_, updated[i].contractId);
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
    oem::PropellantTransactionJournal propellantTransactionJournal_;
    std::string propellantTransactionJournalPath_;
    std::string propellantPricePath_;
    bool propellantRecoveryReady_;
    unsigned long propellantOperationSequence_;
    oem::DeliveryContractBook contractBook_;
    std::map<std::string, oem::DeliveryContract> pendingContractOffers_;
    unsigned long contractOfferSequence_;
    std::string economyJournalPath_;
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

class OEMDeliveryContractsMFD : public MFD {
public:
    OEMDeliveryContractsMFD(DWORD w, DWORD h, VESSEL *vessel)
        : MFD(w, h, vessel),
          vesselId_(vessel && vessel->GetName() ? vessel->GetName() : ""),
          status_("Offers require two registered cargo services.") {}

    void Update(HDC hDC) override {
        Title(hDC, "OEM DELIVERY CONTRACTS");
        SelectDefaultFont(hDC, 0);
        int y = (int)ch + 3;
        if (!g_bridge) {
            DrawLine(hDC, y, "OEM bridge unavailable.");
            return;
        }
        oem::DeliveryContract offer;
        if (g_bridge->currentDeliveryOffer(vesselId_, offer)) {
            char line[256];
            DrawLine(hDC, y, "AVAILABLE CARGO DELIVERY");
            sprintf(line, "CARGO: %.28s x %lld", offer.cargoId.c_str(), offer.quantity);
            DrawLine(hDC, y, line);
            sprintf(line, "FROM: %.34s", offer.originLocationId.c_str());
            DrawLine(hDC, y, line);
            sprintf(line, "TO:   %.34s", offer.destinationLocationId.c_str());
            DrawLine(hDC, y, line);
            sprintf(line, "REWARD: %lld cr  DEADLINE: 7 days", (long long)offer.reward);
            DrawLine(hDC, y, line);
            DrawLine(hDC, y, "A: ACCEPT  |  Then BUY cargo at origin");
        } else {
            DrawLine(hDC, y, "No offer at this location.");
            DrawLine(hDC, y, "Need stock here and another registered");
            DrawLine(hDC, y, "cargo service as destination.");
        }
        DrawLine(hDC, y, "------------------------------");
        DrawLine(hDC, y, "YOUR CONTRACTS");
        const std::vector<oem::DeliveryContract>& contracts =
            g_bridge->deliveryContractBook().contracts();
        int shown = 0;
        for (std::size_t n = contracts.size(); n > 0 && shown < 5; --n) {
            const oem::DeliveryContract& c = contracts[n - 1];
            if (c.vesselId != vesselId_) continue;
            const char *state = c.status == oem::CONTRACT_ACCEPTED ? "ACTIVE" :
                c.status == oem::CONTRACT_COMPLETED ? "PAYMENT PENDING" :
                c.status == oem::CONTRACT_SETTLED ? "PAID" : "EXPIRED";
            char line[256];
            sprintf(line, "%.14s %s", c.contractId.c_str(), state);
            DrawLine(hDC, y, line);
            sprintf(line, "  %.12s -> %.12s  x%lld",
                c.originLocationId.c_str(), c.destinationLocationId.c_str(), c.quantity);
            DrawLine(hDC, y, line);
            ++shown;
        }
        if (shown == 0) DrawLine(hDC, y, "No accepted contracts.");
        DrawLine(hDC, y, "PAYOUT IS AUTOMATIC ON ARRIVAL");
        DrawLine(hDC, y, status_.c_str());
    }

    bool ConsumeKeyBuffered(DWORD key) override {
        if (!g_bridge) return false;
        oem::Result result;
        if (key == OAPI_KEY_A) {
            result = g_bridge->acceptDeliveryOffer(vesselId_);
        } else if (key == OAPI_KEY_S) {
            result = oem::Result::failure("CONTRACT_NOT_COMPLETE", "No completed contract is waiting for payment.");
            const std::vector<oem::DeliveryContract>& contracts =
                g_bridge->deliveryContractBook().contracts();
            for (std::size_t i = 0; i < contracts.size(); ++i) {
                if (contracts[i].vesselId == vesselId_ &&
                    contracts[i].status == oem::CONTRACT_COMPLETED) {
                    result = g_bridge->settleDeliveryContract(contracts[i].contractId);
                    break;
                }
            }
        } else {
            return false;
        }
        status_ = result.ok ? "Contract journal updated." : result.code + ": " + result.message;
        return true;
    }

    bool ConsumeButton(int bt, int event) override {
        if (!(event & PANEL_MOUSE_LBDOWN)) return false;
        if (bt == 0) return ConsumeKeyBuffered(OAPI_KEY_A);
        if (bt == 1) return ConsumeKeyBuffered(OAPI_KEY_S);
        return false;
    }

    char *ButtonLabel(int bt) override {
        static char *labels[2] = { (char*)"ACCEPT", (char*)"RETRY" };
        return bt >= 0 && bt < 2 ? labels[bt] : 0;
    }

    int ButtonMenu(const MFDBUTTONMENU **menu) const override {
        static const MFDBUTTONMENU items[2] = {
            { "Accept the current delivery offer", 0, 'A' },
            { "Retry a pending automatic payout", 0, 'S' }
        };
        if (menu) *menu = items;
        return 2;
    }

    static int MsgProc(UINT msg, UINT mfd, WPARAM wparam, LPARAM lparam) {
        if (msg == OAPI_MSG_MFD_OPENEDEX && g_bridge) {
            MFDMODEOPENSPEC *spec = (MFDMODEOPENSPEC*)wparam;
            return (int)(new OEMDeliveryContractsMFD(
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

    spec.name = "OEM Cargo Exchange Test";
    spec.key = OAPI_KEY_E;
    spec.msgproc = OEMCargoExchangeTestMFD::MsgProc;
    g_cargoExchangeTestMode = oapiRegisterMFDMode(spec);

    spec.name = "OEM Location Market";
    spec.key = OAPI_KEY_M;
    spec.msgproc = OEMMarketMFD::MsgProc;
    g_marketMode = oapiRegisterMFDMode(spec);

    spec.name = "OEM Delivery Contracts";
    spec.key = OAPI_KEY_D;
    spec.msgproc = OEMDeliveryContractsMFD::MsgProc;
    g_deliveryContractsMode = oapiRegisterMFDMode(spec);

    spec.name = "OEM Economic Admin";
    spec.key = OAPI_KEY_A;
    spec.msgproc = OEMEconomicAdminMFD::MsgProc;
    g_economicAdminMode = oapiRegisterMFDMode(spec);

    spec.name = "OEM Propellant Monitor";
    spec.key = OAPI_KEY_P;
    spec.msgproc = OEMPropellantMonitorMFD::MsgProc;
    g_propellantMonitorMode = oapiRegisterMFDMode(spec);
}

DLLCLBK void ExitModule(HINSTANCE hDLL) {
    if (g_flightLogMode >= 0) oapiUnregisterMFDMode(g_flightLogMode);
    if (g_cargoManifestMode >= 0) oapiUnregisterMFDMode(g_cargoManifestMode);
    if (g_cargoExchangeTestMode >= 0) oapiUnregisterMFDMode(g_cargoExchangeTestMode);
    if (g_marketMode >= 0) oapiUnregisterMFDMode(g_marketMode);
    if (g_deliveryContractsMode >= 0) oapiUnregisterMFDMode(g_deliveryContractsMode);
    if (g_economicAdminMode >= 0) oapiUnregisterMFDMode(g_economicAdminMode);
    if (g_propellantMonitorMode >= 0) oapiUnregisterMFDMode(g_propellantMonitorMode);
    g_flightLogMode = -1;
    g_cargoManifestMode = -1;
    g_cargoExchangeTestMode = -1;
    g_marketMode = -1;
    g_deliveryContractsMode = -1;
    g_economicAdminMode = -1;
    g_propellantMonitorMode = -1;
    g_bridge = 0;
    // Orbiter owns and destroys registered oapi::Module instances.
}
