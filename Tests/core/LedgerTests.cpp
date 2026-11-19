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

    const double earthRadius = 6371000.0;
    const double pi = 3.14159265358979323846;
    const double fiveKmInDegrees = 5000.0 / (earthRadius * pi / 180.0);
    assert(std::fabs(oem::surfaceDistanceMetres(0, 0, 0, 0, earthRadius)) < 1e-9);
    const double boundary = oem::surfaceDistanceMetres(0, 0, 0, fiveKmInDegrees, earthRadius);
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
