#include "../../src/core/Ledger.h"
#include <cassert>

int main() {
    oem::Ledger ledger;
    assert(ledger.createAccount("company", 100000).ok);
    assert(ledger.createAccount("station", 0).ok);

    assert(ledger.transfer("purchase-001", "company", "station", 1250, 10, "Test purchase").ok);

    oem::Credits company = 0;
    oem::Credits station = 0;
    assert(ledger.getBalance("company", company));
    assert(ledger.getBalance("station", station));
    assert(company == 98750);
    assert(station == 1250);

    // Replaying the same operation must not transfer funds a second time.
    assert(ledger.transfer("purchase-001", "company", "station", 1250, 10, "Replay").ok);
    assert(ledger.getBalance("company", company));
    assert(ledger.getBalance("station", station));
    assert(company == 98750);
    assert(station == 1250);
    assert(ledger.entries().size() == 1);

    assert(!ledger.transfer("bad-amount", "company", "station", -1, 11, "Invalid").ok);
    assert(!ledger.transfer("too-expensive", "company", "station", 999999, 12, "Invalid").ok);
    return 0;
}
