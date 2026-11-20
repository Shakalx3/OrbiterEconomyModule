#ifndef OEM_CORE_CARGO_MANIFEST_JOURNAL_H
#define OEM_CORE_CARGO_MANIFEST_JOURNAL_H

#include "CargoManifest.h"
#include <string>

namespace oem {

class CargoManifestJournalStore {
public:
    Result load(const std::string& path, CargoManifest& manifest) const;
    Result save(const std::string& path, const CargoManifest& manifest) const;
};

} // namespace oem

#endif
