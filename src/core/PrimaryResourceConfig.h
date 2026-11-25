#ifndef OEM_CORE_PRIMARY_RESOURCE_CONFIG_H
#define OEM_CORE_PRIMARY_RESOURCE_CONFIG_H
#include "PrimaryResource.h"
#include <string>
namespace oem {
class PrimaryResourceConfigStore {
public:
    Result load(const std::string& path, PrimaryResourceBook& book) const;
    Result save(const std::string& path, const PrimaryResourceBook& book) const;
};
}
#endif
