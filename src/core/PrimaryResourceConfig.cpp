#include "PrimaryResourceConfig.h"
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <locale>
#include <sstream>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#endif
namespace oem { namespace {
std::string hex(const std::string& s){static const char d[]="0123456789ABCDEF";std::string o;for(std::size_t i=0;i<s.size();++i){unsigned char c=(unsigned char)s[i];o+=d[c>>4];o+=d[c&15];}return o;}
bool unhex(const std::string&s,std::string&o){if(s.empty()||s.size()%2)return false;o.clear();for(std::size_t i=0;i<s.size();i+=2){char a=s[i],b=s[i+1];int x=a>='0'&&a<='9'?a-'0':a>='A'&&a<='F'?a-'A'+10:-1;int y=b>='0'&&b<='9'?b-'0':b>='A'&&b<='F'?b-'A'+10:-1;if(x<0||y<0)return false;o+=(char)((x<<4)|y);}return !o.empty();}
bool integer(const std::string&s,long long&v){if(s.empty())return false;char*e=0;long long n=std::strtoll(s.c_str(),&e,10);if(!e||*e)return false;v=n;return true;}
bool real(const std::string&s,double&v){if(s.empty())return false;std::istringstream in(s);in.imbue(std::locale::classic());double n=0;in>>n;if(!in||in.peek()!=std::char_traits<char>::eof()||!std::isfinite(n))return false;v=n;return true;}
bool replace(const std::string&a,const std::string&b){
#ifdef _WIN32
return MoveFileExA(a.c_str(),b.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
#else
return std::rename(a.c_str(),b.c_str())==0;
#endif
}
}
Result PrimaryResourceConfigStore::load(const std::string&path,PrimaryResourceBook&book)const{
 std::ifstream in(path.c_str(),std::ios::binary);if(!in)return Result::failure("PRIMARY_CONFIG_MISSING","Primary resource configuration does not exist.");
 std::string line;if(!std::getline(in,line))return Result::failure("PRIMARY_CONFIG_HEADER","Primary resource configuration is empty.");if(!line.empty()&&line.back()=='\r')line.pop_back();
 const bool version1=line=="OEM_PRIMARY_RESOURCES|1";
 const bool version2=line=="OEM_PRIMARY_RESOURCES|2";
 if(!version1&&!version2)return Result::failure("PRIMARY_CONFIG_HEADER","Primary resource configuration version is invalid.");
 PrimaryResourceBook candidate;
 while(std::getline(in,line)){if(!line.empty()&&line.back()=='\r')line.pop_back();if(line.empty())continue;
  std::vector<std::string> f;std::istringstream row(line);std::string v;while(std::getline(row,v,'|'))f.push_back(v);
  if((version1&&f.size()!=10)||(version2&&f.size()!=11)||f[0]!="S")return Result::failure("PRIMARY_CONFIG_RECORD","Primary resource source record has an invalid shape.");
  PrimaryResourceSource s;long long units=0,cycle=0,cap=0,last=0,enabled=0,opening=0;
  if(!unhex(f[1],s.sourceId)||!unhex(f[2],s.locationId)||!unhex(f[3],s.cargoId)||!real(f[4],s.unitMassKg)||!integer(f[5],units)||!integer(f[6],cycle)||!integer(f[7],cap)||!integer(f[8],last)||!integer(f[9],enabled)||(enabled!=0&&enabled!=1)||(version2&&!integer(f[10],opening)))
   return Result::failure("PRIMARY_CONFIG_RECORD","Primary resource source contains invalid values.");
  s.unitsPerCycle=units;s.cycleSeconds=cycle;s.maximumStock=cap;s.lastProductionTime=last;s.enabled=enabled!=0;s.openingStock=opening;
  Result added=candidate.add(s);if(!added.ok)return added;
 }
 if(!in.eof())return Result::failure("PRIMARY_CONFIG_READ","Could not read primary resource configuration.");
 book=candidate;return Result::success();
}
Result PrimaryResourceConfigStore::save(const std::string&path,const PrimaryResourceBook&book)const{
 if(path.empty())return Result::failure("PRIMARY_CONFIG_PATH","Primary resource configuration path is empty.");
 const std::string temp=path+".tmp";
 {std::ofstream out(temp.c_str(),std::ios::binary|std::ios::trunc);if(!out)return Result::failure("PRIMARY_CONFIG_OPEN","Could not create temporary primary resource configuration.");
  out<<"OEM_PRIMARY_RESOURCES|2\n";const std::vector<PrimaryResourceSource>&v=book.sources();
  for(std::size_t i=0;i<v.size();++i){const PrimaryResourceSource&s=v[i];out<<"S|"<<hex(s.sourceId)<<'|'<<hex(s.locationId)<<'|'<<hex(s.cargoId)<<'|'<<std::setprecision(17)<<s.unitMassKg<<'|'<<s.unitsPerCycle<<'|'<<s.cycleSeconds<<'|'<<s.maximumStock<<'|'<<s.lastProductionTime<<'|'<<(s.enabled?1:0)<<"\n";}
  out.flush();if(!out.good())return Result::failure("PRIMARY_CONFIG_WRITE","Could not write primary resource configuration.");out.close();if(out.fail())return Result::failure("PRIMARY_CONFIG_CLOSE","Could not close primary resource configuration.");
 }
 if(!replace(temp,path)){std::remove(temp.c_str());return Result::failure("PRIMARY_CONFIG_REPLACE","Could not atomically replace primary resource configuration.");}
 return Result::success();
}
}
