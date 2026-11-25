#include "MissionObjective.h"
#include <limits>

namespace oem {

MissionObjective::MissionObjective()
    : missionId(), issuerId(), vesselId(), kind(MISSION_OBJECTIVE_DELIVER_CARGO),
      originLocationId(), targetLocationId(), targetBodyId(), cargoId(),
      quantity(0), acceptedAt(0), deadline(0) {}

Result validateMissionObjective(const MissionObjective& o) {
    if (o.missionId.empty() || o.vesselId.empty() || o.targetLocationId.empty())
        return Result::failure("MISSION_OBJECTIVE_IDENTITY",
            "Mission, vessel and target location IDs are required.");
    if (o.acceptedAt < 0 || (o.deadline > 0 && o.deadline < o.acceptedAt))
        return Result::failure("MISSION_OBJECTIVE_TIME",
            "Mission deadline cannot precede acceptance.");
    switch (o.kind) {
    case MISSION_OBJECTIVE_DELIVER_CARGO:
        if (o.originLocationId.empty() || o.cargoId.empty() || o.quantity <= 0)
            return Result::failure("MISSION_OBJECTIVE_CARGO",
                "Cargo delivery requires origin, cargo and positive quantity.");
        if (o.originLocationId == o.targetLocationId)
            return Result::failure("MISSION_OBJECTIVE_ROUTE",
                "Cargo delivery origin and target must differ.");
        break;
    case MISSION_OBJECTIVE_VISIT_SPACEPORT:
        if (o.targetBodyId.empty())
            return Result::failure("MISSION_OBJECTIVE_BODY",
                "A spaceport visit must identify its celestial body.");
        break;
    case MISSION_OBJECTIVE_DOCK_STATION:
        break;
    default:
        return Result::failure("MISSION_OBJECTIVE_KIND",
            "Mission objective kind is not supported.");
    }
    return Result::success();
}

namespace {
long long eligibleCargoAt(const MissionObjective& o,
                          const CargoManifest& manifest,
                          SimulationSeconds eventTime) {
    if (manifest.vesselId() != o.vesselId) return 0;
    long long eligible = 0;
    const std::vector<CargoTransfer>& history = manifest.history();
    for (std::size_t i = 0; i < history.size(); ++i) {
        const CargoTransfer& transfer = history[i];
        if (transfer.simulationTime > eventTime || transfer.cargoId != o.cargoId)
            continue;
        if (transfer.direction == CARGO_LOAD &&
            transfer.locationId == o.originLocationId &&
            transfer.simulationTime >= o.acceptedAt) {
            if (eligible > std::numeric_limits<long long>::max() - transfer.quantity)
                return std::numeric_limits<long long>::max();
            eligible += transfer.quantity;
        } else if (transfer.direction == CARGO_UNLOAD && eligible > 0) {
            eligible = transfer.quantity >= eligible ? 0 : eligible - transfer.quantity;
        }
    }
    return eligible;
}
}

bool missionObjectiveMatchesEvent(const MissionObjective& o,
                                  const SimulationEvent& e,
                                  const CargoManifest& manifest) {
    if (!validateMissionObjective(o).ok || e.eventId.empty() || e.vesselId.empty() ||
        e.vesselId != o.vesselId || e.simulationTime < o.acceptedAt ||
        (o.deadline > 0 && e.simulationTime > o.deadline))
        return false;

    if (o.kind == MISSION_OBJECTIVE_VISIT_SPACEPORT) {
        return e.type == SIM_EVENT_LANDING && e.locationKnown &&
            e.baseId == o.targetLocationId && e.bodyId == o.targetBodyId;
    }
    if (o.kind == MISSION_OBJECTIVE_DOCK_STATION) {
        return e.type == SIM_EVENT_DOCK && e.relatedVesselId == o.targetLocationId;
    }
    if (o.kind == MISSION_OBJECTIVE_DELIVER_CARGO) {
        const bool atTarget =
            (e.type == SIM_EVENT_LANDING && e.locationKnown &&
             e.baseId == o.targetLocationId &&
             (o.targetBodyId.empty() || e.bodyId == o.targetBodyId)) ||
            (e.type == SIM_EVENT_DOCK && e.relatedVesselId == o.targetLocationId);
        return atTarget && eligibleCargoAt(o, manifest, e.simulationTime) >= o.quantity;
    }
    return false;
}

} // namespace oem
