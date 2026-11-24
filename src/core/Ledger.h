#ifndef OEM_CORE_LEDGER_H
#define OEM_CORE_LEDGER_H

#include "Types.h"
#include <cstddef>
#include <map>
#include <string>
#include <vector>

namespace oem {

struct ReputationEntry {
    std::string operationId;
    std::string accountId;
    int delta;
    SimulationSeconds simulationTime;
    std::string reason;
};

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
    Result applyExternal(const std::string& operationId, const std::string& accountId, Credits delta, SimulationSeconds simulationTime, const std::string& reason);

    Result transfer(const std::string& operationId,
                    const std::string& debitAccount,
                    const std::string& creditAccount,
                    Credits amount,
                    SimulationSeconds simulationTime,
                    const std::string& reason);

    bool hasOperation(const std::string& operationId) const;
    bool getBalance(const std::string& accountId, Credits& balance) const;
    Result applyReputation(const std::string& operationId, const std::string& accountId,
                           int delta, SimulationSeconds simulationTime, const std::string& reason);
    bool getReputation(const std::string& accountId, int& reputation) const;
    const std::vector<ReputationEntry>& reputationEntries() const;
    const std::vector<LedgerEntry>& entries() const;
    const std::map<std::string, Credits>& accounts() const;

private:
    std::map<std::string, Credits> balances_;
    std::map<std::string, std::size_t> operations_;
    std::vector<LedgerEntry> entries_;
    std::map<std::string, int> reputations_;
    std::map<std::string, std::size_t> reputationOperations_;
    std::vector<ReputationEntry> reputationEntries_;
};

} // namespace oem

#endif
