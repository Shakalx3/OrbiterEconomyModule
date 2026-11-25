#include "../../src/core/PropellantTransactionJournal.h"
#include <cassert>
#include <cstdio>
#include <fstream>

int main(){
    const std::string path="OEMPropellantTransactionJournalTests.tmp";
    std::remove(path.c_str());
    std::vector<oem::PropellantResourceState> resources;
    resources.push_back(oem::PropellantResourceState(10.0,100.0));
    oem::PropellantPriceContext context;context.locationId="station:test";
    context.locationKind=oem::PROPELLANT_LOCATION_ORBITAL_STATION;context.celestialBody="Earth";
    oem::PropellantQuote quote;assert(oem::createPropellantQuote(resources,"Q1",context,20.0,2.0,1000,quote).ok);
    oem::PropellantTransaction tx;assert(oem::createPreparedPropellantTransaction("OP1","Ship1",resources,quote,tx,9876).ok);
    oem::PropellantTransactionJournal journal;
    assert(journal.prepare(path,tx).ok);
    assert(journal.prepare(path,tx).ok); // idempotent retry
    std::map<std::string,oem::PropellantTransaction> loaded;
    assert(journal.load(path,loaded).ok);assert(loaded.size()==1);
    assert(loaded["OP1"].state==oem::PROPELLANT_TX_PREPARED);
    assert(loaded["OP1"].simulationTime==9876);
    assert(loaded["OP1"].resources.size()==1);
    assert(journal.setTerminalState(path,"OP1",oem::PROPELLANT_TX_COMMITTED).ok);
    assert(journal.setTerminalState(path,"OP1",oem::PROPELLANT_TX_COMMITTED).ok);
    assert(journal.load(path,loaded).ok);
    assert(loaded["OP1"].state==oem::PROPELLANT_TX_COMMITTED);
    assert(!journal.setTerminalState(path,"OP1",oem::PROPELLANT_TX_ABORTED).ok);
    oem::PropellantTransaction conflict=tx;conflict.vesselId="OtherShip";
    assert(!journal.prepare(path,conflict).ok);
    std::remove(path.c_str());
    return 0;
}
