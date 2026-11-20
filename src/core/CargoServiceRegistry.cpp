#include "CargoServiceRegistry.h"

namespace oem {

bool CargoServiceRegistry::registerService(const std::string& vesselId) {
    if (vesselId.empty()) return false;
    return vesselIds_.insert(vesselId).second;
}

bool CargoServiceRegistry::unregisterService(const std::string& vesselId) {
    return vesselIds_.erase(vesselId) != 0;
}

bool CargoServiceRegistry::isRegistered(const std::string& vesselId) const {
    return !vesselId.empty() && vesselIds_.find(vesselId) != vesselIds_.end();
}

std::size_t CargoServiceRegistry::size() const {
    return vesselIds_.size();
}

} // namespace oem
