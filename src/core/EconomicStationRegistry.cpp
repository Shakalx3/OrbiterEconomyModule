#include "EconomicStationRegistry.h"
#include <cstdio>
#include <fstream>
#include <sstream>
#ifdef _WIN32
#include <windows.h>
#endif

namespace oem { namespace {
std::string encodeHex(const std::string& value) {
    static const char digits[] = "0123456789ABCDEF";
    std::string out;
    for (std::size_t i=0;i<value.size();++i) {
        const unsigned char c=static_cast<unsigned char>(value[i]);
        out.push_back(digits[c>>4]); out.push_back(digits[c&15]);
    }
    return out;
}
bool decodeHex(const std::string& value,std::string& out) {
    if (value.empty() || value.size()%2) return false;
    out.clear();
    for (std::size_t i=0;i<value.size();i+=2) {
        const char a=value[i],b=value[i+1];
        const int hi=a>='0'&&a<='9'?a-'0':a>='A'&&a<='F'?a-'A'+10:a>='a'&&a<='f'?a-'a'+10:-1;
        const int lo=b>='0'&&b<='9'?b-'0':b>='A'&&b<='F'?b-'A'+10:b>='a'&&b<='f'?b-'a'+10:-1;
        if (hi<0 || lo<0) return false;
        out.push_back(static_cast<char>((hi<<4)|lo));
    }
    return !out.empty();
}
bool replaceFile(const std::string& from,const std::string& to) {
#ifdef _WIN32
    return MoveFileExA(from.c_str(),to.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
#else
    return std::rename(from.c_str(),to.c_str())==0;
#endif
}
}

bool EconomicStationRegistry::registerStation(const std::string& id) {
    return !id.empty() && vesselIds_.insert(id).second;
}
bool EconomicStationRegistry::unregisterStation(const std::string& id) {
    return vesselIds_.erase(id)!=0;
}
bool EconomicStationRegistry::isStation(const std::string& id) const {
    return !id.empty() && vesselIds_.find(id)!=vesselIds_.end();
}
std::size_t EconomicStationRegistry::size() const { return vesselIds_.size(); }
const std::set<std::string>& EconomicStationRegistry::vesselIds() const { return vesselIds_; }
void EconomicStationRegistry::clear() { vesselIds_.clear(); }

Result EconomicStationRegistry::load(const std::string& path) {
    std::ifstream in(path.c_str(),std::ios::in|std::ios::binary);
    if (!in.is_open()) { vesselIds_.clear(); return Result::success(); }
    std::string raw((std::istreambuf_iterator<char>(in)),std::istreambuf_iterator<char>());
    std::string normalized;
    if (raw.size()>=2 && static_cast<unsigned char>(raw[0])==0xFF && static_cast<unsigned char>(raw[1])==0xFE) {
        if ((raw.size()-2)%2) return Result::failure("ECONOMIC_STATIONS_ENCODING","Invalid UTF-16LE station registry.");
        for (std::size_t i=2;i<raw.size();i+=2) {
            const unsigned char lo=static_cast<unsigned char>(raw[i]), hi=static_cast<unsigned char>(raw[i+1]);
            if (hi!=0) return Result::failure("ECONOMIC_STATIONS_ENCODING","Station registry must contain ASCII-compatible text.");
            normalized.push_back(static_cast<char>(lo));
        }
    } else {
        normalized=raw;
        if (normalized.size()>=3 && static_cast<unsigned char>(normalized[0])==0xEF &&
            static_cast<unsigned char>(normalized[1])==0xBB && static_cast<unsigned char>(normalized[2])==0xBF)
            normalized.erase(0,3);
    }
    std::istringstream text(normalized);
    std::string header;
    if (!std::getline(text,header)) return Result::failure("ECONOMIC_STATIONS_HEADER","Station registry is empty.");
    if (!header.empty() && header[header.size()-1]=='\r') header.erase(header.size()-1);
    if (header!="OEM_ECONOMIC_STATIONS|1") return Result::failure("ECONOMIC_STATIONS_HEADER","Unsupported station registry header or version.");
    std::set<std::string> candidate;
    std::string line;
    while (std::getline(text,line)) {
        if (!line.empty() && line[line.size()-1]=='\r') line.erase(line.size()-1);
        if (line.empty()) continue;
        if (line.compare(0,2,"S|")!=0) return Result::failure("ECONOMIC_STATIONS_RECORD","Invalid station registry record.");
        std::string id;
        if (!decodeHex(line.substr(2),id) || !candidate.insert(id).second)
            return Result::failure("ECONOMIC_STATIONS_RECORD","Invalid or duplicate station vessel ID.");
    }
    vesselIds_.swap(candidate);
    return Result::success();
}

Result EconomicStationRegistry::save(const std::string& path) const {
    const std::string temporary=path+".tmp";
    {
        std::ofstream out(temporary.c_str(),std::ios::out|std::ios::binary|std::ios::trunc);
        if (!out.is_open()) return Result::failure("ECONOMIC_STATIONS_OPEN","Could not create temporary station registry.");
        out<<"OEM_ECONOMIC_STATIONS|1\n";
        for (std::set<std::string>::const_iterator it=vesselIds_.begin();it!=vesselIds_.end();++it)
            out<<"S|"<<encodeHex(*it)<<"\n";
        out.flush();
        if (!out.good()) return Result::failure("ECONOMIC_STATIONS_WRITE","Could not write station registry.");
        out.close();
        if (out.fail()) return Result::failure("ECONOMIC_STATIONS_CLOSE","Could not close station registry.");
    }
    if (!replaceFile(temporary,path)) {
        std::remove(temporary.c_str());
        return Result::failure("ECONOMIC_STATIONS_REPLACE","Could not atomically replace station registry.");
    }
    return Result::success();
}

} // namespace oem
