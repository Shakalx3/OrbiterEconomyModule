#ifndef OEM_CORE_OPENING_STOCK_H
#define OEM_CORE_OPENING_STOCK_H

#include "Market.h"
#include <string>
#include <vector>

namespace oem {

struct OpeningStockRule {
    std::string cargoId;
    long long minimumQuantity;
    long long maximumQuantity;
    OpeningStockRule();
};

struct OpeningStockProfile {
    std::string bodyName;
    bool enabled;
    std::vector<OpeningStockRule> rules;
    OpeningStockProfile();
};

struct OpeningStockContext {
    std::string locationId;
    std::string bodyName;
    unsigned long long seed;
    OpeningStockContext();
};

// Generates deterministic opening inventory only. It never creates physical
// vessel cargo and never changes an inventory that already has stock/history.
class OpeningStockGenerator {
public:
    Result generate(const OpeningStockProfile& profile,
                    const OpeningStockContext& context,
                    const CargoCatalog& catalog,
                    CargoServiceInventory& inventory) const;

    // Existing files are authoritative. Missing opening stock may only be
    // generated when the caller confirms that no journal history exists.
    Result initialize(const std::string& path,
                      const OpeningStockProfile* profile,
                      const OpeningStockContext& context,
                      const CargoCatalog& catalog,
                      bool hasPriorTransactions,
                      CargoServiceInventory& inventory) const;
};

} // namespace oem
#endif
