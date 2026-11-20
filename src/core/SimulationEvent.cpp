#include "SimulationEvent.h"
#include <sstream>

namespace oem {

SimulationEvent::SimulationEvent()
    : eventId(), type(SIM_EVENT_TAKEOFF), vesselId(), bodyId(), baseId(),
      simulationTime(0), simulationTimeMilliseconds(0),
      distanceMetres(-1.0), locationKnown(false) {}

SimulationEventQueue::SimulationEventQueue() : eventIndexes_(), events_() {}

Result SimulationEventQueue::publish(const SimulationEvent& event) {
    if (event.eventId.empty() || event.vesselId.empty())
        return Result::failure("INVALID_EVENT", "Event ID and vessel ID are required.");

    std::map<std::string, std::size_t>::const_iterator prior =
        eventIndexes_.find(event.eventId);
    if (prior != eventIndexes_.end()) {
        const SimulationEvent& old = events_[prior->second];
        if (old.type == event.type && old.vesselId == event.vesselId &&
            old.bodyId == event.bodyId && old.baseId == event.baseId &&
            old.simulationTimeMilliseconds == event.simulationTimeMilliseconds &&
            old.locationKnown == event.locationKnown &&
            old.distanceMetres == event.distanceMetres)
            return Result::success();
        return Result::failure("EVENT_ID_CONFLICT",
                               "Event ID was already used for different facts.");
    }

    events_.push_back(event);
    eventIndexes_[event.eventId] = events_.size() - 1;
    return Result::success();
}

bool SimulationEventQueue::contains(const std::string& eventId) const {
    return eventIndexes_.find(eventId) != eventIndexes_.end();
}

const std::vector<SimulationEvent>& SimulationEventQueue::events() const {
    return events_;
}

const char* simulationEventTypeName(SimulationEventType type) {
    switch (type) {
    case SIM_EVENT_TAKEOFF: return "TAKEOFF";
    case SIM_EVENT_LANDING: return "LANDING";
    case SIM_EVENT_DOCK: return "DOCK";
    case SIM_EVENT_UNDOCK: return "UNDOCK";
    default: return "UNKNOWN";
    }
}

std::string makeSimulationEventId(const std::string& sessionId,
                                  SimulationEventType type,
                                  const std::string& vesselId,
                                  long long simulationTimeMilliseconds) {
    std::ostringstream id;
    id << "orbiter:" << sessionId << ":" << simulationEventTypeName(type)
       << ":" << vesselId << ":" << simulationTimeMilliseconds;
    return id.str();
}

} // namespace oem
