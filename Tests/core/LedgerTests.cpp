#include "../../src/core/Ledger.h"
#include "../../src/core/SurfaceDistance.h"
#include <cassert>
#include <cmath>

int main() {
    oem::Ledger ledger;
    assert(ledger.createAccount("company", 100000).ok);
    assert(ledger.createAccount("station", 0).ok);
    assert(ledger.transfer("purchase-001", "company", "station", 1250, 10, "Test purchase").ok);

    oem::Credits company = 0, station = 0;
    assert(ledger.getBalance("company", company));
    assert(ledger.getBalance("station", station));
    assert(company == 98750 && station == 1250);
    assert(ledger.transfer("purchase-001", "company", "station", 1250, 10, "Replay").ok);
    assert(ledger.getBalance("company", company) && ledger.getBalance("station", station));
    assert(company == 98750 && station == 1250 && ledger.entries().size() == 1);
    assert(!ledger.transfer("purchase-001", "company", "station", 2000, 10, "Conflict").ok);
    assert(ledger.entries().size() == 1);
    assert(!ledger.transfer("bad-amount", "company", "station", -1, 11, "Invalid").ok);
    assert(!ledger.transfer("too-expensive", "company", "station", 999999, 12, "Invalid").ok);

    // Reputation starts at 50, is bounded, and is independently idempotent.
    int reputation = -1;
    assert(!ledger.getReputation("company", reputation));
    assert(ledger.applyReputation("failure-001", "company", -7, 20, "Expired mission").ok);
    assert(ledger.getReputation("company", reputation) && reputation == 43);
    assert(ledger.applyReputation("failure-001", "company", -7, 20, "Replay").ok);
    assert(ledger.getReputation("company", reputation) && reputation == 43);
    assert(ledger.reputationEntries().size() == 1);
    assert(!ledger.applyReputation("failure-001", "company", -8, 20, "Conflict").ok);
    assert(ledger.applyReputation("failure-002", "company", -100, 21, "Repeated failures").ok);
    assert(ledger.getReputation("company", reputation) && reputation == 0);
    assert(ledger.applyReputation("recovery-001", "company", 500, 22, "Recovery").ok);
    assert(ledger.getReputation("company", reputation) && reputation == 100);
    oem::Ledger copied = ledger;
    assert(copied.getReputation("company", reputation) && reputation == 100);
    assert(copied.applyReputation("recovery-001", "company", 500, 22, "Replay").ok);

    const double earthRadius = 6371000.0;
    const double pi = 3.14159265358979323846;
    const double fiveKmInRadians = 5000.0 / earthRadius;
    assert(std::fabs(oem::surfaceDistanceMetres(0, 0, 0, 0, earthRadius)) < 1e-9);
    const double boundary = oem::surfaceDistanceMetres(0, 0, 0, fiveKmInRadians, earthRadius);
    assert(std::fabs(boundary - 5000.0) < 1e-6);
    assert(oem::isWithinSpaceportRadius(boundary, 5000.0));
    assert(!oem::isWithinSpaceportRadius(boundary + 0.01, 5000.0));

    const double wrapDistance = oem::surfaceDistanceMetres(0, 179.999 * pi / 180.0,
                                                            0, -179.999 * pi / 180.0, earthRadius);
    assert(wrapDistance < 250.0);
    assert(oem::surfaceDistanceMetres(0, 0, 0, 0, -1.0) < 0.0);
    assert(!oem::isWithinSpaceportRadius(-1.0, 5000.0));
    return 0;
}
