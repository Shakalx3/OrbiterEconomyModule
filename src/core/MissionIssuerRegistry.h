#ifndef OEM_CORE_MISSION_ISSUER_REGISTRY_H
#define OEM_CORE_MISSION_ISSUER_REGISTRY_H

#include "Types.h"
#include <map>
#include <string>

namespace oem {

// Maps a canonical economic location to the stable organization that issues
// missions there. Several locations may intentionally share one issuer ID.
// Unmapped locations retain a deterministic per-location fallback identity.
class MissionIssuerRegistry {
public:
    Result loadOrCreate(const std::string& path);
    Result save(const std::string& path) const;
    Result setIssuerForLocation(const std::string& canonicalLocationId,
                                const std::string& issuerId);
    std::string issuerForLocation(const std::string& canonicalLocationId) const;
    const std::map<std::string, std::string>& mappings() const;
private:
    std::map<std::string, std::string> mappings_;
};

} // namespace oem
#endif
