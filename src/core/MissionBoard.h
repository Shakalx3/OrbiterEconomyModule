#ifndef OEM_CORE_MISSION_BOARD_H
#define OEM_CORE_MISSION_BOARD_H

#include "DeliveryContract.h"
#include <map>
#include <string>
#include <vector>

namespace oem {

struct MissionCandidate {
    std::string cargoId;
    std::string destinationLocationId;
    std::string destinationBodyId;
    long long quantity;
    Credits reward;
    SimulationSeconds allowanceSeconds;
    MissionCandidate();
};

struct MissionBoard {
    std::string originLocationId;
    long long simulationDay;
    unsigned long generation;
    std::vector<DeliveryContract> offers;
    MissionBoard();
};

class MissionBoardStore {
public:
    Result load(const std::string& path, std::map<std::string, MissionBoard>& boards) const;
    Result save(const std::string& path, const std::map<std::string, MissionBoard>& boards) const;
    Result generate(const std::string& origin, long long day, unsigned long generation,
                    const std::string& seed, const std::vector<MissionCandidate>& candidates,
                    MissionBoard& board) const;
};

}
#endif
