#include "CargoServiceRegistry.h"
#include <cstdio>
#include <fstream>
#include <sstream>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#endif

namespace oem { namespace {
std::string encodeHex(const std::string& value) {
    static const char digits[]="0123456789ABCDEF";
    std::string out;
    for(std::size_t i=0;i<value.size();++i) {
        const unsigned char c=static_cast<unsigned char>(value[i]);
        out.push_back(digits[c>>4]);out.push_back(digits[c&15]);
    }
    return out;
}
bool decodeHex(const std::string& value,std::string& out) {
    if(value.empty()||value.size()%2)return false;
    out.clear();
    for(std::size_t i=0;i<value.size();i+=2) {
        const char a=value[i],b=value[i+1];
        const int hi=a>='0'&&a<='9'?a-'0':a>='A'&&a<='F'?a-'A'+10:a>='a'&&a<='f'?a-'a'+10:-1;
        const int lo=b>='0'&&b<='9'?b-'0':b>='A'&&b<='F'?b-'A'+10:b>='a'&&b<='f'?b-'a'+10:-1;
        if(hi<0||lo<0)return false;
        out.push_back(static_cast<char>((hi<<4)|lo));
    }
    return !out.empty();
}
bool replaceFile(const std::string& temporary,const std::string& target) {
#ifdef _WIN32
    return MoveFileExA(temporary.c_str(),target.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
#else
    return std::rename(temporary.c_str(),target.c_str())==0;
#endif
}
} // namespace

bool CargoServiceRegistry::registerService(const std::string& vesselId) {
    if(vesselId.empty())return false;
    return vesselIds_.insert(vesselId).second;
}
bool CargoServiceRegistry::unregisterService(const std::string& vesselId) {
    return vesselIds_.erase(vesselId)!=0;
}
bool CargoServiceRegistry::isRegistered(const std::string& vesselId) const {
    return !vesselId.empty()&&vesselIds_.find(vesselId)!=vesselIds_.end();
}
std::size_t CargoServiceRegistry::size() const { return vesselIds_.size(); }
const std::set<std::string>& CargoServiceRegistry::vesselIds() const { return vesselIds_; }
void CargoServiceRegistry::clear() { vesselIds_.clear(); }

Result CargoServiceRegistry::load(const std::string& path) {
    std::ifstream in(path.c_str(),std::ios::in|std::ios::binary);
    if(!in.is_open()) { vesselIds_.clear(); return Result::success(); }
    // Read the whole file so common Windows text encodings can be normalized
    // before strict version/record validation.
    std::string raw((std::istreambuf_iterator<char>(in)),std::istreambuf_iterator<char>());
    std::string normalized;
    if(raw.size()>=2 && static_cast<unsigned char>(raw[0])==0xFF &&
       static_cast<unsigned char>(raw[1])==0xFE) {
        // UTF-16LE (as produced by some versions of Windows Notepad).
        if((raw.size()-2)%2!=0)
            return Result::failure("CARGO_SERVICES_ENCODING","Cargo service registry has invalid UTF-16LE data.");
        for(std::size_t i=2;i<raw.size();i+=2) {
            const unsigned char lo=static_cast<unsigned char>(raw[i]);
            const unsigned char hi=static_cast<unsigned char>(raw[i+1]);
            if(hi!=0)
                return Result::failure("CARGO_SERVICES_ENCODING","Cargo service registry must contain ASCII-compatible text.");
            normalized.push_back(static_cast<char>(lo));
        }
    } else {
        normalized=raw;
        if(normalized.size()>=3 &&
           static_cast<unsigned char>(normalized[0])==0xEF &&
           static_cast<unsigned char>(normalized[1])==0xBB &&
           static_cast<unsigned char>(normalized[2])==0xBF)
            normalized.erase(0,3); // UTF-8 BOM
    }
    std::istringstream text(normalized);
    std::string header;
    if(!std::getline(text,header))
        return Result::failure("CARGO_SERVICES_HEADER","Cargo service registry header or version is invalid.");
    if(!header.empty() && header[header.size()-1]=='\r') header.erase(header.size()-1);
    if(header!="OEM_CARGO_SERVICES|1")
        return Result::failure("CARGO_SERVICES_HEADER","Cargo service registry header or version is invalid (expected OEM_CARGO_SERVICES|1).");
    std::set<std::string> candidate;
    std::string line;
    while(std::getline(text,line)) {
        if(!line.empty() && line[line.size()-1]=='\r') line.erase(line.size()-1);
        if(line.empty())continue;
        if(line.compare(0,2,"S|")!=0)
            return Result::failure("CARGO_SERVICES_RECORD","Cargo service registry contains an invalid record.");
        std::string vesselId;
        if(!decodeHex(line.substr(2),vesselId))
            return Result::failure("CARGO_SERVICES_RECORD","Cargo service registry contains an invalid vessel ID.");
        candidate.insert(vesselId);
    }
    vesselIds_.swap(candidate);
    return Result::success();
}

Result CargoServiceRegistry::save(const std::string& path) const {
    const std::string temporary=path+".tmp";
    {
        std::ofstream out(temporary.c_str(),std::ios::out|std::ios::binary|std::ios::trunc);
        if(!out.is_open())return Result::failure("CARGO_SERVICES_OPEN","Could not create temporary cargo service registry.");
        out<<"OEM_CARGO_SERVICES|1\n";
        for(std::set<std::string>::const_iterator it=vesselIds_.begin();it!=vesselIds_.end();++it)
            out<<"S|"<<encodeHex(*it)<<"\n";
        out.flush();
        if(!out.good())return Result::failure("CARGO_SERVICES_WRITE","Could not write cargo service registry.");
        out.close();
        if(out.fail())return Result::failure("CARGO_SERVICES_CLOSE","Could not close cargo service registry.");
    }
    if(!replaceFile(temporary,path)) {
        std::remove(temporary.c_str());
        return Result::failure("CARGO_SERVICES_REPLACE","Could not atomically replace cargo service registry.");
    }
    return Result::success();
}

} // namespace oem
