#include "CargoCatalogConfig.h"
#include <cstdlib>
#include <cmath>
#include <fstream>
#include <sstream>
#include <vector>
namespace oem { namespace {
bool unhex(const std::string&s,std::string&o){if(s.empty()||s.size()%2)return false;o.clear();for(std::size_t i=0;i<s.size();i+=2){int a=s[i]>='0'&&s[i]<='9'?s[i]-'0':s[i]>='A'&&s[i]<='F'?s[i]-'A'+10:s[i]>='a'&&s[i]<='f'?s[i]-'a'+10:-1;int b=s[i+1]>='0'&&s[i+1]<='9'?s[i+1]-'0':s[i+1]>='A'&&s[i+1]<='F'?s[i+1]-'A'+10:s[i+1]>='a'&&s[i+1]<='f'?s[i+1]-'a'+10:-1;if(a<0||b<0)return false;o+=(char)((a<<4)|b);}return !o.empty();}
bool integer(const std::string&s,long long&v){if(s.empty())return false;char*e=0;long long n=std::strtoll(s.c_str(),&e,10);if(!e||*e)return false;v=n;return true;}
bool real(const std::string&s,double&v){if(s.empty())return false;char*e=0;double n=std::strtod(s.c_str(),&e);if(!e||*e||!std::isfinite(n))return false;v=n;return true;}
std::vector<std::string> split(const std::string&s){std::vector<std::string>v;std::istringstream in(s);std::string x;while(std::getline(in,x,'|'))v.push_back(x);return v;}
}
Result CargoCatalogConfigStore::load(const std::string&p,CargoCatalog&catalog)const{
 std::ifstream in(p.c_str(),std::ios::binary);if(!in)return Result::failure("CATALOG_CONFIG_MISSING","Cargo catalog configuration is missing; no cargo definitions or prices are created implicitly.");
 std::string line;if(!std::getline(in,line))return Result::failure("CATALOG_CONFIG_HEADER","Cargo catalog is empty.");if(!line.empty()&&line.back()=='\r')line.pop_back();if(line!="OEM_CARGO_CATALOG|1")return Result::failure("CATALOG_CONFIG_HEADER","Cargo catalog header or version is invalid.");
 CargoCatalog candidate;while(std::getline(in,line)){if(!line.empty()&&line.back()=='\r')line.pop_back();if(line.empty())continue;std::vector<std::string>f=split(line);CargoDefinition d;long long buy=0,sell=0;if(f.size()!=6||f[0]!="C"||!unhex(f[1],d.cargoId)||!unhex(f[2],d.displayName)||!real(f[3],d.unitMassKg)||!integer(f[4],buy)||!integer(f[5],sell))return Result::failure("CATALOG_CONFIG_RECORD","Cargo catalog contains an invalid record.");d.buyPrice=buy;d.sellPrice=sell;Result r=candidate.add(d);if(!r.ok)return Result::failure("CATALOG_CONFIG_RECORD",r.message);}
 catalog=candidate;return Result::success();
}
}
