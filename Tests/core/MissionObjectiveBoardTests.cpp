#include "../../src/core/MissionObjectiveBoard.h"
#include <cassert>
#include <cstdio>
#include <fstream>
#include <map>
#include <string>
#include <vector>

static oem::MissionObjectiveCandidate visit(const char* target,const char* canonical,const char* body) {
    oem::MissionObjectiveCandidate c;
    c.kind=oem::MISSION_OBJECTIVE_VISIT_SPACEPORT;
    c.targetLocationId=target; c.targetCanonicalId=canonical; c.targetBodyId=body;
    c.missionType="VISIT"; c.missionTitle="Spaceport visit";
    c.missionBriefing="Land at the named spaceport and verify ground contact.";
    c.reward=350; c.allowanceSeconds=86400;
    return c;
}
static oem::MissionObjectiveCandidate dock(const char* target,const char* canonical) {
    oem::MissionObjectiveCandidate c;
    c.kind=oem::MISSION_OBJECTIVE_DOCK_STATION;
    c.targetLocationId=target; c.targetCanonicalId=canonical;
    c.missionType="DOCK"; c.missionTitle="Station docking";
    c.missionBriefing="Dock with the registered orbital station.";
    c.reward=500; c.allowanceSeconds=172800;
    return c;
}
int main() {
    oem::MissionObjectiveBoardStore store;
    std::vector<oem::MissionObjectiveCandidate> candidates;
    candidates.push_back(visit("MoonBase","surface:Moon/MoonBase","Moon"));
    candidates.push_back(visit("MarsBase","surface:Mars/MarsBase","Mars"));
    candidates.push_back(dock("ISS","station:ISS"));
    candidates.push_back(dock("Gateway","station:Gateway"));
    oem::MissionObjectiveBoard a,b;
    assert(store.generate("EarthBase","surface:Earth/EarthBase",3,1,"session|origin|3",
        candidates,a).ok);
    std::reverse(candidates.begin(),candidates.end());
    assert(store.generate("EarthBase","surface:Earth/EarthBase",3,1,"session|origin|3",
        candidates,b).ok);
    assert(a.offers.size()==b.offers.size()&&a.offers.size()<=3);
    for(std::size_t i=0;i<a.offers.size();++i) {
        assert(a.offers[i].missionId==b.offers[i].missionId);
        assert(a.offers[i].targetCanonicalId==b.offers[i].targetCanonicalId);
    }
    const std::string path="OEMMissionObjectiveBoardTests.dat";
    std::remove(path.c_str());
    std::map<std::string,oem::MissionObjectiveBoard> boards;
    boards["surface:Earth/EarthBase"]=a;
    assert(store.save(path,boards).ok);
    std::map<std::string,oem::MissionObjectiveBoard> restored;
    assert(store.load(path,restored).ok);
    assert(restored.size()==1);
    assert(restored.begin()->second.offers.size()==a.offers.size());
    for(std::size_t i=0;i<a.offers.size();++i) {
        assert(restored.begin()->second.offers[i].missionId==a.offers[i].missionId);
        assert(restored.begin()->second.offers[i].missionBriefing==a.offers[i].missionBriefing);
        assert(restored.begin()->second.offers[i].targetBodyId==a.offers[i].targetBodyId);
    }
    oem::MissionObjectiveBoard empty;
    std::vector<oem::MissionObjectiveCandidate> invalid(1,visit("EarthBase","surface:Earth/EarthBase","Earth"));
    assert(store.generate("EarthBase","surface:Earth/EarthBase",3,2,"empty",invalid,empty).ok);
    assert(empty.offers.empty());
    std::remove(path.c_str());
    return 0;
}
