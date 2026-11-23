#include "MissionBoard.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#ifdef _WIN32
#include <windows.h>
#endif
#include <fstream>
#include <sstream>
#include <set>

namespace oem {
namespace {
const char* kHeader = "OEM_MISSION_BOARDS|3";
std::string hex(const std::string& value) {
    if(value.empty()) return "-";
    static const char digits[] = "0123456789ABCDEF";
    std::string out;
    for(std::size_t i=0;i<value.size();++i) {
        const unsigned char c=(unsigned char)value[i];
        out.push_back(digits[c>>4]); out.push_back(digits[c&15]);
    }
    return out;
}
int hexDigit(char c) {
    if(c>='0'&&c<='9') return c-'0';
    if(c>='A'&&c<='F') return c-'A'+10;
    if(c>='a'&&c<='f') return c-'a'+10;
    return -1;
}
bool unhex(const std::string& value,std::string& out) {
    if(value=="-"){out.clear();return true;}
    if(value.size()%2) return false;
    out.clear();
    for(std::size_t i=0;i<value.size();i+=2) {
        const int high=hexDigit(value[i]), low=hexDigit(value[i+1]);
        if(high<0||low<0) return false;
        out.push_back((char)((high<<4)|low));
    }
    return true;
}
std::vector<std::string> split(const std::string& line) {
    std::vector<std::string> fields; std::istringstream in(line); std::string item;
    while(std::getline(in,item,'|')) fields.push_back(item);
    return fields;
}
bool number(const std::string& s,long long& n) {
    if(s.empty()) return false;
    char* end=0; n=std::strtoll(s.c_str(),&end,10);
    return end && *end==0;
}
unsigned long score(const std::string& s) {
    unsigned long h=2166136261UL;
    for(std::size_t i=0;i<s.size();++i){h^=(unsigned char)s[i];h*=16777619UL;}
    return h;
}
bool writeBoard(std::ostream& out,const MissionBoard& b) {
    out<<"B|"<<hex(b.originCanonicalId)<<"|"<<hex(b.originLocationId)<<"|"
       <<b.simulationDay<<"|"<<b.generation<<"|"<<b.offers.size()<<"\n";
    for(std::size_t i=0;i<b.offers.size();++i) {
        const DeliveryContract& c=b.offers[i];
        out<<"O|"<<hex(c.contractId)<<"|"<<hex(c.vesselId)<<"|"<<hex(c.cargoId)<<"|"<<c.quantity
           <<"|"<<hex(c.originLocationId)<<"|"<<hex(c.destinationLocationId)<<"|"<<hex(c.destinationBodyId)
           <<"|"<<c.reward<<"|"<<c.acceptedAt<<"|"<<c.deadline<<"\n";
    }
    return (bool)out;
}
}
MissionCandidate::MissionCandidate()
    : cargoId(),destinationLocationId(),destinationBodyId(),destinationCanonicalId(),
      missionType(),missionTitle(),missionBriefing(),quantity(1),reward(0),allowanceSeconds(604800) {}
MissionBoard::MissionBoard()
    : originLocationId(),originCanonicalId(),simulationDay(-1),generation(0),offers() {}

Result MissionBoardStore::generate(const std::string& originStorageId,
        const std::string& originCanonicalId,long long day,unsigned long generation,
        const std::string& seed,const std::vector<MissionCandidate>& candidates,
        MissionBoard& board) const {
    if(originStorageId.empty()||originCanonicalId.empty()||day<0||seed.empty())
        return Result::failure("MISSION_BOARD_IDENTITY","Storage origin, canonical origin, non-negative day and seed are required.");
    std::vector<MissionCandidate> eligible;
    for(std::size_t i=0;i<candidates.size();++i) {
        const MissionCandidate& c=candidates[i];
        if(c.cargoId.empty()||c.destinationLocationId.empty()||c.destinationCanonicalId.empty()||
           c.destinationCanonicalId==originCanonicalId||c.quantity<=0||c.reward<=0) continue;
        eligible.push_back(c);
    }
    std::sort(eligible.begin(),eligible.end(),[](const MissionCandidate& a,const MissionCandidate& b) {
        if(a.destinationCanonicalId!=b.destinationCanonicalId)
            return a.destinationCanonicalId<b.destinationCanonicalId;
        if(a.cargoId!=b.cargoId) return a.cargoId<b.cargoId;
        return a.quantity<b.quantity;
    });
    MissionBoard next;
    next.originLocationId=originStorageId;
    next.originCanonicalId=originCanonicalId;
    next.simulationDay=day;
    next.generation=generation;
    if(!eligible.empty()) {
        const std::size_t desired=(std::size_t)(score(seed+"|count")%4UL);
        std::set<std::size_t> used;
        for(std::size_t n=0;n<desired&&used.size()<eligible.size();++n) {
            std::size_t best=eligible.size(); unsigned long bestScore=0;
            for(std::size_t i=0;i<eligible.size();++i) if(!used.count(i)) {
                const unsigned long candidateScore=score(seed+"|"+eligible[i].destinationCanonicalId+
                    "|"+eligible[i].cargoId+"|"+std::to_string(i));
                if(best==eligible.size()||candidateScore<bestScore){best=i;bestScore=candidateScore;}
            }
            if(best==eligible.size()) break;
            used.insert(best);
            const MissionCandidate& c=eligible[best];
            DeliveryContract offer;
            offer.contractId="OEMMISSION-"+std::to_string(score(originCanonicalId))+"-"+
                std::to_string(day)+"-"+std::to_string(generation)+"-"+std::to_string(n);
            offer.cargoId=c.cargoId;
            offer.quantity=c.quantity;
            offer.originLocationId=originStorageId;
            offer.destinationLocationId=c.destinationLocationId;
            offer.destinationBodyId=c.destinationBodyId;
            offer.reward=c.reward;
            offer.acceptedAt=day*86400;
            offer.deadline=offer.acceptedAt+(c.allowanceSeconds>0?c.allowanceSeconds:604800);
            next.offers.push_back(offer);
        }
    }
    board=next;
    return Result::success();
}

Result MissionBoardStore::save(const std::string& path,
        const std::map<std::string,MissionBoard>& boards) const {
    if(path.empty()) return Result::failure("MISSION_BOARD_PATH","Mission board path is empty.");
    const std::string temp=path+".tmp";
    std::ofstream out(temp.c_str(),std::ios::binary|std::ios::trunc);
    if(!out) return Result::failure("MISSION_BOARD_WRITE","Could not create temporary mission board file.");
    out<<kHeader<<"\n";
    for(std::map<std::string,MissionBoard>::const_iterator it=boards.begin();it!=boards.end();++it)
        if(!writeBoard(out,it->second)) {
            out.close(); std::remove(temp.c_str());
            return Result::failure("MISSION_BOARD_WRITE","Could not write mission board record.");
        }
    out.close();
    if(!out) {std::remove(temp.c_str());return Result::failure("MISSION_BOARD_WRITE","Could not finish mission board file.");}
#ifdef _WIN32
    if(!MoveFileExA(temp.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)) {
        std::remove(temp.c_str());
        return Result::failure("MISSION_BOARD_RENAME","Could not atomically replace mission board file.");
    }
#else
    if(std::rename(temp.c_str(),path.c_str())!=0) {
        std::remove(temp.c_str());
        return Result::failure("MISSION_BOARD_RENAME","Could not atomically replace mission board file.");
    }
#endif
    return Result::success();
}

Result MissionBoardStore::load(const std::string& path,
        std::map<std::string,MissionBoard>& boards) const {
    std::ifstream in(path.c_str(),std::ios::binary);
    if(!in) return Result::failure("MISSION_BOARD_READ","Could not open mission board file.");
    std::string line;
    if(!std::getline(in,line)) return Result::failure("MISSION_BOARD_HEADER","Mission board header/version is invalid.");
    const bool versionOne=line=="OEM_MISSION_BOARDS|1";
    const bool versionTwo=line==kHeader;
    if(!versionOne&&!versionTwo) return Result::failure("MISSION_BOARD_HEADER","Mission board header/version is invalid.");
    std::map<std::string,MissionBoard> parsed;
    while(std::getline(in,line)) {
        if(!line.empty()&&line[line.size()-1]=='\r') line.erase(line.size()-1);
        if(line.empty()) continue;
        std::vector<std::string> f=split(line);
        if(f.empty()||f[0]!="B"||(versionOne&&f.size()!=5)||((versionTwo||versionThree)&&f.size()!=6))
            return Result::failure("MISSION_BOARD_CORRUPT","Mission board header record is malformed.");
        MissionBoard b; long long day=0,generation=0,count=0;
        if(versionOne) {
            if(!unhex(f[1],b.originLocationId)||b.originLocationId.empty()||
               !number(f[2],day)||day<0||!number(f[3],generation)||generation<0||
               !number(f[4],count)||count<0||count>3)
                return Result::failure("MISSION_BOARD_CORRUPT","Legacy mission board metadata is invalid.");
            b.originCanonicalId="legacy:"+b.originLocationId;
        } else {
            if(!unhex(f[1],b.originCanonicalId)||b.originCanonicalId.empty()||
               !unhex(f[2],b.originLocationId)||b.originLocationId.empty()||
               !number(f[3],day)||day<0||!number(f[4],generation)||generation<0||
               !number(f[5],count)||count<0||count>3)
                return Result::failure("MISSION_BOARD_CORRUPT","Mission board metadata is invalid.");
        }
        b.simulationDay=day; b.generation=(unsigned long)generation;
        for(long long i=0;i<count;++i) {
            if(!std::getline(in,line)) return Result::failure("MISSION_BOARD_CORRUPT","Mission board offer is missing.");
            if(!line.empty()&&line[line.size()-1]=='\r') line.erase(line.size()-1);
            f=split(line); DeliveryContract c;
            long long qty=0,reward=0,accepted=0,deadline=0;
            if((versionThree?f.size()!=14:f.size()!=11)||f[0]!="O"||!unhex(f[1],c.contractId)||!unhex(f[2],c.vesselId)||
               !unhex(f[3],c.cargoId)||!number(f[4],qty)||qty<=0||!unhex(f[5],c.originLocationId)||
               !unhex(f[6],c.destinationLocationId)||!unhex(f[7],c.destinationBodyId)||
               !number(f[8],reward)||reward<=0||!number(f[9],accepted)||!number(f[10],deadline)||
               c.contractId.empty()||c.cargoId.empty()||c.originLocationId!=b.originLocationId||
               c.destinationLocationId.empty()||c.destinationLocationId==c.originLocationId)
                return Result::failure("MISSION_BOARD_CORRUPT","Mission board offer is invalid.");
            if(!versionThree) { c.missionType="DELIVERY"; c.missionTitle="Cargo delivery"; c.missionBriefing="Deliver the contracted cargo to the named destination."; }\n            c.quantity=qty; c.reward=reward; c.acceptedAt=accepted; c.deadline=deadline;
            b.offers.push_back(c);
        }
        if(!parsed.insert(std::make_pair(b.originCanonicalId,b)).second)
            return Result::failure("MISSION_BOARD_DUPLICATE","Mission board contains duplicate canonical origins.");
    }
    if(in.bad()) return Result::failure("MISSION_BOARD_READ","Error reading mission board file.");
    boards.swap(parsed);
    return Result::success();
}
}
