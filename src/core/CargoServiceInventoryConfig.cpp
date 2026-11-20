#include "CargoServiceInventoryConfig.h"
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#endif
namespace oem { namespace {
std::string encodeHex(const std::string& s){static const char d[]="0123456789ABCDEF";std::string o;for(std::size_t i=0;i<s.size();++i){unsigned char c=(unsigned char)s[i];o.push_back(d[c>>4]);o.push_back(d[c&15]);}return o;}
bool decodeHex(const std::string& s,std::string& o){if(s.empty()||s.size()%2)return false;o.clear();for(std::size_t i=0;i<s.size();i+=2){char a=s[i],b=s[i+1];int h=a>='0'&&a<='9'?a-'0':a>='A'&&a<='F'?a-'A'+10:a>='a'&&a<='f'?a-'a'+10:-1;int l=b>='0'&&b<='9'?b-'0':b>='A'&&b<='F'?b-'A'+10:b>='a'&&b<='f'?b-'a'+10:-1;if(h<0||l<0)return false;o.push_back((char)((h<<4)|l));}return !o.empty();}
bool parseLong(const std::string& s,long long& v){if(s.empty())return false;char* e=0;long long n=std::strtoll(s.c_str(),&e,10);if(!e||*e!='\0')return false;v=n;return true;}
bool parseDouble(const std::string& s,double& v){if(s.empty())return false;char* e=0;double n=std::strtod(s.c_str(),&e);if(!e||*e!='\0'||!std::isfinite(n))return false;v=n;return true;}
bool replaceFile(const std::string& a,const std::string& b){
#ifdef _WIN32
return MoveFileExA(a.c_str(),b.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
#else
return std::rename(a.c_str(),b.c_str())==0;
#endif
}
}
Result CargoServiceInventoryConfigStore::load(const std::string& path,const std::string& serviceId,CargoServiceInventory& inventory) const {
    if(serviceId.empty()||inventory.serviceVesselId()!=serviceId)return Result::failure("CARGO_STOCK_ID","Expected service ID must match the target inventory.");
    std::ifstream in(path.c_str(),std::ios::binary);if(!in.is_open()){CargoServiceInventory empty(serviceId);inventory=empty;return Result::success();}
    std::string raw((std::istreambuf_iterator<char>(in)),std::istreambuf_iterator<char>()),normalized;
    if(raw.size()>=2&&(unsigned char)raw[0]==0xFF&&(unsigned char)raw[1]==0xFE){
        if((raw.size()-2)%2)return Result::failure("CARGO_STOCK_ENCODING","Invalid UTF-16LE stock configuration.");
        for(std::size_t i=2;i<raw.size();i+=2){unsigned char lo=(unsigned char)raw[i],hi=(unsigned char)raw[i+1];if(hi!=0)return Result::failure("CARGO_STOCK_ENCODING","Stock configuration must contain ASCII-compatible text.");normalized.push_back((char)lo);}
    }else{normalized=raw;if(normalized.size()>=3&&(unsigned char)normalized[0]==0xEF&&(unsigned char)normalized[1]==0xBB&&(unsigned char)normalized[2]==0xBF)normalized.erase(0,3);}
    std::istringstream text(normalized);std::string line;if(!std::getline(text,line))return Result::failure("CARGO_STOCK_HEADER","Cargo stock configuration is empty.");
    if(!line.empty()&&line[line.size()-1]=='\r')line.erase(line.size()-1);
    if(line!="OEM_CARGO_STOCK|1|"+encodeHex(serviceId))return Result::failure("CARGO_STOCK_HEADER","Cargo stock configuration version or service ID does not match.");
    CargoServiceInventory candidate(serviceId);std::map<std::string,bool> seen;
    while(std::getline(text,line)){if(!line.empty()&&line[line.size()-1]=='\r')line.erase(line.size()-1);if(line.empty())continue;
        std::istringstream row(line);std::string tag,cargoHex,qtyText,massText,extra;
        if(!std::getline(row,tag,'|')||!std::getline(row,cargoHex,'|')||!std::getline(row,qtyText,'|')||!std::getline(row,massText,'|')||std::getline(row,extra,'|')||tag!="S")
            return Result::failure("CARGO_STOCK_RECORD","Cargo stock configuration contains an invalid record.");
        std::string cargo;long long qty=0;double mass=0.0;
        if(!decodeHex(cargoHex,cargo)||seen.find(cargo)!=seen.end()||!parseLong(qtyText,qty)||qty<0||!parseDouble(massText,mass)||!(mass>0.0))
            return Result::failure("CARGO_STOCK_RECORD","Cargo stock line has an invalid ID, duplicate cargo, quantity, or unit mass.");
        Result added=candidate.setInitialStock(cargo,qty,mass);if(!added.ok)return added;seen[cargo]=true;
    }
    inventory=candidate;return Result::success();
}
Result CargoServiceInventoryConfigStore::save(const std::string& path,const CargoServiceInventory& inventory) const {
    if(inventory.serviceVesselId().empty()||!inventory.history().empty())return Result::failure("CARGO_STOCK_ACTIVE","Only a named service inventory before transfers can be saved as initial stock configuration.");
    const std::string temp=path+".tmp";
    {std::ofstream out(temp.c_str(),std::ios::binary|std::ios::trunc);if(!out.is_open())return Result::failure("CARGO_STOCK_OPEN","Could not create temporary cargo stock configuration.");
     out<<"OEM_CARGO_STOCK|1|"<<encodeHex(inventory.serviceVesselId())<<"\n";
     for(std::size_t i=0;i<inventory.lines().size();++i){const CargoStockLine& l=inventory.lines()[i];out<<"S|"<<encodeHex(l.cargoId)<<'|'<<l.quantity<<'|'<<std::setprecision(17)<<l.unitMassKg<<"\n";}
     out.flush();if(!out.good())return Result::failure("CARGO_STOCK_WRITE","Could not write cargo stock configuration.");out.close();if(out.fail())return Result::failure("CARGO_STOCK_CLOSE","Could not close cargo stock configuration.");}
    if(!replaceFile(temp,path)){std::remove(temp.c_str());return Result::failure("CARGO_STOCK_REPLACE","Could not atomically replace cargo stock configuration.");}
    return Result::success();
}
}
