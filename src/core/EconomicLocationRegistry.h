#ifndef OEM_CORE_ECONOMIC_LOCATION_REGISTRY_H
#define OEM_CORE_ECONOMIC_LOCATION_REGISTRY_H

#include "Types.h"
#include <string>
#include <vector>

namespace oem {

enum EconomicLocationKind {
    ECONOMIC_LOCATION_SPACEPORT = 1,
    ECONOMIC_LOCATION_STATION = 2
};

// canonicalId is namespaced ("surface:Earth/Canaveral", "station:ISS").
// storageId deliberately preserves the pre-existing stock/journal key.
struct EconomicLocation {
    std::string canonicalId;
    std::string storageId;
    std::string displayName;
    EconomicLocationKind kind;
    std::string celestialBody;
    double longitudeRad;
    double latitudeRad;
    EconomicLocation();
};

class EconomicLocationRegistry {
public:
    EconomicLocationRegistry();
    Result addSpaceport(const std::string& bodyName, const std::string& baseName,
                        double longitudeRad, double latitudeRad);
    Result addStation(const std::string& vesselId);
    const EconomicLocation* find(const std::string& canonicalId) const;
    const EconomicLocation* findStorageId(const std::string& storageId,
                                          EconomicLocationKind kind) const;
    const std::vector<EconomicLocation>& entries() const;
    void clear();
private:
    std::vector<EconomicLocation> entries_;
};

} // namespace oem
#endif
