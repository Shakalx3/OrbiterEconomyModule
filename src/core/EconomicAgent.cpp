#include "EconomicAgent.h"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#ifdef _WIN32
#include <windows.h>
#endif

namespace oem { namespace {
std::string hexEncode(const std::string& value) {
    static const char digits[]="0123456789ABCDEF";
    std::string out;
    for (std::size_t i=0;i<value.size();++i) {
        const unsigned char c=static_cast<unsigned char>(value[i]);
        out.push_back(digits[c>>4]); out.push_back(digits[c&15]);
    }
    return out;
}
bool hexDecode(const std::string& value,std::string& out) {
    if (value.empty() || value.size()%2) return false;
    out.clear();
    for (std::size_t i=0;i<value.size();i+=2) {
        const char a=value[i],b=value[i+1];
        const int hi=a>='0'&&a<='9'?a-'0':a>='A'&&a<='F'?a-'A'+10:a>='a'&&a<='f'?a-'a'+10:-1;
        const int lo=b>='0'&&b<='9'?b-'0':b>='A'&&b<='F'?b-'A'+10:b>='a'&&b<='f'?b-'a'+10:-1;
        if (hi<0||lo<0) return false;
        out.push_back(static_cast<char>((hi<<4)|lo));
    }
    return !out.empty();
}
bool parseLong(const std::string& s,long long& v) {
    if(s.empty())return false;char* e=0;const long long n=std::strtoll(s.c_str(),&e,10);
    if(!e||*e!='\\0')return false;v=n;return true;
}
bool replaceFile(const std::string& from,const std::string& to) {
#ifdef _WIN32
    return MoveFileExA(from.c_str(),to.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
#else
    return std::rename(from.c_str(),to.c_str())==0;
#endif
}
std::vector<std::string> split(const std::string& line) {
    std::vector<std::string> fields;std::istringstream in(line);std::string item;
    while(std::getline(in,item,'|'))fields.push_back(item);
    return fields;
}
} // namespace

EconomicAgent::EconomicAgent()
    : agentId(),displayName(),role(ECONOMIC_AGENT_SUPPLIER),homeLocationId(),
      operatingLocationIds(),openingBalance(0),currentBalance(0),
      tradeIntervalSeconds(86400),nextTradeAt(0),inventory(),buyTargets(),sellTargets() {}

Result EconomicAgentRegistry::add(const EconomicAgent& a) {
    if(a.agentId.empty()||a.displayName.empty()||a.homeLocationId.empty())
        return Result::failure("AGENT_IDENTITY","Agent ID, display name and home location are required.");
    if(a.role<ECONOMIC_AGENT_SUPPLIER||a.role>ECONOMIC_AGENT_COMPETITOR)
        return Result::failure("AGENT_ROLE","Unknown economic agent role.");
    if(a.openingBalance<0||a.currentBalance<0||a.tradeIntervalSeconds<=0||a.nextTradeAt<0)
        return Result::failure("AGENT_STATE","Agent balances and schedule are invalid.");
    if(indexes_.find(a.agentId)!=indexes_.end())
        return Result::failure("AGENT_DUPLICATE","Economic agent ID already exists.");
    for(std::map<std::string,long long>::const_iterator i=a.inventory.begin();i!=a.inventory.end();++i)
        if(i->first.empty()||i->second<0)return Result::failure("AGENT_INVENTORY","Agent inventory contains an invalid line.");
    for(std::map<std::string,long long>::const_iterator i=a.buyTargets.begin();i!=a.buyTargets.end();++i)
        if(i->first.empty()||i->second<=0)return Result::failure("AGENT_BUY_TARGET","Agent buy targets must be positive.");
    for(std::map<std::string,long long>::const_iterator i=a.sellTargets.begin();i!=a.sellTargets.end();++i)
        if(i->first.empty()||i->second<=0)return Result::failure("AGENT_SELL_TARGET","Agent sell targets must be positive.");
    entries_.push_back(a);indexes_[a.agentId]=entries_.size()-1;return Result::success();
}
bool EconomicAgentRegistry::find(const std::string& id,EconomicAgent& a) const {
    std::map<std::string,std::size_t>::const_iterator i=indexes_.find(id);
    if(i==indexes_.end())return false;a=entries_[i->second];return true;
}
Result EconomicAgentRegistry::updateState(const EconomicAgent& a) {
    std::map<std::string,std::size_t>::iterator i=indexes_.find(a.agentId);
    if(i==indexes_.end())return Result::failure("AGENT_NOT_FOUND","Economic agent is not registered.");
    if(a.agentId.empty()||a.displayName.empty()||a.homeLocationId.empty()||
       a.openingBalance<0||a.currentBalance<0||a.tradeIntervalSeconds<=0||a.nextTradeAt<0)
        return Result::failure("AGENT_STATE","Updated agent state is invalid.");
    for(std::map<std::string,long long>::const_iterator n=a.inventory.begin();n!=a.inventory.end();++n)
        if(n->first.empty()||n->second<0)return Result::failure("AGENT_INVENTORY","Agent inventory contains an invalid line.");
    entries_[i->second]=a;return Result::success();
}
const std::vector<EconomicAgent>& EconomicAgentRegistry::entries() const{return entries_;}
void EconomicAgentRegistry::clear(){indexes_.clear();entries_.clear();}

Result EconomicAgentConfigStore::load(const std::string& path,EconomicAgentRegistry& registry) const {
    if(path.empty())return Result::failure("AGENT_CONFIG_PATH","Economic agent config path is empty.");
    std::ifstream in(path.c_str(),std::ios::binary);
    if(!in.is_open())return Result::failure("AGENT_CONFIG_MISSING","Economic agent config does not exist.");
    std::string header;if(!std::getline(in,header))return Result::failure("AGENT_CONFIG_HEADER","Economic agent config is empty.");
    if(!header.empty()&&header[header.size()-1]=='\\r')header.erase(header.size()-1);
    if(header!="OEM_ECONOMIC_AGENTS|1")return Result::failure("AGENT_CONFIG_VERSION","Unsupported economic agent config version.");
    EconomicAgentRegistry parsed;std::string line;std::map<std::string,bool> seen;
    while(std::getline(in,line)) {
        if(!line.empty()&&line[line.size()-1]=='\\r')line.erase(line.size()-1);
        if(line.empty())continue;const std::vector<std::string> f=split(line);
        if(f.size()!=9||f[0]!="A")return Result::failure("AGENT_CONFIG_RECORD","Invalid economic agent record.");
        EconomicAgent a;long long role=0,balance=0,interval=0,next=0;
        if(!hexDecode(f[1],a.agentId)||!hexDecode(f[2],a.displayName)||
           !parseLong(f[3],role)||!hexDecode(f[4],a.homeLocationId)||
           !parseLong(f[5],balance)||!parseLong(f[6],interval)||!parseLong(f[7],next)||
           !parseLong(f[8],a.currentBalance))
            return Result::failure("AGENT_CONFIG_RECORD","Invalid identity, role, balance or schedule.");
        a.role=static_cast<EconomicAgentRole>(role);a.openingBalance=balance;
        a.tradeIntervalSeconds=interval;a.nextTradeAt=next;
        Result added=parsed.add(a);if(!added.ok)return added;
    }
    registry=parsed;return Result::success();
}
Result EconomicAgentConfigStore::save(const std::string& path,const EconomicAgentRegistry& registry) const {
    if(path.empty())return Result::failure("AGENT_CONFIG_PATH","Economic agent config path is empty.");
    const std::string temp=path+".tmp";
    {std::ofstream out(temp.c_str(),std::ios::binary|std::ios::trunc);
     if(!out.is_open())return Result::failure("AGENT_CONFIG_OPEN","Could not create temporary agent config.");
     out<<"OEM_ECONOMIC_AGENTS|1\\n";
     for(std::size_t i=0;i<registry.entries().size();++i){const EconomicAgent& a=registry.entries()[i];
        if(a.agentId.empty()||a.displayName.empty()||a.homeLocationId.empty()||a.openingBalance<0||
           a.currentBalance<0||a.tradeIntervalSeconds<=0||a.nextTradeAt<0)
            return Result::failure("AGENT_CONFIG_STATE","Cannot save invalid economic agent state.");
        out<<"A|"<<hexEncode(a.agentId)<<"|"<<hexEncode(a.displayName)<<"|"<<(int)a.role<<"|"
           <<hexEncode(a.homeLocationId)<<"|"<<a.openingBalance<<"|"<<a.tradeIntervalSeconds<<"|"
           <<a.nextTradeAt<<"|"<<a.currentBalance<<"\\n";
     }
     out.flush();if(!out.good())return Result::failure("AGENT_CONFIG_WRITE","Could not write economic agent config.");
     out.close();if(out.fail())return Result::failure("AGENT_CONFIG_CLOSE","Could not close economic agent config.");}
    if(!replaceFile(temp,path)){std::remove(temp.c_str());return Result::failure("AGENT_CONFIG_REPLACE","Could not atomically replace economic agent config.");}
    return Result::success();
}

} // namespace oem
