#include "Ledger.h"

namespace oem {

Ledger::Ledger() : balances_(), operations_(), entries_() {}

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

} // namespace oem
