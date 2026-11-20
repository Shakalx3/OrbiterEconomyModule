#include "SimulationEventJournal.h"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#else
#include <cstdio>
#endif
namespace oem { namespace {
const char* HEADER_V1="OEM_EVENT_JOURNAL|1";
const char* HEADER_V2="OEM_EVENT_JOURNAL|2";
std::string encodeHex(const std::string& v) {
    static const char d[]="0123456789ABCDEF"; std::string o; o.reserve(v.size()*2);
    for(std::size_t i=0;i<v.size();++i){unsigned char c=(unsigned char)v[i];o.push_back(d[c>>4]);o.push_back(d[c&15]);}
    return o;
}
bool decodeHex(const std::string& v,std::string& o) {
    if(v.size()&1u)return false;o.clear();o.reserve(v.size()/2);
    for(std::size_t i=0;i<v.size();i+=2){
        char a=v[i],b=v[i+1];
        int h=a>='0'&&a<='9'?a-'0':a>='A'&&a<='F'?a-'A'+10:a>='a'&&a<='f'?a-'a'+10:-1;
        int l=b>='0'&&b<='9'?b-'0':b>='A'&&b<='F'?b-'A'+10:b>='a'&&b<='f'?b-'a'+10:-1;
        if(h<0||l<0)return false;o.push_back((char)((h<<4)|l));
    }return true;
}
std::vector<std::string> splitFields(const std::string& line) {
    std::vector<std::string> f;std::istringstream in(line);std::string x;
    while(std::getline(in,x,'|'))f.push_back(x);return f;
}
bool parseLL(const std::string& s,long long& v) {
    if(s.empty())return false;char* e=0;long long n=std::strtoll(s.c_str(),&e,10);
    if(!e||*e!='\0')return false;v=n;return true;
}
bool parseD(const std::string& s,double& v) {
    if(s.empty())return false;char* e=0;double n=std::strtod(s.c_str(),&e);
    if(!e||*e!='\0')return false;v=n;return true;
}
bool parseEvent(const std::string& line,int version,SimulationEvent& e) {
    const std::vector<std::string> f=splitFields(line);
    if(f.empty()||f[0]!="E"||(version==1&&f.size()!=10)||(version==2&&f.size()!=12))return false;
    long long type=0,t=0,ms=0,known=0,port=-1;
    if(!decodeHex(f[1],e.eventId)||!parseLL(f[2],type)||type<SIM_EVENT_TAKEOFF||
       type>SIM_EVENT_UNDOCK||!decodeHex(f[3],e.vesselId)||!decodeHex(f[4],e.bodyId)||
       !decodeHex(f[5],e.baseId)||!parseLL(f[6],t)||!parseLL(f[7],ms)||
       !parseD(f[8],e.distanceMetres)||!parseLL(f[9],known)||(known!=0&&known!=1))return false;
    if(version==2&&(!decodeHex(f[10],e.relatedVesselId)||!parseLL(f[11],port)||
                    port< -1||port>2147483647LL))return false;
    e.type=(SimulationEventType)type;e.simulationTime=t;e.simulationTimeMilliseconds=ms;
    e.locationKnown=known!=0;e.dockingPortIndex=(int)port;
    return !e.eventId.empty()&&!e.vesselId.empty();
}
void writeEvent(std::ostream& o,const SimulationEvent& e) {
    o<<"E|"<<encodeHex(e.eventId)<<'|'<<(int)e.type<<'|'<<encodeHex(e.vesselId)<<'|'
     <<encodeHex(e.bodyId)<<'|'<<encodeHex(e.baseId)<<'|'<<e.simulationTime<<'|'
     <<e.simulationTimeMilliseconds<<'|'<<std::setprecision(17)<<e.distanceMetres<<'|'
     <<(e.locationKnown?1:0)<<'|'<<encodeHex(e.relatedVesselId)<<'|'<<e.dockingPortIndex<<'\n';
}
bool replaceFile(const std::string& a,const std::string& b) {
#ifdef _WIN32
    return MoveFileExA(a.c_str(),b.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
#else
    return std::rename(a.c_str(),b.c_str())==0;
#endif
}
}
Result SimulationEventJournalStore::load(const std::string& path,SimulationEventQueue& queue) const {
    std::ifstream in(path.c_str(),std::ios::in|std::ios::binary);if(!in.is_open())return Result::success();
    std::string header;if(!std::getline(in,header))return Result::failure("JOURNAL_EMPTY","Journal file is empty.");
    int version=header==HEADER_V1?1:header==HEADER_V2?2:0;
    if(!version)return Result::failure("JOURNAL_VERSION","Journal header or version is not supported.");
    std::string line;while(std::getline(in,line)){if(line.empty())continue;SimulationEvent e;
        if(!parseEvent(line,version,e)){if(in.eof())break;return Result::failure("JOURNAL_CORRUPT","A complete journal record is invalid.");}
        Result p=queue.publish(e);if(!p.ok)return Result::failure(p.code,p.message);
    }return Result::success();
}
Result SimulationEventJournalStore::save(const std::string& path,const SimulationEventQueue& queue) const {
    const std::string tmp=path+".tmp";
    {std::ofstream out(tmp.c_str(),std::ios::out|std::ios::binary|std::ios::trunc);
     if(!out.is_open())return Result::failure("JOURNAL_OPEN_FAILED","Could not create temporary journal file.");
     out<<HEADER_V2<<'\n';const std::vector<SimulationEvent>& es=queue.events();
     for(std::size_t i=0;i<es.size();++i)writeEvent(out,es[i]);out.flush();
     if(!out.good())return Result::failure("JOURNAL_WRITE_FAILED","Could not write complete journal.");
     out.close();if(out.fail())return Result::failure("JOURNAL_CLOSE_FAILED","Could not close journal file.");}
    if(!replaceFile(tmp,path)){std::remove(tmp.c_str());return Result::failure("JOURNAL_REPLACE_FAILED","Could not atomically replace journal file.");}
    return Result::success();
}
}
