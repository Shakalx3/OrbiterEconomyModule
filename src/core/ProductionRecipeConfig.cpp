#include "ProductionRecipeConfig.h"
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
std::string hex(const std::string&s){static const char d[]="0123456789ABCDEF";std::string o;for(std::size_t i=0;i<s.size();++i){unsigned char c=(unsigned char)s[i];o+=d[c>>4];o+=d[c&15];}return o;}
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
Result ProductionRecipeConfigStore::load(const std::string&path,ProductionRecipeBook&book)const{
 std::ifstream in(path.c_str(),std::ios::binary);if(!in)return Result::failure("PRODUCTION_CONFIG_MISSING","Production recipe configuration does not exist.");
 std::string line;if(!std::getline(in,line))return Result::failure("PRODUCTION_CONFIG_HEADER","Production recipe configuration is empty.");if(!line.empty()&&line.back()=='\r')line.pop_back();
 if(line!="OEM_PRODUCTION_RECIPES|1")return Result::failure("PRODUCTION_CONFIG_HEADER","Production recipe configuration version is invalid.");
 ProductionRecipeBook candidate;
 while(std::getline(in,line)){
  if(!line.empty()&&line.back()=='\r')line.pop_back();if(line.empty())continue;
  std::vector<std::string> f;std::istringstream row(line);std::string v;while(std::getline(row,v,'|'))f.push_back(v);
  if(f.size()<9||f[0]!="R")return Result::failure("PRODUCTION_CONFIG_RECORD","Production recipe record has an invalid shape.");
  ProductionRecipe r;long long cycle=0,last=0,enabled=0,inputCount=0,outputCount=0;
  if(!unhex(f[1],r.recipeId)||!unhex(f[2],r.locationId)||!integer(f[3],cycle)||!integer(f[4],last)||!integer(f[5],enabled)||!integer(f[6],inputCount)||cycle<=0||last<0||(enabled!=0&&enabled!=1)||inputCount<=0)
   return Result::failure("PRODUCTION_CONFIG_RECORD","Production recipe record contains invalid header values.");
  r.cycleSeconds=cycle;r.lastProductionTime=last;r.enabled=enabled!=0;std::size_t pos=7;
  for(long long i=0;i<inputCount;++i){if(pos+2>=f.size())return Result::failure("PRODUCTION_CONFIG_RECORD","Production recipe input list is truncated.");ProductionMaterial m;if(!unhex(f[pos],m.cargoId)||!integer(f[pos+1],m.quantity)||!real(f[pos+2],m.unitMassKg))return Result::failure("PRODUCTION_CONFIG_RECORD","Production recipe input is invalid.");r.inputs.push_back(m);pos+=3;}
  if(pos>=f.size()||!integer(f[pos],outputCount)||outputCount<=0)return Result::failure("PRODUCTION_CONFIG_RECORD","Production recipe output count is invalid.");++pos;
  for(long long i=0;i<outputCount;++i){if(pos+2>=f.size())return Result::failure("PRODUCTION_CONFIG_RECORD","Production recipe output list is truncated.");ProductionMaterial m;if(!unhex(f[pos],m.cargoId)||!integer(f[pos+1],m.quantity)||!real(f[pos+2],m.unitMassKg))return Result::failure("PRODUCTION_CONFIG_RECORD","Production recipe output is invalid.");r.outputs.push_back(m);pos+=3;}
  if(pos!=f.size())return Result::failure("PRODUCTION_CONFIG_RECORD","Unexpected trailing production recipe fields.");
  Result added=candidate.add(r);if(!added.ok)return added;
 }
 if(!in.eof())return Result::failure("PRODUCTION_CONFIG_READ","Could not read production recipe configuration.");
 book=candidate;return Result::success();
}
Result ProductionRecipeConfigStore::save(const std::string&path,const ProductionRecipeBook&book)const{
 if(path.empty())return Result::failure("PRODUCTION_CONFIG_PATH","Production recipe configuration path is empty.");
 const std::string temp=path+".tmp";
 {std::ofstream out(temp.c_str(),std::ios::binary|std::ios::trunc);if(!out)return Result::failure("PRODUCTION_CONFIG_OPEN","Could not create temporary production recipe configuration.");out.imbue(std::locale::classic());out<<"OEM_PRODUCTION_RECIPES|1\n";
  const std::vector<ProductionRecipe>& recipes=book.recipes();
  for(std::size_t i=0;i<recipes.size();++i){const ProductionRecipe&r=recipes[i];out<<"R|"<<hex(r.recipeId)<<'|'<<hex(r.locationId)<<'|'<<r.cycleSeconds<<'|'<<r.lastProductionTime<<'|'<<(r.enabled?1:0)<<'|'<<r.inputs.size();for(std::size_t j=0;j<r.inputs.size();++j)out<<'|'<<hex(r.inputs[j].cargoId)<<'|'<<r.inputs[j].quantity<<'|'<<std::setprecision(17)<<r.inputs[j].unitMassKg;out<<'|'<<r.outputs.size();for(std::size_t j=0;j<r.outputs.size();++j)out<<'|'<<hex(r.outputs[j].cargoId)<<'|'<<r.outputs[j].quantity<<'|'<<std::setprecision(17)<<r.outputs[j].unitMassKg;out<<"\n";}
  out.flush();if(!out.good())return Result::failure("PRODUCTION_CONFIG_WRITE","Could not write production recipe configuration.");out.close();if(out.fail())return Result::failure("PRODUCTION_CONFIG_CLOSE","Could not close production recipe configuration.");
 }
 if(!replace(temp,path)){std::remove(temp.c_str());return Result::failure("PRODUCTION_CONFIG_REPLACE","Could not atomically replace production recipe configuration.");}
 return Result::success();
}
}
