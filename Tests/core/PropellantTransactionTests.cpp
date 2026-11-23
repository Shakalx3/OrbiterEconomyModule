#include "../../src/core/PropellantTransaction.h"
#include <cassert>
#include <cmath>

int main() {
    std::vector<oem::PropellantResourceState> before;
    before.push_back(oem::PropellantResourceState(20.0, 100.0));
    before.push_back(oem::PropellantResourceState(5.0, 25.0));

    oem::PropellantPriceContext context;
    context.locationId = "station:ISS";
    context.locationKind = oem::PROPELLANT_LOCATION_ORBITAL_STATION;
    context.celestialBody = "Earth";
    oem::PropellantQuote quote;
    assert(oem::createPropellantQuote(before, "QUOTE-1", context,
        50.0, 2.0, 1000, quote).ok);

    oem::PropellantTransaction tx;
    assert(oem::createPreparedPropellantTransaction(
        "REFUEL-OP-1", "TestShip", before, quote, tx).ok);
    assert(tx.state == oem::PROPELLANT_TX_PREPARED);
    assert(tx.resources.size() == 2);
    assert(std::fabs(tx.resources[0].afterMassKg - 70.0) < 1e-9);
    assert(std::fabs(tx.resources[1].afterMassKg - 5.0) < 1e-9);

    std::vector<double> rollback;
    assert(oem::classifyPropellantRecovery(tx, before, rollback) ==
        oem::PROPELLANT_RECOVERY_ABORT_UNCHANGED);
    assert(rollback.empty());

    std::vector<oem::PropellantResourceState> after;
    after.push_back(oem::PropellantResourceState(70.0, 100.0));
    after.push_back(oem::PropellantResourceState(5.0, 25.0));
    assert(oem::classifyPropellantRecovery(tx, after, rollback) ==
        oem::PROPELLANT_RECOVERY_FINALIZE_CHARGE);
    assert(rollback.empty());

    std::vector<oem::PropellantResourceState> partial;
    partial.push_back(oem::PropellantResourceState(70.0, 100.0));
    partial.push_back(oem::PropellantResourceState(5.0, 25.0));
    // The second resource did not change, so this is a complete after-state
    // for this deterministic plan and must finalize, not roll back.
    assert(oem::classifyPropellantRecovery(tx, partial, rollback) ==
        oem::PROPELLANT_RECOVERY_FINALIZE_CHARGE);

    // Use a second plan that changes both tanks to exercise partial-write recovery.
    std::vector<oem::PropellantResourceState> twoTanks;
    twoTanks.push_back(oem::PropellantResourceState(20.0, 100.0));
    twoTanks.push_back(oem::PropellantResourceState(5.0, 25.0));
    oem::PropellantQuote twoQuote;
    assert(oem::createPropellantQuote(twoTanks, "QUOTE-2", context,
        80.0, 2.0, 1000, twoQuote).ok);
    oem::PropellantTransaction twoTx;
    assert(oem::createPreparedPropellantTransaction(
        "REFUEL-OP-2", "TestShip", twoTanks, twoQuote, twoTx).ok);
    std::vector<oem::PropellantResourceState> mixed;
    mixed.push_back(oem::PropellantResourceState(100.0, 100.0));
    mixed.push_back(oem::PropellantResourceState(5.0, 25.0));
    assert(oem::classifyPropellantRecovery(twoTx, mixed, rollback) ==
        oem::PROPELLANT_RECOVERY_ROLLBACK_PARTIAL);
    assert(rollback.size() == 2);
    assert(std::fabs(rollback[0] - 20.0) < 1e-9);
    assert(std::fabs(rollback[1] - 5.0) < 1e-9);

    std::vector<oem::PropellantResourceState> conflict;
    conflict.push_back(oem::PropellantResourceState(99.0, 100.0));
    conflict.push_back(oem::PropellantResourceState(5.0, 25.0));
    assert(oem::classifyPropellantRecovery(twoTx, conflict, rollback) ==
        oem::PROPELLANT_RECOVERY_CONFLICT);

    std::vector<oem::PropellantResourceState> wrongCapacity;
    wrongCapacity.push_back(oem::PropellantResourceState(20.0, 90.0));
    wrongCapacity.push_back(oem::PropellantResourceState(5.0, 25.0));
    assert(oem::classifyPropellantRecovery(twoTx, wrongCapacity, rollback) ==
        oem::PROPELLANT_RECOVERY_CONFLICT);
    return 0;
}
