#include "EconomicLocationRegistry.h"
#include <cmath>

namespace oem {

EconomicLocation::EconomicLocation()
    : canonicalId(), storageId(), displayName(), kind(ECONOMIC_LOCATION_SPACEPORT),
      celestialBody(), longitudeRad(0.0), latitudeRad(0.0) {}

EconomicLocationRegistry::EconomicLocationRegistry() : entries_() {}

Result EconomicLocationRegistry::addSpaceport(const std::string& body,
                                               const std::string& base,
                                               double longitude, double latitude) {
    if (body.empty() || base.empty() || !std::isfinite(longitude) ||
        !std::isfinite(latitude) || latitude < -1.5707963267948966 ||
        latitude > 1.5707963267948966)
        return Result::failure("LOCATION_SPACEPORT_INVALID",
            "A spaceport requires body/base names and finite geographic coordinates.");
    EconomicLocation location;
    location.canonicalId = "surface:" + body + "/" + base;
    location.storageId = base; // Preserve existing cargo stock and journal identifiers.
    location.displayName = base;
    location.kind = ECONOMIC_LOCATION_SPACEPORT;
    location.celestialBody = body;
    location.longitudeRad = longitude;
    location.latitudeRad = latitude;
    if (find(location.canonicalId))
        return Result::failure("LOCATION_DUPLICATE", "Economic location already exists.");
    entries_.push_back(location);
    return Result::success();
}

Result EconomicLocationRegistry::addStation(const std::string& vesselId) {
    if (vesselId.empty())
        return Result::failure("LOCATION_STATION_INVALID", "Station vessel ID is required.");
    EconomicLocation location;
    location.canonicalId = "station:" + vesselId;
    location.storageId = vesselId; // Preserve existing registered-service inventory keys.
    location.displayName = vesselId;
    location.kind = ECONOMIC_LOCATION_STATION;
    if (find(location.canonicalId))
        return Result::failure("LOCATION_DUPLICATE", "Economic location already exists.");
    entries_.push_back(location);
    return Result::success();
}

const EconomicLocation* EconomicLocationRegistry::find(const std::string& id) const {
    for (std::size_t i = 0; i < entries_.size(); ++i)
        if (entries_[i].canonicalId == id) return &entries_[i];
    return 0;
}

const EconomicLocation* EconomicLocationRegistry::findStorageId(
    const std::string& storageId, EconomicLocationKind kind) const {
    for (std::size_t i = 0; i < entries_.size(); ++i)
        if (entries_[i].storageId == storageId && entries_[i].kind == kind)
            return &entries_[i];
    return 0;
}

const std::vector<EconomicLocation>& EconomicLocationRegistry::entries() const {
    return entries_;
}

void EconomicLocationRegistry::clear() {
    entries_.clear();
}

} // namespace oem
