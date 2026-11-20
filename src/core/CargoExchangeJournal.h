#ifndef OEM_CORE_CARGO_EXCHANGE_JOURNAL_H
#define OEM_CORE_CARGO_EXCHANGE_JOURNAL_H
#include "CargoManifest.h"
#include "CargoServiceInventory.h"
#include <map>
#include <string>
namespace oem {
class CargoExchangeJournalStore {
public:
    // The supplied inventory must already contain its explicit configured initial stock.
    // Vessel manifests are rebuilt/extended from the same committed exchange stream.
    Result load(const std::string& path, CargoServiceInventory& inventory,
                std::map<std::string, CargoManifest>& manifests) const;
    // One durable journal record is the commit point for both accounting projections.
    Result apply(const std::string& path, CargoServiceInventory& inventory,
                 CargoManifest& manifest, const CargoTransfer& transfer) const;
};
}
#endif
