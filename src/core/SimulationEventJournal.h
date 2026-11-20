#ifndef OEM_CORE_SIMULATION_EVENT_JOURNAL_H
#define OEM_CORE_SIMULATION_EVENT_JOURNAL_H

#include "SimulationEvent.h"
#include <string>

namespace oem {

// Versioned, append-only-in-meaning event history. Saves use a temporary file
// and atomic replacement; an incomplete final line is ignored during recovery.
class SimulationEventJournalStore {
public:
    Result load(const std::string& path, SimulationEventQueue& queue) const;
    Result save(const std::string& path, const SimulationEventQueue& queue) const;
};

} // namespace oem

#endif
