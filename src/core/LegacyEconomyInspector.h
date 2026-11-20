#ifndef OEM_CORE_LEGACY_ECONOMY_INSPECTOR_H
#define OEM_CORE_LEGACY_ECONOMY_INSPECTOR_H
#include "Types.h"
#include <string>
#include <vector>
namespace oem {
enum LegacyJournalKind { LEGACY_CARGO_EXCHANGE=1, LEGACY_MARKET_TRADE=2 };
struct LegacyJournalInput { LegacyJournalKind kind; std::string stationId; std::string path; };
struct LegacyJournalFinding {
    std::string path, stationId;
    LegacyJournalKind kind;
    bool exists, valid, hasIncompleteTail;
    std::size_t completeRecords;
    std::string code, message;
    LegacyJournalFinding() : kind(LEGACY_CARGO_EXCHANGE), exists(false), valid(true),
        hasIncompleteTail(false), completeRecords(0) {}
};
struct LegacyEconomyMigrationReport {
    bool safeSingleStream;
    std::size_t activeStreams, totalRecords;
    std::vector<LegacyJournalFinding> findings;
    std::string summary;
    LegacyEconomyMigrationReport() : safeSingleStream(false), activeStreams(0), totalRecords(0) {}
};
class LegacyEconomyJournalInspector {
public:
    // Read-only: never repairs, truncates, renames, or writes legacy files.
    Result inspect(const std::vector<LegacyJournalInput>& inputs,
                   LegacyEconomyMigrationReport& report) const;
};
}
#endif
