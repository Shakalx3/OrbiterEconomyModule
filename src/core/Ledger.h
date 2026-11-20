#ifndef OEM_CORE_LEDGER_H
#define OEM_CORE_LEDGER_H

#include "Types.h"
#include <cstddef>
#include <map>
#include <string>
#include <vector>

namespace oem {

struct LedgerEntry {
    std::string operationId;
    std::string debitAccount;
    std::string creditAccount;
    Credits amount;
    SimulationSeconds simulationTime;
    std::string reason;
};

class Ledger {
public:
    Ledger();

    Result createAccount(const std::string& accountId, Credits openingBalance);
    Result transfer(const std::string& operationId,
                    const std::string& debitAccount,
                    const std::string& creditAccount,
                    Credits amount,
                    SimulationSeconds simulationTime,
                    const std::string& reason);

    bool hasOperation(const std::string& operationId) const;
    bool getBalance(const std::string& accountId, Credits& balance) const;
    const std::vector<LedgerEntry>& entries() const;
    const std::map<std::string, Credits>& accounts() const;

private:
    std::map<std::string, Credits> balances_;
    std::map<std::string, std::size_t> operations_;
    std::vector<LedgerEntry> entries_;
};

} // namespace oem

#endif
