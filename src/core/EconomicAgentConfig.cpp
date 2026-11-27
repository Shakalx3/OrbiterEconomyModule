#include "EconomicAgentConfig.h"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#endif
namespace oem { namespace {
std::string hex(const std::string&s){static const char d[]="0123456789ABCDEF";std::string o;for(std::size_t i=0;i<s.size();++i){unsigned char c=(unsigned char)s[i];o+=d[c>>4];o+=d[c&15];}return o;}
bool unhex(const std::string&s,std::string&o){if(s.empty()||s.size()%2)return false;o.clear();for(std::size_t i=0;i<s.size();i+=2){char a=s[i],b=s[i+1];int x=a>='0'&&a<='9'?a-'0':a>='A'&&a<='F'?a-'A'+10:-1;int y=b>='0'&&b<='9'?b-'0':b>='A'&&b<='F'?b-'A'+10:-1;if(x<0||y<0)return false;o+=(char)((x<<4)|y);}return !o.empty();}
bool integer(const std::string&s,long long&v){if(s.empty())return false;char*e=0;long long n=std::strtoll(s.c_str(),&e,10);if(!e||*e)return false;v=n;return true;}
std::vector<std::string> split(const std::string&s){std::vector<std::string>v;std::istringstream in(s);std::string x;while(std::getline(in,x,'|'))v.push_back(x);return v;}
bool replace(const std::string&a,const std::string&b){
#ifdef _WIN32
 return MoveFileExA(a.c_str(),b.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
#else
 return std::rename(a.c_str(),b.c_str())==0;
#endif
}
}
Result EconomicAgentConfigStore::load(const std::string&path,EconomicAgentBook&book)const{
 std::ifstream in(path.c_str(),std::ios::binary);if(!in)return Result::failure("AGENT_CONFIG_MISSING","Economic agent configuration is missing.");
 std::string line;if(!std::getline(in,line))return Result::failure("AGENT_CONFIG_HEADER","Economic agent configuration is empty.");if(!line.empty()&&line.back()=='\r')line.pop_back();if(line!="OEM_ECONOMIC_AGENTS|1")return Result::failure("AGENT_CONFIG_HEADER","Economic agent configuration version is invalid.");
 std::map<std::string,EconomicAgent> agents;
 while(std::getline(in,line)){
  if(!line.empty()&&line.back()=='\r')line.pop_back();if(line.empty())continue;std::vector<std::string>f=split(line);long long a=0,b=0,c=0,d=0,e=0;
  if(f.size()==10&&f[0]=="A"){
   EconomicAgent agent;if(!unhex(f[1],agent.agentId)||!unhex(f[2],agent.displayName)||!integer(f[3],a)||!integer(f[5],b)||!integer(f[6],c)||!integer(f[7],d)||!integer(f[8],e)||!integer(f[9],a))return Result::failure("AGENT_CONFIG_RECORD","Economic agent identity record is invalid.");
   // Parse role separately; the final integer above is enabled.
   long long role=0,enabled=0;if(!integer(f[3],role)||!integer(f[9],enabled)||role<AGENT_SUPPLIER||role>AGENT_COMPETITOR||(c!=0&&c!=1)||(enabled!=0&&enabled!=1)||d<=0||e<0||b<0)return Result::failure("AGENT_CONFIG_RECORD","Economic agent numeric fields are invalid.");
   agent.role=(EconomicAgentRole)role;if(!unhex(f[4],agent.homeLocationId))return Result::failure("AGENT_CONFIG_RECORD","Economic agent home location is invalid.");agent.balance=b;agent.unlimitedFunds=c!=0;agent.activityInterval=d;agent.lastActivityTime=e;agent.enabled=enabled!=0;
   if(agents.find(agent.agentId)!=agents.end())return Result::failure("AGENT_CONFIG_DUPLICATE","Economic agent ID is duplicated.");agents[agent.agentId]=agent;continue;
  }
  if(f.size()==3&&f[0]=="L"){
   std::string id,location;if(!unhex(f[1],id)||!unhex(f[2],location)||agents.find(id)==agents.end())return Result::failure("AGENT_CONFIG_LOCATION","Agent location record references an unknown agent or invalid location.");agents[id].operatingLocationIds.push_back(location);continue;
  }
  if(f.size()==6&&f[0]=="P"){
   std::string id;AgentCargoPolicy p;if(!unhex(f[1],id)||!unhex(f[2],p.cargoId)||!integer(f[3],a)||!integer(f[4],b)||!integer(f[5],c)||agents.find(id)==agents.end())return Result::failure("AGENT_CONFIG_POLICY","Agent cargo policy record is invalid.");p.targetQuantity=a;p.maximumBuyPrice=b;p.minimumSellPrice=c;agents[id].cargoPolicies.push_back(p);continue;
  }
  if(f.size()==6&&f[0]=="I"){
   std::string id,cargo,origin;if(!unhex(f[1],id)||!unhex(f[2],cargo)||!integer(f[3],a)||!integer(f[5],b)||agents.find(id)==agents.end()||a<0||b<0)return Result::failure("AGENT_CONFIG_INVENTORY","Agent opening inventory record is invalid.");if(f[4]!="-"&&!unhex(f[4],origin))return Result::failure("AGENT_CONFIG_INVENTORY","Agent cargo origin is invalid.");if(agents[id].inventory.find(cargo)!=agents[id].inventory.end())return Result::failure("AGENT_CONFIG_INVENTORY","Agent cargo inventory is duplicated.");agents[id].inventory[cargo]=a;if(!origin.empty())agents[id].cargoOriginLocation[cargo]=origin;agents[id].cargoUnitCost[cargo]=b;continue;
  }
  return Result::failure("AGENT_CONFIG_RECORD","Economic agent configuration contains an unknown or malformed record.");
 }
 if(!in.eof())return Result::failure("AGENT_CONFIG_READ","Could not read economic agent configuration.");
 EconomicAgentBook candidate;for(std::map<std::string,EconomicAgent>::const_iterator i=agents.begin();i!=agents.end();++i){Result added=candidate.add(i->second);if(!added.ok)return added;}
 book=candidate;return Result::success();
}
Result EconomicAgentConfigStore::save(const std::string&path,const EconomicAgentBook&book)const{
 if(path.empty())return Result::failure("AGENT_CONFIG_PATH","Economic agent configuration path is empty.");const std::string temp=path+".tmp";
 {std::ofstream out(temp.c_str(),std::ios::binary|std::ios::trunc);if(!out)return Result::failure("AGENT_CONFIG_OPEN","Could not create temporary economic agent configuration.");out<<"OEM_ECONOMIC_AGENTS|1\n";
  const std::vector<EconomicAgent>&agents=book.agents();for(std::size_t i=0;i<agents.size();++i){const EconomicAgent&a=agents[i];out<<"A|"<<hex(a.agentId)<<'|'<<hex(a.displayName)<<'|'<<(int)a.role<<'|'<<hex(a.homeLocationId)<<'|'<<a.balance<<'|'<<(a.unlimitedFunds?1:0)<<'|'<<a.activityInterval<<'|'<<a.lastActivityTime<<'|'<<(a.enabled?1:0)<<"\n";
   for(std::size_t j=0;j<a.operatingLocationIds.size();++j)out<<"L|"<<hex(a.agentId)<<'|'<<hex(a.operatingLocationIds[j])<<"\n";
   for(std::size_t j=0;j<a.cargoPolicies.size();++j){const AgentCargoPolicy&p=a.cargoPolicies[j];out<<"P|"<<hex(a.agentId)<<'|'<<hex(p.cargoId)<<'|'<<p.targetQuantity<<'|'<<p.maximumBuyPrice<<'|'<<p.minimumSellPrice<<"\n";}
   for(std::map<std::string,long long>::const_iterator j=a.inventory.begin();j!=a.inventory.end();++j){const std::string origin=a.cargoOriginLocation.count(j->first)?a.cargoOriginLocation.find(j->first)->second:std::string();const Credits cost=a.cargoUnitCost.count(j->first)?a.cargoUnitCost.find(j->first)->second:Credits(0);out<<"I|"<<hex(a.agentId)<<'|'<<hex(j->first)<<'|'<<j->second<<'|'<<(origin.empty()?std::string("-"):hex(origin))<<'|'<<cost<<"\n";}
  }
  out.flush();if(!out.good())return Result::failure("AGENT_CONFIG_WRITE","Could not write economic agent configuration.");out.close();if(out.fail())return Result::failure("AGENT_CONFIG_CLOSE","Could not close economic agent configuration.");
 }
 if(!replace(temp,path)){std::remove(temp.c_str());return Result::failure("AGENT_CONFIG_REPLACE","Could not atomically replace economic agent configuration.");}return Result::success();
}
}
