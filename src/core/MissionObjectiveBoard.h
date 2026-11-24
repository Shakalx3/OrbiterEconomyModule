#ifndef OEM_CORE_MISSION_OBJECTIVE_BOARD_H
#define OEM_CORE_MISSION_OBJECTIVE_BOARD_H

#include "MissionObjective.h"
#include <map>
#include <string>
#include <vector>

namespace oem {

struct MissionObjectiveCandidate {
    std::string issuerId;
    MissionObjectiveKind kind;
    std::string targetLocationId;
    std::string targetBodyId;
    std::string targetCanonicalId;
    std::string missionType;
    std::string missionTitle;
    std::string missionBriefing;
    Credits reward;
    SimulationSeconds allowanceSeconds;
    MissionObjectiveCandidate();
};

struct MissionObjectiveOffer {
    std::string missionId;
    std::string issuerId;
    MissionObjectiveKind kind;
    std::string targetLocationId;
    std::string targetBodyId;
    std::string targetCanonicalId;
    std::string missionType;
    std::string missionTitle;
    std::string missionBriefing;
    Credits reward;
    SimulationSeconds allowanceSeconds;
    MissionObjectiveOffer();
};

struct MissionObjectiveBoard {
    std::string originLocationId;
    std::string originCanonicalId;
    long long simulationDay;
    unsigned long generation;
    std::vector<MissionObjectiveOffer> offers;
    MissionObjectiveBoard();
};

class MissionObjectiveBoardStore {
public:
    Result load(const std::string& path,
                std::map<std::string, MissionObjectiveBoard>& boards) const;
    Result save(const std::string& path,
                const std::map<std::string, MissionObjectiveBoard>& boards) const;
    Result generate(const std::string& originLocationId,
                    const std::string& originCanonicalId,
                    long long day, unsigned long generation,
                    const std::string& seed,
                    const std::vector<MissionObjectiveCandidate>& candidates,
                    MissionObjectiveBoard& board) const;
};

} // namespace oem
#endif
