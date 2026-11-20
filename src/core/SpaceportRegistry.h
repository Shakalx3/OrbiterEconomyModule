#ifndef OEM_CORE_SPACEPORT_REGISTRY_H
#define OEM_CORE_SPACEPORT_REGISTRY_H

#include "Types.h"
#include <string>
#include <vector>

namespace oem {
struct SpaceportPadRecord {
    unsigned int index;
    double longitudeRad;
    double latitudeRad;
    double radiusMetres;
    bool hasIls;
    SpaceportPadRecord();
};
struct SpaceportRecord {
    std::string id; // Stable key: <body name>/<base name>
    std::string bodyName;
    std::string baseName;
    double longitudeRad;
    double latitudeRad;
    double bodyRadiusMetres;
    double bodyMassKg;
    double surfaceGravityMps2;
    double escapeVelocityMps;
    std::vector<SpaceportPadRecord> pads;
    SpaceportRecord();
};
class SpaceportRegistry {
public:
    SpaceportRegistry();
    Result add(const SpaceportRecord& record);
    Result addPad(const std::string& spaceportId,const SpaceportPadRecord& pad);
    void clear();
    const std::vector<SpaceportRecord>& entries() const;
    const SpaceportRecord* find(const std::string& id) const;
private:
    std::vector<SpaceportRecord> entries_;
};
class SpaceportRegistryConfigStore {
public:
    Result load(const std::string& path,SpaceportRegistry& registry) const;
    Result save(const std::string& path,const SpaceportRegistry& registry) const;
};
}
#endif
