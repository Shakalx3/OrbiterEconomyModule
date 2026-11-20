#ifndef OEM_CORE_CARGO_CATALOG_CONFIG_H
#define OEM_CORE_CARGO_CATALOG_CONFIG_H
#include "Market.h"
#include <string>
namespace oem {
class CargoCatalogConfigStore {
public:
    Result load(const std::string& path, CargoCatalog& catalog) const;
};
}
#endif
