#ifndef OEM_CORE_MISSION_OBJECTIVE_H
#define OEM_CORE_MISSION_OBJECTIVE_H

#include "CargoManifest.h"
#include "SimulationEvent.h"
#include <string>

namespace oem {

enum MissionObjectiveKind {
    MISSION_OBJECTIVE_DELIVER_CARGO = 1,
    MISSION_OBJECTIVE_VISIT_SPACEPORT = 2,
    MISSION_OBJECTIVE_DOCK_STATION = 3
};

// A portable objective definition. targetLocationId matches the exact base or
// station name recorded by Orbiter events; targetBodyId disambiguates surface bases.
// A mission-board offer may retain the canonical economic-location ID separately.
struct MissionObjective {
    std::string missionId;
    std::string issuerId; // Stable identity of the organization issuing this mission.
    std::string vesselId;
    MissionObjectiveKind kind;
    std::string originLocationId;
    std::string targetLocationId;
    std::string targetBodyId;
    std::string cargoId;
    long long quantity;
    SimulationSeconds acceptedAt;
    SimulationSeconds deadline;
    MissionObjective();
};

// Validates the objective's type-specific requirements without consulting Orbiter.
Result validateMissionObjective(const MissionObjective& objective);

// Evaluates only recorded Orbiter evidence. Delivery additionally requires the
// contracted cargo to have been loaded at the named origin after acceptance and
// to remain aboard at the time of the destination event.
bool missionObjectiveMatchesEvent(const MissionObjective& objective,
                                  const SimulationEvent& event,
                                  const CargoManifest& manifest);

} // namespace oem

#endif
