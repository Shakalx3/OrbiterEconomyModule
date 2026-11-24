#ifndef OEM_CORE_MISSION_OBJECTIVE_BOOK_H
#define OEM_CORE_MISSION_OBJECTIVE_BOOK_H
#include "Ledger.h"
#include "MissionObjective.h"
#include <map>
#include <string>
#include <vector>
namespace oem {
enum MissionObjectiveStatus { MISSION_OBJECTIVE_ACCEPTED=1, MISSION_OBJECTIVE_COMPLETED=2, MISSION_OBJECTIVE_SETTLED=3, MISSION_OBJECTIVE_EXPIRED=4, MISSION_OBJECTIVE_CANCELLED=5 };
struct MissionObjectiveContract {
    MissionObjective objective; Credits reward; MissionObjectiveStatus status;
    std::string completionEventId; SimulationSeconds completedAt;
    MissionObjectiveContract();
};
// Portable lifecycle for non-delivery objectives. Durable serialization and Orbiter wiring are separate.
class MissionObjectiveBook {
public:
    MissionObjectiveBook();
    Result accept(const MissionObjective& objective, Credits reward);
    Result observe(const SimulationEvent& event, const CargoManifest& manifest);
    Result settle(const std::string& missionId, const std::string& accountId, Ledger& ledger);
    Result restoreCompleted(const std::string& missionId, const std::string& eventId, SimulationSeconds completedAt);
    Result restoreSettled(const std::string& missionId);
    Result restoreExpired(const std::string& missionId);
    Result cancel(const std::string& missionId);
    std::vector<std::string> expireDue(SimulationSeconds now);
    const MissionObjectiveContract* find(const std::string& missionId) const;
    const std::vector<MissionObjectiveContract>& missions() const;
private:
    std::map<std::string, std::size_t> indexes_;
    std::vector<MissionObjectiveContract> missions_;
};
}
#endif
