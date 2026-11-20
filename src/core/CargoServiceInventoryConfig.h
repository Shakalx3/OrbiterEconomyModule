#ifndef OEM_CORE_CARGO_SERVICE_INVENTORY_CONFIG_H
#define OEM_CORE_CARGO_SERVICE_INVENTORY_CONFIG_H
#include "CargoServiceInventory.h"
#include <string>
namespace oem {
class CargoServiceInventoryConfigStore {
public:
    // Missing file means explicitly empty stock; it never creates stock implicitly.
    Result load(const std::string& path,const std::string& serviceVesselId,
                CargoServiceInventory& inventory) const;
    Result save(const std::string& path,const CargoServiceInventory& inventory) const;
};
}
#endif
