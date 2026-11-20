#include "../../src/core/LegacyEconomyInspector.h"
#include <cassert>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>
static void write(const char*p,const std::string&s){std::ofstream o(p,std::ios::binary);o<<s;}
int main(){
 const char*a="legacy_exchange_a.journal",*b="legacy_market_b.journal";
 const std::string xhead="OEM_CARGO_EXCHANGE|1|495353\n";
 const std::string thead="OEM_MARKET_TRADES|1|495353\n";
 const std::string x="X|7831|1|474C2D3031|495353|4F3258|1|1|10\n";
 const std::string t="T|7431|636F6D70616E79|495353|474C2D3031|4F3258|1|1|100|100|11\n";
 oem::LegacyEconomyJournalInspector inspector;oem::LegacyEconomyMigrationReport report;std::vector<oem::LegacyJournalInput> inputs;
 oem::LegacyJournalInput i;i.kind=oem::LEGACY_CARGO_EXCHANGE;i.stationId="ISS";i.path=a;inputs.push_back(i);
 i.kind=oem::LEGACY_MARKET_TRADE;i.path=b;inputs.push_back(i);
 write(a,xhead+x);std::remove(b);
 assert(inspector.inspect(inputs,report).ok);assert(report.safeSingleStream&&report.activeStreams==1&&report.totalRecords==1);
 write(b,thead+t);assert(inspector.inspect(inputs,report).ok);assert(!report.safeSingleStream&&report.activeStreams==2);
 write(a,xhead+"X|broken\n");assert(!inspector.inspect(inputs,report).ok);assert(!report.safeSingleStream);
 // Inspection must not mutate a torn tail.
 write(a,xhead+x+"partial");assert(inspector.inspect(inputs,report).ok);assert(report.findings[0].hasIncompleteTail);
 std::remove(a);std::remove(b);return 0;
}
