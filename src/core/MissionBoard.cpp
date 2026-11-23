#include "MissionBoard.h"
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <set>
#include <locale>

namespace oem {
namespace {
const char* kHeader = "OEM_MISSION_BOARDS|1";
std::string hex(const std::string& value) {
    static const char digits[] = "0123456789ABCDEF";
    std::string out;
    for (std::size_t i=0;i<value.size();++i) {
        const unsigned char c=(unsigned char)value[i];
        out.push_back(digits[c>>4]); out.push_back(digits[c&15]);
    }
    return out;
}
bool unhex(const std::string& value, std::string& out) {
    if (value.size()%2) return false;
    out.clear();
    for (std::size_t i=0;i<value.size();i+=2) {
        const char a=value[i], b=value[i+1];
        const int x=a>='0'&&a<='9'?a-'0':a>='A'&&a<='F'?a-'A'+10:a>='a'&&a<='f'?a-'a'+10:-1;
        const int y=b>='0'&&b<='9'?b-'0':b>='A'&&b<='F'?b-'0'+10:b>='a'&&b<='f'?b-'a'+10:-1;
        if(x<0||y<0) return false;
        out.push_back((char)((x<<4)|y));
    }
    return true;
}
std::vector<std::string> split(const std::string& line) {
    std::vector<std::string> fields; std::istringstream in(line); std::string item;
    while(std::getline(in,item,'|')) fields.push_back(item);
    return fields;
}
bool number(const std::string& s, long long& n) {
    if(s.empty()) return false; char* end=0; n=std::strtoll(s.c_str(),&end,10);
    return end && *end==0;
}
unsigned long score(const std::string& s) {
    unsigned long h=2166136261UL;
    for(std::size_t i=0;i<s.size();++i){h^=(unsigned char)s[i];h*=16777619UL;}
    return h;
}
bool writeBoard(std::ostream& out, const MissionBoard& b) {
    out<<"B|"<<hex(b.originLocationId)<<"|"<<b.simulationDay<<"|"<<b.generation<<"|"<<b.offers.size()<<"\n";
    for(std::size_t i=0;i<b.offers.size();++i){
        const DeliveryContract& c=b.offers[i];
        out<<"O|"<<hex(c.contractId)<<"|"<<hex(c.vesselId)<<"|"<<hex(c.cargoId)<<"|"<<c.quantity
           <<"|"<<hex(c.originLocationId)<<"|"<<hex(c.destinationLocationId)<<"|"<<hex(c.destinationBodyId)
           <<"|"<<c.reward<<"|"<<c.acceptedAt<<"|"<<c.deadline<<"\n";
    }
    return (bool)out;
}
}
MissionCandidate::MissionCandidate():cargoId(),destinationLocationId(),destinationBodyId(),quantity(1),reward(0){}
MissionBoard::MissionBoard():originLocationId(),simulationDay(-1),generation(0),offers(){}

Result MissionBoardStore::generate(const std::string& origin,long long day,unsigned long generation,
        const std::string& seed,const std::vector<MissionCandidate>& candidates,MissionBoard& board) const {
    if(origin.empty()||day<0||seed.empty())
        return Result::failure("MISSION_BOARD_IDENTITY","Origin, non-negative simulation day and generator seed are required.");
    std::vector<MissionCandidate> eligible;
    for(std::size_t i=0;i<candidates.size();++i){
        const MissionCandidate& c=candidates[i];
        if(c.cargoId.empty()||c.destinationLocationId.empty()||c.destinationLocationId==origin||
           c.quantity<=0||c.reward<=0) continue;
        eligible.push_back(c);
    }
    std::sort(eligible.begin(),eligible.end(),[](const MissionCandidate&a,const MissionCandidate&b){
        if(a.destinationLocationId!=b.destinationLocationId)return a.destinationLocationId<b.destinationLocationId;
        if(a.cargoId!=b.cargoId)return a.cargoId<b.cargoId;
        return a.quantity<b.quantity;
    });
    MissionBoard next; next.originLocationId=origin; next.simulationDay=day; next.generation=generation;
    if(!eligible.empty()){
        const unsigned long roll=score(seed+"|count");
        const std::size_t desired=(std::size_t)(roll%4UL); // zero to three missions; zero is intentional.
        std::set<std::size_t> used;
        for(std::size_t n=0;n<desired && used.size()<eligible.size();++n){
            std::size_t best=eligible.size(); unsigned long bestScore=0;
            for(std::size_t i=0;i<eligible.size();++i) if(!used.count(i)){
                const unsigned long candidateScore=score(seed+"|"+eligible[i].destinationLocationId+"|"+
                    eligible[i].cargoId+"|"+std::to_string(i));
                if(best==eligible.size()||candidateScore<bestScore){best=i;bestScore=candidateScore;}
            }
            if(best==eligible.size())break;
            used.insert(best);
            const MissionCandidate& c=eligible[best];
            DeliveryContract offer; offer.contractId="OEMMISSION-"+std::to_string(day)+"-"+
                std::to_string(generation)+"-"+std::to_string(n);
            offer.cargoId=c.cargoId; offer.quantity=c.quantity; offer.originLocationId=origin;
            offer.destinationLocationId=c.destinationLocationId; offer.destinationBodyId=c.destinationBodyId;
            offer.reward=c.reward; offer.acceptedAt=day*86400; offer.deadline=offer.acceptedAt+7*86400;
            next.offers.push_back(offer);
        }
    }
    board=next; return Result::success();
}

Result MissionBoardStore::save(const std::string& path,const std::map<std::string,MissionBoard>& boards) const {
    if(path.empty())return Result::failure("MISSION_BOARD_PATH","Mission board path is empty.");
    const std::string temp=path+".tmp";
    std::ofstream out(temp.c_str(),std::ios::binary|std::ios::trunc);
    if(!out)return Result::failure("MISSION_BOARD_WRITE","Could not create temporary mission board file.");
    out<<kHeader<<"\n";
    for(std::map<std::string,MissionBoard>::const_iterator it=boards.begin();it!=boards.end();++it)
        if(!writeBoard(out,it->second)){out.close();std::remove(temp.c_str());return Result::failure("MISSION_BOARD_WRITE","Could not write mission board record.");}
    out.close(); if(!out){std::remove(temp.c_str());return Result::failure("MISSION_BOARD_WRITE","Could not finish mission board file.");}
    std::remove(path.c_str());
    if(std::rename(temp.c_str(),path.c_str())!=0){std::remove(temp.c_str());return Result::failure("MISSION_BOARD_RENAME","Could not replace mission board file.");}
    return Result::success();
}

Result MissionBoardStore::load(const std::string& path,std::map<std::string,MissionBoard>& boards) const {
    std::ifstream in(path.c_str(),std::ios::binary);
    if(!in)return Result::failure("MISSION_BOARD_READ","Could not open mission board file.");
    std::string line; if(!std::getline(in,line)||line!=kHeader)
        return Result::failure("MISSION_BOARD_HEADER","Mission board header/version is invalid.");
    std::map<std::string,MissionBoard> parsed;
    while(std::getline(in,line)){
        if(!line.empty()&&line[line.size()-1]=='\r')line.erase(line.size()-1);
        if(line.empty())continue;
        std::vector<std::string> f=split(line);
        if(f.size()!=5||f[0]!="B")return Result::failure("MISSION_BOARD_CORRUPT","Mission board header record is malformed.");
        MissionBoard b; long long day=0,generation=0,count=0;
        if(!unhex(f[1],b.originLocationId)||b.originLocationId.empty()||!number(f[2],day)||day<0||
           !number(f[3],generation)||generation<0||!number(f[4],count)||count<0||count>3)
            return Result::failure("MISSION_BOARD_CORRUPT","Mission board metadata is invalid.");
        b.simulationDay=day;b.generation=(unsigned long)generation;
        for(long long i=0;i<count;++i){
            if(!std::getline(in,line))return Result::failure("MISSION_BOARD_CORRUPT","Mission board offer is missing.");
            if(!line.empty()&&line[line.size()-1]=='\r')line.erase(line.size()-1);
            f=split(line); DeliveryContract c; long long qty=0,reward=0,accepted=0,deadline=0;
            if(f.size()!=11||f[0]!="O"||!unhex(f[1],c.contractId)||!unhex(f[2],c.vesselId)||
               !unhex(f[3],c.cargoId)||!number(f[4],qty)||qty<=0||!unhex(f[5],c.originLocationId)||
               !unhex(f[6],c.destinationLocationId)||!unhex(f[7],c.destinationBodyId)||
               !number(f[8],reward)||reward<=0||!number(f[9],accepted)||!number(f[10],deadline)||
               c.contractId.empty()||c.cargoId.empty()||c.originLocationId!=b.originLocationId||
               c.destinationLocationId.empty()||c.destinationLocationId==c.originLocationId)
                return Result::failure("MISSION_BOARD_CORRUPT","Mission board offer is invalid.");
            c.quantity=qty;c.reward=reward;c.acceptedAt=accepted;c.deadline=deadline;
            b.offers.push_back(c);
        }
        if(!parsed.insert(std::make_pair(b.originLocationId,b)).second)
            return Result::failure("MISSION_BOARD_DUPLICATE","Mission board contains duplicate origins.");
    }
    if(in.bad())return Result::failure("MISSION_BOARD_READ","Error reading mission board file.");
    boards.swap(parsed); return Result::success();
}
}
