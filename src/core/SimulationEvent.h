#ifndef OEM_CORE_SIMULATION_EVENT_H
#define OEM_CORE_SIMULATION_EVENT_H

#include "Types.h"
#include <map>
#include <string>
#include <vector>

namespace oem {

enum SimulationEventType {
    SIM_EVENT_TAKEOFF = 1,
    SIM_EVENT_LANDING = 2,
    SIM_EVENT_DOCK = 3,
    SIM_EVENT_UNDOCK = 4
};

struct SimulationEvent {
    std::string eventId;
    SimulationEventType type;
    std::string vesselId;
    std::string bodyId;
    std::string baseId;
    SimulationSeconds simulationTime;
    long long simulationTimeMilliseconds;
    double distanceMetres;
    bool locationKnown;

    SimulationEvent();
};

// Append-only in-memory journal. Event IDs are idempotency keys: publishing an
// identical ID twice is a no-op; reusing an ID for different facts is rejected.
class SimulationEventQueue {
public:
    SimulationEventQueue();

    Result publish(const SimulationEvent& event);
    bool contains(const std::string& eventId) const;
    const std::vector<SimulationEvent>& events() const;

private:
    std::map<std::string, std::size_t> eventIndexes_;
    std::vector<SimulationEvent> events_;
};

const char* simulationEventTypeName(SimulationEventType type);
std::string makeSimulationEventId(const std::string& sessionId,
                                  SimulationEventType type,
                                  const std::string& vesselId,
                                  long long simulationTimeMilliseconds);

} // namespace oem

#endif
