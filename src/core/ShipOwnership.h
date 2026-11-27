#ifndef OEM_CORE_SHIP_OWNERSHIP_H
#define OEM_CORE_SHIP_OWNERSHIP_H

#include "Types.h"
#include <map>
#include <string>
#include <vector>

namespace oem {

enum ShipDeliveryKind {
    SHIP_DELIVERY_ORBITAL = 1,
    SHIP_DELIVERY_SURFACE = 2
};

struct ShipDefinition {
    std::string className;
    std::string displayName;
    Credits price;
    bool orbitalDelivery;
    bool surfaceDelivery;
    ShipDefinition();
};

class ShipCatalog {
public:
    Result add(const ShipDefinition& definition);
    bool find(const std::string& className, ShipDefinition& definition) const;
    const std::vector<ShipDefinition>& entries() const;
    void clear();
private:
    std::vector<ShipDefinition> entries_;
    std::map<std::string, std::size_t> index_;
};

struct ShipPurchaseQuote {
    std::string purchaseId;
    std::string className;
    std::string displayName;
    std::string locationId;
    ShipDeliveryKind deliveryKind;
    Credits price;
    SimulationSeconds simulationTime;
    ShipPurchaseQuote();
};

Result createShipPurchaseQuote(const ShipCatalog& catalog,
                               const std::string& purchaseId,
                               const std::string& className,
                               const std::string& locationId,
                               ShipDeliveryKind deliveryKind,
                               Credits availableBalance,
                               SimulationSeconds simulationTime,
                               ShipPurchaseQuote& quote);

struct OwnedVesselRecord {
    std::string purchaseId;
    std::string vesselId;
    std::string className;
    std::string acquisitionLocationId;
    Credits acquisitionPrice;
    SimulationSeconds acquiredAt;
};

class PlayerFleetRegistry {
public:
    Result registerPurchase(const OwnedVesselRecord& record);
    bool isOwned(const std::string& vesselId) const;
    bool findByVessel(const std::string& vesselId, OwnedVesselRecord& record) const;
    bool findByPurchase(const std::string& purchaseId, OwnedVesselRecord& record) const;
    const std::vector<OwnedVesselRecord>& vessels() const;
    void clear();
    Result load(const std::string& path);
    Result save(const std::string& path) const;
private:
    std::vector<OwnedVesselRecord> vessels_;
    std::map<std::string, std::size_t> vesselIndex_;
    std::map<std::string, std::size_t> purchaseIndex_;
};

} // namespace oem
#endif
