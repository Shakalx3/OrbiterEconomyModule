#ifndef OEM_CORE_PROPELLANT_TRANSACTION_H
#define OEM_CORE_PROPELLANT_TRANSACTION_H

#include "PropellantQuote.h"
#include <vector>

namespace oem {

enum PropellantTransactionState {
    PROPELLANT_TX_PREPARED = 1,
    PROPELLANT_TX_COMMITTED = 2,
    PROPELLANT_TX_ABORTED = 3
};

struct PropellantResourceChange {
    std::size_t resourceIndex;
    double capacityKg;
    double beforeMassKg;
    double afterMassKg;
    PropellantResourceChange();
};

struct PropellantTransaction {
    std::string operationId;
    std::string vesselId;
    PropellantQuote quote;
    PropellantTransactionState state;
    std::vector<PropellantResourceChange> resources;
    PropellantTransaction();
};

enum PropellantRecoveryAction {
    PROPELLANT_RECOVERY_INVALID = 0,
    PROPELLANT_RECOVERY_FINALIZE_CHARGE = 1,
    PROPELLANT_RECOVERY_ABORT_UNCHANGED = 2,
    PROPELLANT_RECOVERY_ROLLBACK_PARTIAL = 3,
    PROPELLANT_RECOVERY_CONFLICT = 4
};

// Captures a durable-intent payload from a validated quote and the exact
// pre-write snapshot. Resource indices are stable only within this transaction;
// recovery must re-enumerate Orbiter resources and validate count/capacity.
Result createPreparedPropellantTransaction(
    const std::string& operationId, const std::string& vesselId,
    const std::vector<PropellantResourceState>& before,
    const PropellantQuote& quote, PropellantTransaction& transaction);

// Classifies restart state without mutating Orbiter. An all-after snapshot may
// be charged exactly once; all-before means no physical change; mixed before/
// after values require rollback; any third state is a conflict and must stop.
PropellantRecoveryAction classifyPropellantRecovery(
    const PropellantTransaction& transaction,
    const std::vector<PropellantResourceState>& observed,
    std::vector<double>& rollbackTargetsKg);

} // namespace oem
#endif
