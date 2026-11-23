#include "MissionBoard.h"
#include <cassert>
#include <cstdio>
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
        c.quantity=1;c.reward=100+i;c.allowanceSeconds=86400+i*3600;
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
    assert(recovered["station:empty-origin"].offers.empty());
    std::remove(path.c_str());
    return 0;
}
