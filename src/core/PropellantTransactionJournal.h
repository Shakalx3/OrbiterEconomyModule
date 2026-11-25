#ifndef OEM_CORE_PROPELLANT_TRANSACTION_JOURNAL_H
#define OEM_CORE_PROPELLANT_TRANSACTION_JOURNAL_H

#include "PropellantTransaction.h"
#include <map>

namespace oem {

// Dedicated append-only durable intent log. It is deliberately separate from
// the v1 economy journal so old installations remain readable and untouched.
class PropellantTransactionJournal {
public:
    Result load(const std::string& path,
        std::map<std::string, PropellantTransaction>& transactions) const;
    Result prepare(const std::string& path,
        const PropellantTransaction& transaction) const;
    Result setTerminalState(const std::string& path,
        const std::string& operationId, PropellantTransactionState state) const;
};

} // namespace oem
#endif
