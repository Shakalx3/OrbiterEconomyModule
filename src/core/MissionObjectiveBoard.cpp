#include "MissionObjectiveBoard.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <set>
#ifdef _WIN32
#include <windows.h>
#endif

namespace oem {
namespace {
const char* kHeader="OEM_OBJECTIVE_BOARDS|1";
std::string hex(const std::string& value) {
    if(value.empty()) return "-";
    static const char digits[]="0123456789ABCDEF";
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
        const int hi=hexDigit(value[i]),lo=hexDigit(value[i+1]);
        if(hi<0||lo<0) return false;
        out.push_back((char)((hi<<4)|lo));
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
bool valid(const MissionObjectiveCandidate& c,const std::string& origin) {
    if((c.kind!=MISSION_OBJECTIVE_VISIT_SPACEPORT&&c.kind!=MISSION_OBJECTIVE_DOCK_STATION)||
       c.targetLocationId.empty()||c.targetCanonicalId.empty()||
       c.targetCanonicalId==origin||c.missionType.empty()||c.missionTitle.empty()||
       c.missionBriefing.empty()||c.reward<=0||c.allowanceSeconds<=0) return false;
    if(c.kind==MISSION_OBJECTIVE_VISIT_SPACEPORT&&c.targetBodyId.empty()) return false;
    return true;
}
bool writeBoard(std::ostream& out,const MissionObjectiveBoard& b) {
    out<<"B|"<<hex(b.originCanonicalId)<<"|"<<hex(b.originLocationId)<<"|"
       <<b.simulationDay<<"|"<<b.generation<<"|"<<b.offers.size()<<"\n";
    for(std::size_t i=0;i<b.offers.size();++i) {
        const MissionObjectiveOffer& o=b.offers[i];
        out<<"O|"<<hex(o.missionId)<<"|"<<(int)o.kind<<"|"<<hex(o.targetLocationId)
           <<"|"<<hex(o.targetBodyId)<<"|"<<hex(o.targetCanonicalId)
           <<"|"<<hex(o.missionType)<<"|"<<hex(o.missionTitle)<<"|"<<hex(o.missionBriefing)
           <<"|"<<o.reward<<"|"<<o.allowanceSeconds<<"\n";
    }
    return (bool)out;
}
}
MissionObjectiveCandidate::MissionObjectiveCandidate()
    : kind(MISSION_OBJECTIVE_VISIT_SPACEPORT),targetLocationId(),targetBodyId(),
      targetCanonicalId(),missionType(),missionTitle(),missionBriefing(),
      reward(0),allowanceSeconds(604800) {}
MissionObjectiveOffer::MissionObjectiveOffer()
    : missionId(),kind(MISSION_OBJECTIVE_VISIT_SPACEPORT),targetLocationId(),targetBodyId(),
      targetCanonicalId(),missionType(),missionTitle(),missionBriefing(),
      reward(0),allowanceSeconds(604800) {}
MissionObjectiveBoard::MissionObjectiveBoard()
    : originLocationId(),originCanonicalId(),simulationDay(-1),generation(0),offers() {}

Result MissionObjectiveBoardStore::generate(const std::string& originLocationId,
        const std::string& originCanonicalId,long long day,unsigned long generation,
        const std::string& seed,const std::vector<MissionObjectiveCandidate>& candidates,
        MissionObjectiveBoard& board) const {
    if(originLocationId.empty()||originCanonicalId.empty()||day<0||seed.empty())
        return Result::failure("OBJECTIVE_BOARD_IDENTITY","Origin IDs, non-negative day and seed are required.");
    std::vector<MissionObjectiveCandidate> eligible;
    for(std::size_t i=0;i<candidates.size();++i)
        if(valid(candidates[i],originCanonicalId)) eligible.push_back(candidates[i]);
    std::sort(eligible.begin(),eligible.end(),[](const MissionObjectiveCandidate& a,const MissionObjectiveCandidate& b) {
        if(a.targetCanonicalId!=b.targetCanonicalId) return a.targetCanonicalId<b.targetCanonicalId;
        if(a.kind!=b.kind) return a.kind<b.kind;
        return a.missionType<b.missionType;
    });
    MissionObjectiveBoard next;
    next.originLocationId=originLocationId; next.originCanonicalId=originCanonicalId;
    next.simulationDay=day; next.generation=generation;
    if(!eligible.empty()) {
        const std::size_t desired=(std::size_t)(score(seed+"|count")%4UL);
        std::set<std::size_t> used;
        for(std::size_t n=0;n<desired&&used.size()<eligible.size();++n) {
            std::size_t best=eligible.size(); unsigned long bestScore=0;
            for(std::size_t i=0;i<eligible.size();++i) if(!used.count(i)) {
                const MissionObjectiveCandidate& candidate=eligible[i];
                const unsigned long value=score(seed+"|"+candidate.targetCanonicalId+"|"+
                    std::to_string((int)candidate.kind)+"|"+candidate.missionType);
                if(best==eligible.size()||value<bestScore){best=i;bestScore=value;}
            }
            if(best==eligible.size()) break;
            used.insert(best);
            const MissionObjectiveCandidate& candidate=eligible[best];
            MissionObjectiveOffer offer;
            offer.missionId="OEMOBJ-"+std::to_string(score(seed+"|"+originCanonicalId+"|"+
                std::to_string(day)+"|"+std::to_string(generation)))+"-"+std::to_string(n);
            offer.kind=candidate.kind;
            offer.targetLocationId=candidate.targetLocationId;
            offer.targetBodyId=candidate.targetBodyId;
            offer.targetCanonicalId=candidate.targetCanonicalId;
            offer.missionType=candidate.missionType;
            offer.missionTitle=candidate.missionTitle;
            offer.missionBriefing=candidate.missionBriefing;
            offer.reward=candidate.reward;
            offer.allowanceSeconds=candidate.allowanceSeconds;
            next.offers.push_back(offer);
        }
    }
    board=next;
    return Result::success();
}

Result MissionObjectiveBoardStore::save(const std::string& path,
        const std::map<std::string,MissionObjectiveBoard>& boards) const {
    if(path.empty()) return Result::failure("OBJECTIVE_BOARD_PATH","Objective mission board path is empty.");
    const std::string temp=path+".tmp";
    std::ofstream out(temp.c_str(),std::ios::binary|std::ios::trunc);
    if(!out) return Result::failure("OBJECTIVE_BOARD_WRITE","Could not create temporary objective board file.");
    out<<kHeader<<"\n";
    for(std::map<std::string,MissionObjectiveBoard>::const_iterator it=boards.begin();it!=boards.end();++it)
        if(!writeBoard(out,it->second)) {
            out.close(); std::remove(temp.c_str());
            return Result::failure("OBJECTIVE_BOARD_WRITE","Could not write objective board record.");
        }
    out.close();
    if(!out) {std::remove(temp.c_str());return Result::failure("OBJECTIVE_BOARD_WRITE","Could not finish objective board file.");}
#ifdef _WIN32
    if(!MoveFileExA(temp.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)) {
        std::remove(temp.c_str());
        return Result::failure("OBJECTIVE_BOARD_RENAME","Could not atomically replace objective board file.");
    }
#else
    if(std::rename(temp.c_str(),path.c_str())!=0) {
        std::remove(temp.c_str());
        return Result::failure("OBJECTIVE_BOARD_RENAME","Could not atomically replace objective board file.");
    }
#endif
    return Result::success();
}

Result MissionObjectiveBoardStore::load(const std::string& path,
        std::map<std::string,MissionObjectiveBoard>& boards) const {
    std::ifstream in(path.c_str(),std::ios::binary);
    if(!in) return Result::failure("OBJECTIVE_BOARD_READ","Could not open objective board file.");
    std::string line;
    if(!std::getline(in,line)||line!=kHeader)
        return Result::failure("OBJECTIVE_BOARD_HEADER","Objective board header/version is invalid.");
    std::map<std::string,MissionObjectiveBoard> parsed;
    while(std::getline(in,line)) {
        if(!line.empty()&&line[line.size()-1]=='\r') line.erase(line.size()-1);
        if(line.empty()) continue;
        std::vector<std::string> f=split(line);
        if(f.size()!=6||f[0]!="B")
            return Result::failure("OBJECTIVE_BOARD_CORRUPT","Objective board header record is malformed.");
        MissionObjectiveBoard board; long long day=0,generation=0,count=0;
        if(!unhex(f[1],board.originCanonicalId)||board.originCanonicalId.empty()||
           !unhex(f[2],board.originLocationId)||board.originLocationId.empty()||
           !number(f[3],day)||day<0||!number(f[4],generation)||generation<0||
           !number(f[5],count)||count<0||count>3)
            return Result::failure("OBJECTIVE_BOARD_CORRUPT","Objective board metadata is invalid.");
        board.simulationDay=day; board.generation=(unsigned long)generation;
        std::set<std::string> ids;
        for(long long i=0;i<count;++i) {
            if(!std::getline(in,line)) return Result::failure("OBJECTIVE_BOARD_CORRUPT","Objective board offer is missing.");
            if(!line.empty()&&line[line.size()-1]=='\r') line.erase(line.size()-1);
            f=split(line); MissionObjectiveOffer offer; long long kind=0,reward=0,allowance=0;
            if(f.size()!=11||f[0]!="O"||!unhex(f[1],offer.missionId)||offer.missionId.empty()||
               !number(f[2],kind)||(kind!=MISSION_OBJECTIVE_VISIT_SPACEPORT&&kind!=MISSION_OBJECTIVE_DOCK_STATION)||
               !unhex(f[3],offer.targetLocationId)||!unhex(f[4],offer.targetBodyId)||
               !unhex(f[5],offer.targetCanonicalId)||!unhex(f[6],offer.missionType)||
               !unhex(f[7],offer.missionTitle)||!unhex(f[8],offer.missionBriefing)||
               !number(f[9],reward)||reward<=0||!number(f[10],allowance)||allowance<=0)
                return Result::failure("OBJECTIVE_BOARD_CORRUPT","Objective board offer is invalid.");
            offer.kind=(MissionObjectiveKind)kind; offer.reward=reward; offer.allowanceSeconds=allowance;
            MissionObjectiveCandidate candidate; candidate.kind=offer.kind;
            candidate.targetLocationId=offer.targetLocationId; candidate.targetBodyId=offer.targetBodyId;
            candidate.targetCanonicalId=offer.targetCanonicalId; candidate.missionType=offer.missionType;
            candidate.missionTitle=offer.missionTitle; candidate.missionBriefing=offer.missionBriefing;
            candidate.reward=offer.reward; candidate.allowanceSeconds=offer.allowanceSeconds;
            if(!valid(candidate,board.originCanonicalId)||!ids.insert(offer.missionId).second)
                return Result::failure("OBJECTIVE_BOARD_CORRUPT","Objective board contains an invalid or duplicate offer.");
            board.offers.push_back(offer);
        }
        if(!parsed.insert(std::make_pair(board.originCanonicalId,board)).second)
            return Result::failure("OBJECTIVE_BOARD_DUPLICATE","Objective board contains duplicate canonical origins.");
    }
    if(in.bad()) return Result::failure("OBJECTIVE_BOARD_READ","Error reading objective board file.");
    boards.swap(parsed);
    return Result::success();
}
} // namespace oem
