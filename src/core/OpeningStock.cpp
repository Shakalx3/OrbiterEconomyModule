#include "OpeningStock.h"
#include "CargoServiceInventoryConfig.h"
#include <fstream>
#include <map>

namespace oem { namespace {
unsigned long long hashText(unsigned long long h, const std::string& text) {
    for (std::size_t i = 0; i < text.size(); ++i) {
        h ^= (unsigned char)text[i];
        h *= 1099511628211ULL;
    }
    return h;
}
unsigned long long stockHash(const OpeningStockContext& context,
                             const std::string& cargoId) {
    unsigned long long h = 1469598103934665603ULL;
    h = hashText(h, context.bodyName);
    h = hashText(h, "|");
    h = hashText(h, context.locationId);
    h = hashText(h, "|");
    h = hashText(h, cargoId);
    h ^= context.seed;
    h *= 1099511628211ULL;
    h ^= h >> 30;
    h *= 0xbf58476d1ce4e5b9ULL;
    h ^= h >> 27;
    h *= 0x94d049bb133111ebULL;
    h ^= h >> 31;
    return h;
}
}

OpeningStockRule::OpeningStockRule()
    : cargoId(), minimumQuantity(0), maximumQuantity(0) {}
OpeningStockProfile::OpeningStockProfile() : bodyName(), enabled(false), rules() {}
OpeningStockContext::OpeningStockContext()
    : locationId(), bodyName(), seed(0) {}

Result OpeningStockGenerator::generate(const OpeningStockProfile& profile,
                                       const OpeningStockContext& context,
                                       const CargoCatalog& catalog,
                                       CargoServiceInventory& inventory) const {
    if (context.locationId.empty() || context.bodyName.empty() ||
        inventory.serviceVesselId() != context.locationId)
        return Result::failure("OPENING_STOCK_CONTEXT",
            "Opening-stock location and body must be named and match the target inventory.");
    if (!inventory.lines().empty() || !inventory.history().empty())
        return Result::failure("OPENING_STOCK_NOT_EMPTY",
            "Opening stock can only be generated for a completely empty, untouched inventory.");
    if (!profile.enabled) return Result::success();
    if (profile.bodyName.empty() || profile.bodyName != context.bodyName)
        return Result::failure("OPENING_STOCK_PROFILE_BODY",
            "Opening-stock profile must explicitly match the location's celestial body.");

    std::map<std::string, bool> seen;
    CargoServiceInventory candidate(context.locationId);
    for (std::size_t i = 0; i < profile.rules.size(); ++i) {
        const OpeningStockRule& rule = profile.rules[i];
        if (rule.cargoId.empty() || rule.minimumQuantity < 0 ||
            rule.maximumQuantity < rule.minimumQuantity ||
            seen.find(rule.cargoId) != seen.end())
            return Result::failure("OPENING_STOCK_RULE",
                "Opening-stock rules require unique cargo IDs and valid non-negative quantity bounds.");
        seen[rule.cargoId] = true;
        CargoDefinition definition;
        if (!catalog.find(rule.cargoId, definition))
            return Result::failure("OPENING_STOCK_UNKNOWN_CARGO",
                "Opening-stock profile references cargo missing from the catalog.");
        const unsigned long long span =
            (unsigned long long)(rule.maximumQuantity - rule.minimumQuantity) + 1ULL;
        const unsigned long long value = stockHash(context, rule.cargoId) % span;
        const long long quantity = rule.minimumQuantity + (long long)value;
        Result added = candidate.setInitialStock(rule.cargoId, quantity, definition.unitMassKg);
        if (!added.ok) return added;
    }
    inventory = candidate;
    return Result::success();
}

Result OpeningStockGenerator::initialize(const std::string& path,
                                         const OpeningStockProfile* profile,
                                         const OpeningStockContext& context,
                                         const CargoCatalog& catalog,
                                         bool hasPriorTransactions,
                                         CargoServiceInventory& inventory) const {
    if (path.empty() || context.locationId.empty() ||
        inventory.serviceVesselId() != context.locationId)
        return Result::failure("OPENING_STOCK_PATH",
            "A stock path and matching location inventory are required.");

    std::ifstream existing(path.c_str(), std::ios::binary);
    if (existing.good()) {
        CargoServiceInventoryConfigStore store;
        return store.load(path, context.locationId, inventory);
    }
    if (hasPriorTransactions)
        return Result::failure("OPENING_STOCK_HISTORY_MISSING",
            "Opening stock is missing for a location with journal history; refusing to invent a recovery baseline.");

    CargoServiceInventory candidate(context.locationId);
    if (profile) {
        Result generated = generate(*profile, context, catalog, candidate);
        if (!generated.ok) return generated;
    }
    CargoServiceInventoryConfigStore store;
    Result saved = store.save(path, candidate);
    if (!saved.ok) return saved;
    inventory = candidate;
    return Result::success();
}

} // namespace oem
