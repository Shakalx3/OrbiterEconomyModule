#include "MissionBoard.h"
#include <cassert>
#include <cstdio>
#include <fstream>
#include <map>
#include <vector>

int main() {
    oem::MissionBoardStore store;
    std::vector<oem::MissionCandidate> candidates;
    for(int i=0;i<8;++i){
        oem::MissionCandidate c;
        c.cargoId="cargo-"+std::to_string(i);
        c.destinationLocationId="destination-"+std::to_string(i);
        c.destinationBodyId=i%2?"Mars":"Earth";
        c.destinationCanonicalId="surface:"+c.destinationBodyId+"/destination-"+std::to_string(i);
        c.missionType="SUPPLY";
        c.missionTitle="Routine supply run";
        c.missionBriefing="Deliver cargo to the destination before the deadline.";
        c.quantity=1;c.reward=100+i;c.allowanceSeconds=86400+i*3600;
        c.missionType=i%2?"PRIORITY":"SUPPLY";
        c.missionTitle="Mission title "+std::to_string(i);
        c.missionBriefing="Briefing text for cargo "+std::to_string(i);
        candidates.push_back(c);
    }
    oem::MissionBoard first,repeat;
    assert(store.generate("origin","surface:Earth/origin",4,9,"stable-seed",candidates,first).ok);
    assert(store.generate("origin","surface:Earth/origin",4,9,"stable-seed",candidates,repeat).ok);
    assert(first.offers.size()==repeat.offers.size());
    assert(first.offers.size()<=3);
    for(std::size_t i=0;i<first.offers.size();++i){
        assert(first.offers[i].contractId==repeat.offers[i].contractId);
        assert(first.offers[i].destinationLocationId==repeat.offers[i].destinationLocationId);
        assert(first.offers[i].originLocationId=="origin");
        assert(first.offers[i].missionType==repeat.offers[i].missionType);
        assert(!first.offers[i].missionTitle.empty());
        assert(!first.offers[i].missionBriefing.empty());
        assert(first.offers[i].missionType=="SUPPLY");
        assert(first.offers[i].missionTitle=="Routine supply run");
        assert(first.offers[i].deadline-first.offers[i].acceptedAt>=86400);
    }
    oem::MissionBoard empty;
    std::vector<oem::MissionCandidate> none;
    assert(store.generate("empty-origin","station:empty-origin",4,10,"empty-seed",none,empty).ok);
    assert(empty.offers.empty());

    std::map<std::string,oem::MissionBoard> boards;
    boards[first.originCanonicalId]=first;
    boards[empty.originCanonicalId]=empty;
    const std::string path="OEMMissionBoardTests.dat";
    std::remove(path.c_str());
    assert(store.save(path,boards).ok);
    std::map<std::string,oem::MissionBoard> recovered;
    assert(store.load(path,recovered).ok);
    assert(recovered.size()==2);
    assert(recovered["surface:Earth/origin"].offers.size()==first.offers.size());
    if(!first.offers.empty()) {
        assert(recovered["surface:Earth/origin"].offers[0].missionTitle==first.offers[0].missionTitle);
        assert(recovered["surface:Earth/origin"].offers[0].missionBriefing==first.offers[0].missionBriefing);
    }
    if(!first.offers.empty()) assert(recovered["surface:Earth/origin"].offers[0].missionBriefing=="Deliver cargo to the destination before the deadline.");
    assert(recovered["station:empty-origin"].offers.empty());
    std::remove(path.c_str());

    // Canonical origins with identical legacy storage IDs remain distinct.
    oem::MissionBoard earth, mars;
    assert(store.generate("TwinBase","surface:Earth/TwinBase",5,1,"earth-seed",none,earth).ok);
    assert(store.generate("TwinBase","surface:Mars/TwinBase",5,1,"mars-seed",none,mars).ok);
    std::map<std::string,oem::MissionBoard> twinBoards;
    twinBoards[earth.originCanonicalId]=earth;
    twinBoards[mars.originCanonicalId]=mars;
    assert(store.save(path,twinBoards).ok);
    recovered.clear();
    assert(store.load(path,recovered).ok);
    assert(recovered.size()==2);
    assert(recovered["surface:Earth/TwinBase"].originLocationId=="TwinBase");
    assert(recovered["surface:Mars/TwinBase"].originLocationId=="TwinBase");
    std::remove(path.c_str());

    // Existing v1 mission-board files are migrated to a namespaced legacy key.
    {
        std::ofstream legacy(path.c_str(),std::ios::binary|std::ios::trunc);
        legacy<<"OEM_MISSION_BOARDS|1\nB|6F6C6462617365|3|7|0\n";
    }
    recovered.clear();
    assert(store.load(path,recovered).ok);
    assert(recovered.size()==1);
    assert(recovered["legacy:oldbase"].originLocationId=="oldbase");
    std::remove(path.c_str());
    return 0;
}
