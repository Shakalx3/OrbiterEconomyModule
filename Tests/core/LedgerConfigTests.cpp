#include "../../src/core/LedgerConfig.h"
#include <cassert>
#include <cstdio>
#include <fstream>
int main(){const char*p="OEMLedgerConfigTests.cfg";std::remove(p);oem::Ledger l;assert(!oem::LedgerConfigStore().load(p,l).ok);{std::ofstream o(p);o<<"OEM_ACCOUNTS|1\nA|636F6D70616E79|1000\nA|495353|250\n";}assert(oem::LedgerConfigStore().load(p,l).ok);oem::Credits b=0;assert(l.getBalance("company",b)&&b==1000);assert(l.getBalance("ISS",b)&&b==250);{std::ofstream o(p);o<<"OEM_ACCOUNTS|1\nA|636F6D70616E79|1000\nA|636F6D70616E79|5\n";}oem::Ledger unchanged;assert(unchanged.createAccount("keep",7).ok);assert(!oem::LedgerConfigStore().load(p,unchanged).ok);assert(unchanged.getBalance("keep",b)&&b==7);std::remove(p);return 0;}
