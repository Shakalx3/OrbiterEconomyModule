#include "SpaceportRegistry.h"
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <map>
#include <iterator>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#endif
namespace oem { namespace {
std::string hexEncode(const std::string& s){static const char d[]="0123456789ABCDEF";std::string o;for(std::size_t i=0;i<s.size();++i){unsigned char c=(unsigned char)s[i];o.push_back(d[c>>4]);o.push_back(d[c&15]);}return o;}
bool hexDecode(const std::string& s,std::string& o){if(s.empty()||s.size()%2)return false;o.clear();for(std::size_t i=0;i<s.size();i+=2){char a=s[i],b=s[i+1];int h=a>='0'&&a<='9'?a-'0':a>='A'&&a<='F'?a-'A'+10:a>='a'&&a<='f'?a-'a'+10:-1;int l=b>='0'&&b<='9'?b-'0':b>='A'&&b<='F'?b-'A'+10:b>='a'&&b<='f'?b-'a'+10:-1;if(h<0||l<0)return false;o.push_back((char)((h<<4)|l));}return !o.empty();}
bool parseDouble(const std::string& s,double& v){if(s.empty())return false;char* e=0;double n=std::strtod(s.c_str(),&e);if(!e||*e!='\0'||!std::isfinite(n))return false;v=n;return true;}
bool parseUnsigned(const std::string& s,unsigned int& v){if(s.empty()||s[0]=='-')return false;char* e=0;unsigned long n=std::strtoul(s.c_str(),&e,10);if(!e||*e!='\0'||n>0xFFFFFFFFUL)return false;v=(unsigned int)n;return true;}
bool replaceFile(const std::string& a,const std::string& b){
#ifdef _WIN32
return MoveFileExA(a.c_str(),b.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
#else
return std::rename(a.c_str(),b.c_str())==0;
#endif
}
bool validPad(const SpaceportPadRecord& p){return std::isfinite(p.longitudeRad)&&std::isfinite(p.latitudeRad)&&std::isfinite(p.radiusMetres)&&p.radiusMetres>0.0&&p.latitudeRad>=-1.5707963267948966&&p.latitudeRad<=1.5707963267948966;}
bool validRecord(const SpaceportRecord& r){
 if(r.id.empty()||r.bodyName.empty()||r.baseName.empty()||r.id!=r.bodyName+"/"+r.baseName||
 !(r.bodyRadiusMetres>0.0)||!(r.bodyMassKg>0.0)||!std::isfinite(r.longitudeRad)||!std::isfinite(r.latitudeRad)||
 !std::isfinite(r.bodyRadiusMetres)||!std::isfinite(r.bodyMassKg)||!std::isfinite(r.surfaceGravityMps2)||
 !std::isfinite(r.escapeVelocityMps)||r.latitudeRad < -1.5707963267948966||r.latitudeRad>1.5707963267948966)return false;
 for(std::size_t i=0;i<r.pads.size();++i){if(!validPad(r.pads[i]))return false;for(std::size_t j=0;j<i;++j)if(r.pads[j].index==r.pads[i].index)return false;}
 return true;
}
}
SpaceportPadRecord::SpaceportPadRecord():index(0),longitudeRad(0),latitudeRad(0),radiusMetres(0),hasIls(false){}
SpaceportRecord::SpaceportRecord():longitudeRad(0),latitudeRad(0),bodyRadiusMetres(0),bodyMassKg(0),surfaceGravityMps2(0),escapeVelocityMps(0){}
SpaceportRegistry::SpaceportRegistry():entries_(){}
Result SpaceportRegistry::add(const SpaceportRecord& r){SpaceportRecord candidate=r;candidate.pads.clear();if(!validRecord(candidate))return Result::failure("SPACEPORT_RECORD","Spaceport record contains invalid identity, coordinates or physical characteristics.");if(find(r.id))return Result::failure("SPACEPORT_DUPLICATE","Duplicate body/base identifier.");entries_.push_back(candidate);return Result::success();}
Result SpaceportRegistry::addPad(const std::string& id,const SpaceportPadRecord& p){if(!validPad(p))return Result::failure("SPACEPORT_PAD","Landing-pad record contains invalid coordinates.");for(std::size_t i=0;i<entries_.size();++i)if(entries_[i].id==id){for(std::size_t j=0;j<entries_[i].pads.size();++j)if(entries_[i].pads[j].index==p.index)return Result::failure("SPACEPORT_PAD_DUPLICATE","Duplicate landing-pad index.");entries_[i].pads.push_back(p);return Result::success();}return Result::failure("SPACEPORT_UNKNOWN","Landing pad references an unknown spaceport.");}
void SpaceportRegistry::clear(){entries_.clear();}
const std::vector<SpaceportRecord>& SpaceportRegistry::entries()const{return entries_;}
const SpaceportRecord* SpaceportRegistry::find(const std::string& id)const{for(std::size_t i=0;i<entries_.size();++i)if(entries_[i].id==id)return &entries_[i];return 0;}
Result SpaceportRegistryConfigStore::save(const std::string& path,const SpaceportRegistry& registry)const{
 const std::string temp=path+".tmp";{std::ofstream out(temp.c_str(),std::ios::binary|std::ios::trunc);if(!out.is_open())return Result::failure("SPACEPORT_REGISTRY_OPEN","Could not create temporary registry.");out<<"OEM_SPACEPORT_REGISTRY|1\n";
 for(std::size_t i=0;i<registry.entries().size();++i){const SpaceportRecord& r=registry.entries()[i];if(!validRecord(r))return Result::failure("SPACEPORT_REGISTRY_RECORD","Registry contains an invalid record.");out<<"B|"<<hexEncode(r.id)<<"|"<<hexEncode(r.bodyName)<<"|"<<hexEncode(r.baseName)<<"|"<<std::setprecision(17)<<r.longitudeRad<<"|"<<r.latitudeRad<<"|"<<r.bodyRadiusMetres<<"|"<<r.bodyMassKg<<"|"<<r.surfaceGravityMps2<<"|"<<r.escapeVelocityMps<<"|"<<r.pads.size()<<"\n";for(std::size_t p=0;p<r.pads.size();++p){const SpaceportPadRecord& x=r.pads[p];out<<"P|"<<hexEncode(r.id)<<"|"<<x.index<<"|"<<x.longitudeRad<<"|"<<x.latitudeRad<<"|"<<x.radiusMetres<<"|"<<(x.hasIls?1:0)<<"\n";}}
 out.flush();if(!out.good())return Result::failure("SPACEPORT_REGISTRY_WRITE","Could not write registry.");out.close();if(out.fail())return Result::failure("SPACEPORT_REGISTRY_CLOSE","Could not close registry.");}
 if(!replaceFile(temp,path)){std::remove(temp.c_str());return Result::failure("SPACEPORT_REGISTRY_REPLACE","Could not atomically replace registry.");}return Result::success();
}
Result SpaceportRegistryConfigStore::load(const std::string& path,SpaceportRegistry& registry)const{
 std::ifstream in(path.c_str(),std::ios::binary);if(!in.is_open())return Result::failure("SPACEPORT_REGISTRY_MISSING","Registry file is missing.");std::string raw((std::istreambuf_iterator<char>(in)),std::istreambuf_iterator<char>());
 if(raw.size()>=2&&(unsigned char)raw[0]==0xFF&&(unsigned char)raw[1]==0xFE){if((raw.size()-2)%2)return Result::failure("SPACEPORT_REGISTRY_ENCODING","Invalid UTF-16LE registry.");std::string a;for(std::size_t i=2;i<raw.size();i+=2){if(raw[i+1]!=0)return Result::failure("SPACEPORT_REGISTRY_ENCODING","Registry must be ASCII-compatible.");a.push_back(raw[i]);}raw=a;}else if(raw.size()>=3&&(unsigned char)raw[0]==0xEF&&(unsigned char)raw[1]==0xBB&&(unsigned char)raw[2]==0xBF)raw.erase(0,3);
 std::istringstream input(raw);std::string line;if(!std::getline(input,line))return Result::failure("SPACEPORT_REGISTRY_HEADER","Registry is empty.");if(!line.empty()&&line[line.size()-1]=='\r')line.erase(line.size()-1);if(line!="OEM_SPACEPORT_REGISTRY|1")return Result::failure("SPACEPORT_REGISTRY_HEADER","Registry header or version is invalid.");
 SpaceportRegistry candidate;std::map<std::string,unsigned int> expected;while(std::getline(input,line)){if(!line.empty()&&line[line.size()-1]=='\r')line.erase(line.size()-1);if(line.empty())continue;std::vector<std::string> f;std::istringstream row(line);std::string part;while(std::getline(row,part,'|'))f.push_back(part);
 if(f[0]=="B"){if(f.size()!=11)return Result::failure("SPACEPORT_REGISTRY_RECORD","Invalid base record field count.");SpaceportRecord r;unsigned int pads=0;if(!hexDecode(f[1],r.id)||!hexDecode(f[2],r.bodyName)||!hexDecode(f[3],r.baseName)||!parseDouble(f[4],r.longitudeRad)||!parseDouble(f[5],r.latitudeRad)||!parseDouble(f[6],r.bodyRadiusMetres)||!parseDouble(f[7],r.bodyMassKg)||!parseDouble(f[8],r.surfaceGravityMps2)||!parseDouble(f[9],r.escapeVelocityMps)||!parseUnsigned(f[10],pads))return Result::failure("SPACEPORT_REGISTRY_RECORD","Invalid base record values.");Result a=candidate.add(r);if(!a.ok)return a;expected[r.id]=pads;
 }else if(f[0]=="P"){if(f.size()!=7)return Result::failure("SPACEPORT_REGISTRY_PAD","Invalid pad record field count.");std::string id;unsigned int index=0,ils=0;double lng=0,lat=0,rad=0;if(!hexDecode(f[1],id)||!parseUnsigned(f[2],index)||!parseDouble(f[3],lng)||!parseDouble(f[4],lat)||!parseDouble(f[5],rad)||!parseUnsigned(f[6],ils)||ils>1)return Result::failure("SPACEPORT_REGISTRY_PAD","Invalid pad record values.");SpaceportPadRecord p;p.index=index;p.longitudeRad=lng;p.latitudeRad=lat;p.radiusMetres=rad;p.hasIls=ils!=0;Result a=candidate.addPad(id,p);if(!a.ok)return a;
 }else return Result::failure("SPACEPORT_REGISTRY_RECORD","Unknown registry record type.");}
 for(std::map<std::string,unsigned int>::const_iterator i=expected.begin();i!=expected.end();++i){const SpaceportRecord* r=candidate.find(i->first);if(!r||r->pads.size()!=i->second)return Result::failure("SPACEPORT_REGISTRY_PAD_COUNT","Pad count does not match base record.");}
 registry=candidate;return Result::success();
}
}
