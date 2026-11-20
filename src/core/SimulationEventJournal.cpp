#include "SimulationEventJournal.h"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#else
#include <cstdio>
#endif

namespace oem {
namespace {

const char* JOURNAL_HEADER = "OEM_EVENT_JOURNAL|1";

std::string encodeHex(const std::string& value) {
    static const char digits[] = "0123456789ABCDEF";
    std::string out;
    out.reserve(value.size() * 2);
    for (std::size_t i = 0; i < value.size(); ++i) {
        const unsigned char c = static_cast<unsigned char>(value[i]);
        out.push_back(digits[c >> 4]);
        out.push_back(digits[c & 15]);
    }
    return out;
}

bool decodeHex(const std::string& value, std::string& out) {
    if ((value.size() & 1u) != 0) return false;
    out.clear();
    out.reserve(value.size() / 2);
    for (std::size_t i = 0; i < value.size(); i += 2) {
        const char a = value[i], b = value[i + 1];
        const int hi = a >= '0' && a <= '9' ? a - '0' :
                       a >= 'A' && a <= 'F' ? a - 'A' + 10 :
                       a >= 'a' && a <= 'f' ? a - 'a' + 10 : -1;
        const int lo = b >= '0' && b <= '9' ? b - '0' :
                       b >= 'A' && b <= 'F' ? b - 'A' + 10 :
                       b >= 'a' && b <= 'f' ? b - 'a' + 10 : -1;
        if (hi < 0 || lo < 0) return false;
        out.push_back(static_cast<char>((hi << 4) | lo));
    }
    return true;
}

std::vector<std::string> splitFields(const std::string& line) {
    std::vector<std::string> fields;
    std::istringstream input(line);
    std::string field;
    while (std::getline(input, field, '|')) fields.push_back(field);
    return fields;
}

bool parseLongLong(const std::string& text, long long& value) {
    if (text.empty()) return false;
    char* end = 0;
    const long long parsed = std::strtoll(text.c_str(), &end, 10);
    if (!end || *end != '\0') return false;
    value = parsed;
    return true;
}

bool parseDouble(const std::string& text, double& value) {
    if (text.empty()) return false;
    char* end = 0;
    const double parsed = std::strtod(text.c_str(), &end);
    if (!end || *end != '\0') return false;
    value = parsed;
    return true;
}

bool parseEvent(const std::string& line, SimulationEvent& event) {
    const std::vector<std::string> f = splitFields(line);
    if (f.size() != 10 || f[0] != "E") return false;

    long long type = 0, simTime = 0, simMs = 0, known = 0;
    if (!decodeHex(f[1], event.eventId) ||
        !parseLongLong(f[2], type) || type < SIM_EVENT_TAKEOFF ||
        type > SIM_EVENT_UNDOCK ||
        !decodeHex(f[3], event.vesselId) ||
        !decodeHex(f[4], event.bodyId) ||
        !decodeHex(f[5], event.baseId) ||
        !parseLongLong(f[6], simTime) ||
        !parseLongLong(f[7], simMs) ||
        !parseDouble(f[8], event.distanceMetres) ||
        !parseLongLong(f[9], known) || (known != 0 && known != 1))
        return false;

    event.type = static_cast<SimulationEventType>(type);
    event.simulationTime = simTime;
    event.simulationTimeMilliseconds = simMs;
    event.locationKnown = known != 0;
    return !event.eventId.empty() && !event.vesselId.empty();
}

void writeEvent(std::ostream& out, const SimulationEvent& e) {
    out << "E|" << encodeHex(e.eventId) << '|'
        << static_cast<int>(e.type) << '|'
        << encodeHex(e.vesselId) << '|'
        << encodeHex(e.bodyId) << '|'
        << encodeHex(e.baseId) << '|'
        << e.simulationTime << '|'
        << e.simulationTimeMilliseconds << '|'
        << std::setprecision(17) << e.distanceMetres << '|'
        << (e.locationKnown ? 1 : 0) << '\n';
}

bool replaceFile(const std::string& temporary, const std::string& destination) {
#ifdef _WIN32
    return MoveFileExA(temporary.c_str(), destination.c_str(),
                       MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
    return std::rename(temporary.c_str(), destination.c_str()) == 0;
#endif
}

} // namespace

Result SimulationEventJournalStore::load(const std::string& path,
                                         SimulationEventQueue& queue) const {
    std::ifstream input(path.c_str(), std::ios::in | std::ios::binary);
    if (!input.is_open()) return Result::success();

    std::string header;
    if (!std::getline(input, header))
        return Result::failure("JOURNAL_EMPTY", "Journal file is empty.");
    if (header != JOURNAL_HEADER)
        return Result::failure("JOURNAL_VERSION", "Journal header or version is not supported.");

    std::string line;
    while (std::getline(input, line)) {
        if (line.empty()) continue;
        SimulationEvent event;
        if (!parseEvent(line, event)) {
            // A final line without a terminating newline may be a torn write.
            if (input.eof()) break;
            return Result::failure("JOURNAL_CORRUPT", "A complete journal record is invalid.");
        }
        const Result published = queue.publish(event);
        if (!published.ok)
            return Result::failure(published.code, published.message);
    }
    return Result::success();
}

Result SimulationEventJournalStore::save(const std::string& path,
                                         const SimulationEventQueue& queue) const {
    const std::string temporary = path + ".tmp";
    {
        std::ofstream output(temporary.c_str(),
                             std::ios::out | std::ios::binary | std::ios::trunc);
        if (!output.is_open())
            return Result::failure("JOURNAL_OPEN_FAILED", "Could not create temporary journal file.");

        output << JOURNAL_HEADER << '\n';
        const std::vector<SimulationEvent>& events = queue.events();
        for (std::size_t i = 0; i < events.size(); ++i)
            writeEvent(output, events[i]);
        output.flush();
        if (!output.good())
            return Result::failure("JOURNAL_WRITE_FAILED", "Could not write complete journal.");
        output.close();
        if (output.fail())
            return Result::failure("JOURNAL_CLOSE_FAILED", "Could not close journal file.");
    }

    if (!replaceFile(temporary, path)) {
        std::remove(temporary.c_str());
        return Result::failure("JOURNAL_REPLACE_FAILED", "Could not atomically replace journal file.");
    }
    return Result::success();
}

} // namespace oem
