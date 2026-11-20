#ifndef OEM_CORE_CARGO_SERVICE_REGISTRY_H
#define OEM_CORE_CARGO_SERVICE_REGISTRY_H

#include "Types.h"
#include <cstddef>
#include <set>
#include <string>

namespace oem {

class CargoServiceRegistry {
public:
    bool registerService(const std::string& vesselId);
    bool unregisterService(const std::string& vesselId);
    bool isRegistered(const std::string& vesselId) const;
    std::size_t size() const;
    Result load(const std::string& path);
    Result save(const std::string& path) const;

private:
    std::set<std::string> vesselIds_;
};

} // namespace oem

#endif
