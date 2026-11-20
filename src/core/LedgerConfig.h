#ifndef OEM_CORE_LEDGER_CONFIG_H
#define OEM_CORE_LEDGER_CONFIG_H
#include "Ledger.h"
#include <string>
namespace oem {
class LedgerConfigStore {
public:
    // Missing file is an error: opening balances must always be explicit.
    Result load(const std::string& path, Ledger& ledger) const;
    Result save(const std::string& path, const Ledger& ledger) const;
};
}
#endif
