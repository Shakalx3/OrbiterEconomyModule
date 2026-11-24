#include "Ledger.h"

namespace oem {

Ledger::Ledger() : balances_(), operations_(), entries_(), reputations_(),
    reputationOperations_(), reputationEntries_() {}

Result Ledger::createAccount(const std::string& accountId, Credits openingBalance) {
    if (accountId.empty())
        return Result::failure("INVALID_ACCOUNT", "Account ID is empty.");
    if (openingBalance < 0)
        return Result::failure("INVALID_BALANCE", "Opening balance cannot be negative.");
    if (balances_.find(accountId) != balances_.end())
        return Result::failure("ACCOUNT_EXISTS", "Account already exists.");

    balances_[accountId] = openingBalance;
    return Result::success();
}

Result Ledger::applyExternal(const std::string& operationId, const std::string& accountId, Credits delta, SimulationSeconds simulationTime, const std::string& reason) {
    if (operationId.empty() || accountId.empty() || delta == 0) return Result::failure("INVALID_EXTERNAL_ENTRY", "Operation, account and non-zero delta are required.");
    std::map<std::string, std::size_t>::const_iterator prior = operations_.find(operationId);
    if (prior != operations_.end()) {
        const LedgerEntry& old = entries_[prior->second];
        const bool same = (delta > 0 && old.debitAccount == "EXTERNAL" && old.creditAccount == accountId && old.amount == delta) ||
                          (delta < 0 && old.debitAccount == accountId && old.creditAccount == "EXTERNAL" && old.amount == -delta);
        if (same && old.simulationTime == simulationTime) return Result::success();
        return Result::failure("OPERATION_CONFLICT", "Operation ID was already used for a different external entry.");
    }
    std::map<std::string, Credits>::iterator account = balances_.find(accountId);
    if (account == balances_.end()) return Result::failure("UNKNOWN_ACCOUNT", "Account does not exist.");
    const Credits maxValue = static_cast<Credits>(~static_cast<unsigned long long>(0) >> 1);
    if (delta < 0) {
        if (delta == (-maxValue - 1) || account->second < -delta) return Result::failure("INSUFFICIENT_FUNDS", "Account has insufficient funds.");
        account->second += delta;
    } else {
        if (account->second > maxValue - delta) return Result::failure("BALANCE_OVERFLOW", "Account balance would overflow.");
        account->second += delta;
    }
    LedgerEntry entry; entry.operationId=operationId; entry.debitAccount=delta<0?accountId:"EXTERNAL"; entry.creditAccount=delta>0?accountId:"EXTERNAL";
    entry.amount=delta<0?-delta:delta; entry.simulationTime=simulationTime; entry.reason=reason;
    entries_.push_back(entry); operations_[operationId]=entries_.size()-1; return Result::success();
}

Result Ledger::transfer(const std::string& operationId,
                        const std::string& debitAccount,
                        const std::string& creditAccount,
                        Credits amount,
                        SimulationSeconds simulationTime,
                        const std::string& reason) {
    if (operationId.empty())
        return Result::failure("INVALID_OPERATION", "Operation ID is empty.");

    std::map<std::string, std::size_t>::const_iterator prior = operations_.find(operationId);
    if (prior != operations_.end()) {
        const LedgerEntry& oldEntry = entries_[prior->second];
        if (oldEntry.debitAccount == debitAccount &&
            oldEntry.creditAccount == creditAccount &&
            oldEntry.amount == amount &&
            oldEntry.simulationTime == simulationTime)
            return Result::success(); // Exact replay: do not apply twice.
        return Result::failure("OPERATION_CONFLICT",
                               "Operation ID was already used for a different transfer.");
    }

    if (amount <= 0)
        return Result::failure("INVALID_AMOUNT", "Transfer amount must be positive.");
    if (debitAccount == creditAccount)
        return Result::failure("SAME_ACCOUNT", "Debit and credit accounts must differ.");

    std::map<std::string, Credits>::iterator debit = balances_.find(debitAccount);
    std::map<std::string, Credits>::iterator credit = balances_.find(creditAccount);
    if (debit == balances_.end() || credit == balances_.end())
        return Result::failure("UNKNOWN_ACCOUNT", "Debit or credit account does not exist.");
    if (debit->second < amount)
        return Result::failure("INSUFFICIENT_FUNDS", "Debit account has insufficient funds.");

    const Credits maxValue = static_cast<Credits>(~static_cast<unsigned long long>(0) >> 1);
    if (credit->second > maxValue - amount)
        return Result::failure("BALANCE_OVERFLOW", "Credit account balance would overflow.");

    debit->second -= amount;
    credit->second += amount;

    LedgerEntry entry;
    entry.operationId = operationId;
    entry.debitAccount = debitAccount;
    entry.creditAccount = creditAccount;
    entry.amount = amount;
    entry.simulationTime = simulationTime;
    entry.reason = reason;

    entries_.push_back(entry);
    operations_[operationId] = entries_.size() - 1;
    return Result::success();
}

Result Ledger::applyReputation(const std::string& operationId,
                               const std::string& accountId,
                               int delta,
                               SimulationSeconds simulationTime,
                               const std::string& reason) {
    if (operationId.empty() || accountId.empty() || delta == 0 || simulationTime < 0)
        return Result::failure("INVALID_REPUTATION_ENTRY",
                               "Operation, account, non-zero delta and non-negative time are required.");
    std::map<std::string, std::size_t>::const_iterator prior =
        reputationOperations_.find(operationId);
    if (prior != reputationOperations_.end()) {
        const ReputationEntry& old = reputationEntries_[prior->second];
        if (old.accountId == accountId && old.delta == delta &&
            old.simulationTime == simulationTime)
            return Result::success();
        return Result::failure("REPUTATION_OPERATION_CONFLICT",
                               "Reputation operation ID was reused with different data.");
    }
    std::map<std::string, int>::iterator current = reputations_.find(accountId);
    if (current == reputations_.end())
        current = reputations_.insert(std::make_pair(accountId, 50)).first;
    const int next = current->second + delta;
    current->second = next < 0 ? 0 : (next > 100 ? 100 : next);
    ReputationEntry entry;
    entry.operationId = operationId;
    entry.accountId = accountId;
    entry.delta = delta;
    entry.simulationTime = simulationTime;
    entry.reason = reason;
    reputationEntries_.push_back(entry);
    reputationOperations_[operationId] = reputationEntries_.size() - 1;
    return Result::success();
}

bool Ledger::getReputation(const std::string& accountId, int& reputation) const {
    std::map<std::string, int>::const_iterator it = reputations_.find(accountId);
    if (it == reputations_.end()) return false;
    reputation = it->second;
    return true;
}

const std::vector<ReputationEntry>& Ledger::reputationEntries() const {
    return reputationEntries_;
}

bool Ledger::hasOperation(const std::string& operationId) const {
    return operations_.find(operationId) != operations_.end();
}

bool Ledger::getBalance(const std::string& accountId, Credits& balance) const {
    std::map<std::string, Credits>::const_iterator it = balances_.find(accountId);
    if (it == balances_.end())
        return false;
    balance = it->second;
    return true;
}

const std::vector<LedgerEntry>& Ledger::entries() const {
    return entries_;
}

const std::map<std::string, Credits>& Ledger::accounts() const {
    return balances_;
}

} // namespace oem
