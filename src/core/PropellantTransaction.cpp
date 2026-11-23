#include "PropellantTransaction.h"
#include <cmath>

namespace oem {

PropellantResourceChange::PropellantResourceChange()
    : resourceIndex(0), capacityKg(0.0), beforeMassKg(0.0), afterMassKg(0.0) {}

PropellantTransaction::PropellantTransaction()
    : operationId(), vesselId(), quote(), simulationTime(0), state(PROPELLANT_TX_PREPARED), resources() {}

namespace {
bool near(double a, double b) {
    const double scale = (std::fabs(a) > std::fabs(b) ? std::fabs(a) : std::fabs(b));
    return std::fabs(a - b) <= 1.0e-9 * (scale > 1.0 ? scale : 1.0);
}
}

Result createPreparedPropellantTransaction(
    const std::string& operationId, const std::string& vesselId,
    const std::vector<PropellantResourceState>& before,
    const PropellantQuote& quote, PropellantTransaction& transaction,
    SimulationSeconds simulationTime) {
    if (operationId.empty() || vesselId.empty() || quote.quoteId.empty() ||
        quote.totalCostCredits <= 0 || quote.fillPlan.targetMassKg.size() != before.size() ||
        before.empty())
        return Result::failure("PROPELLANT_TX_INPUT",
            "A prepared transaction requires an operation, vessel, valid quote, and matching non-empty resource plan.");
    PropellantTotals totals;
    const Result inspected = inspectPropellantResources(before, totals);
    if (!inspected.ok) return inspected;
    if (quote.fillPlan.targetMassKg.size() != before.size() ||
        !std::isfinite(quote.requestedMassKg) ||
        !std::isfinite(quote.fillPlan.plannedMassKg) ||
        !near(quote.requestedMassKg, quote.fillPlan.plannedMassKg))
        return Result::failure("PROPELLANT_TX_PLAN",
            "The quote does not contain a complete fill plan for this resource snapshot.");

    long double plannedDelta = 0.0L;
    for (std::size_t i = 0; i < before.size(); ++i) {
        const double target = quote.fillPlan.targetMassKg[i];
        if (!std::isfinite(target) || target < before[i].currentMassKg ||
            target > before[i].capacityKg)
            return Result::failure("PROPELLANT_TX_TARGET",
                "A planned resource target is outside its captured before/capacity range.");
        plannedDelta += static_cast<long double>(target) -
            static_cast<long double>(before[i].currentMassKg);
    }
    const long double requested = static_cast<long double>(quote.requestedMassKg);
    if (!std::isfinite(plannedDelta) ||
        std::fabs(plannedDelta - requested) >
            1.0e-9L * (std::fabs(requested) > 1.0L ? std::fabs(requested) : 1.0L))
        return Result::failure("PROPELLANT_TX_AMOUNT",
            "The resource targets do not deliver exactly the quoted fuel quantity.");

    PropellantTransaction candidate;
    candidate.operationId = operationId;
    candidate.vesselId = vesselId;
    candidate.quote = quote;
    candidate.simulationTime = simulationTime;
    candidate.state = PROPELLANT_TX_PREPARED;
    for (std::size_t i = 0; i < before.size(); ++i) {
        const double target = quote.fillPlan.targetMassKg[i];
        PropellantResourceChange change;
        change.resourceIndex = i;
        change.capacityKg = before[i].capacityKg;
        change.beforeMassKg = before[i].currentMassKg;
        change.afterMassKg = target;
        candidate.resources.push_back(change);
    }
    transaction = candidate;
    return Result::success();
}

PropellantRecoveryAction classifyPropellantRecovery(
    const PropellantTransaction& transaction,
    const std::vector<PropellantResourceState>& observed,
    std::vector<double>& rollbackTargetsKg) {
    if (transaction.state != PROPELLANT_TX_PREPARED ||
        transaction.operationId.empty() || transaction.vesselId.empty() ||
        transaction.resources.empty() ||
        observed.size() != transaction.resources.size())
        return PROPELLANT_RECOVERY_INVALID;

    bool allBefore = true;
    bool allAfter = true;
    bool everyValueKnown = true;
    std::vector<double> rollback;
    rollback.reserve(transaction.resources.size());
    for (std::size_t i = 0; i < transaction.resources.size(); ++i) {
        const PropellantResourceChange& change = transaction.resources[i];
        const PropellantResourceState& actual = observed[i];
        if (change.resourceIndex != i || !std::isfinite(actual.currentMassKg) ||
            !std::isfinite(actual.capacityKg) || actual.currentMassKg < 0.0 ||
            actual.capacityKg < 0.0 || actual.currentMassKg > actual.capacityKg ||
            !near(actual.capacityKg, change.capacityKg))
            return PROPELLANT_RECOVERY_CONFLICT;
        const bool before = near(actual.currentMassKg, change.beforeMassKg);
        const bool after = near(actual.currentMassKg, change.afterMassKg);
        if (!before && !after) everyValueKnown = false;
        if (!before) allBefore = false;
        if (!after) allAfter = false;
        rollback.push_back(change.beforeMassKg);
    }
    if (!everyValueKnown) return PROPELLANT_RECOVERY_CONFLICT;
    if (allAfter) {
        rollbackTargetsKg.clear();
        return PROPELLANT_RECOVERY_FINALIZE_CHARGE;
    }
    if (allBefore) {
        rollbackTargetsKg.clear();
        return PROPELLANT_RECOVERY_ABORT_UNCHANGED;
    }
    rollbackTargetsKg.swap(rollback);
    return PROPELLANT_RECOVERY_ROLLBACK_PARTIAL;
}

} // namespace oem
