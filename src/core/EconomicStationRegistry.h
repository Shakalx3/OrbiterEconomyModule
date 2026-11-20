#ifndef OEM_CORE_ECONOMIC_STATION_REGISTRY_H
#define OEM_CORE_ECONOMIC_STATION_REGISTRY_H

#include "Types.h"
#include <set>
#include <string>

namespace oem {

// Explicit economic identity for orbital stations. Dockable vessels are not
// stations unless they appear in this registry.
class EconomicStationRegistry {
public:
    bool registerStation(const std::string& vesselId);
    bool unregisterStation(const std::string& vesselId);
    bool isStation(const std::string& vesselId) const;
    std::size_t size() const;
    const std::set<std::string>& vesselIds() const;
    void clear();
    Result load(const std::string& path);
    Result save(const std::string& path) const;
private:
    std::set<std::string> vesselIds_;
};

} // namespace oem
#endif
