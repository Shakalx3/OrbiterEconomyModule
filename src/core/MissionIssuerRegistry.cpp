#include "MissionIssuerRegistry.h"
#include <cstdio>
#include <fstream>
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
} // namespace

Result MissionIssuerRegistry::loadOrCreate(const std::string& path) {
    if (path.empty()) return Result::failure("MISSION_ISSUER_PATH","Mission issuer registry path is empty.");
    std::ifstream in(path.c_str(),std::ios::in|std::ios::binary);
    if (!in.is_open()) {
        std::ofstream out(path.c_str(),std::ios::out|std::ios::binary|std::ios::trunc);
        if (!out.is_open()) return Result::failure("MISSION_ISSUER_CREATE","Could not create mission issuer registry.");
        out << "OEM_MISSION_ISSUERS|1\n";
        out.close();
        if (out.fail()) return Result::failure("MISSION_ISSUER_WRITE","Could not finish mission issuer registry.");
        mappings_.clear();
        return Result::success();
    }
    std::string header;
    if (!std::getline(in,header)) return Result::failure("MISSION_ISSUER_HEADER","Mission issuer registry is empty.");
    if (!header.empty() && header[header.size()-1]=='\r') header.erase(header.size()-1);
    if (header!="OEM_MISSION_ISSUERS|1")
        return Result::failure("MISSION_ISSUER_HEADER","Unsupported mission issuer registry version.");
    std::map<std::string,std::string> parsed;
    std::string line;
    while (std::getline(in,line)) {
        if (!line.empty() && line[line.size()-1]=='\r') line.erase(line.size()-1);
        if (line.empty()) continue;
        if (line.compare(0,2,"L|")!=0)
            return Result::failure("MISSION_ISSUER_RECORD","Invalid mission issuer mapping record.");
        const std::size_t separator=line.find('|',2);
        std::string location,issuer;
        if (separator==std::string::npos ||
            !decodeHex(line.substr(2,separator-2),location) ||
            !decodeHex(line.substr(separator+1),issuer) ||
            !parsed.insert(std::make_pair(location,issuer)).second)
            return Result::failure("MISSION_ISSUER_RECORD","Invalid or duplicate mission issuer mapping.");
    }
    mappings_.swap(parsed);
    return Result::success();
}

Result MissionIssuerRegistry::save(const std::string& path) const {
    if (path.empty()) return Result::failure("MISSION_ISSUER_PATH","Mission issuer registry path is empty.");
    const std::string temporary=path+".tmp";
    {
        std::ofstream out(temporary.c_str(),std::ios::out|std::ios::binary|std::ios::trunc);
        if (!out.is_open()) return Result::failure("MISSION_ISSUER_OPEN","Could not create temporary mission issuer registry.");
        out << "OEM_MISSION_ISSUERS|1\n";
        for (std::map<std::string,std::string>::const_iterator it=mappings_.begin();it!=mappings_.end();++it)
            out << "L|" << encodeHex(it->first) << "|" << encodeHex(it->second) << "\n";
        out.flush();
        if (!out.good()) return Result::failure("MISSION_ISSUER_WRITE","Could not write mission issuer registry.");
        out.close();
        if (out.fail()) return Result::failure("MISSION_ISSUER_CLOSE","Could not close mission issuer registry.");
    }
    if (!replaceFile(temporary,path)) {
        std::remove(temporary.c_str());
        return Result::failure("MISSION_ISSUER_REPLACE","Could not atomically replace mission issuer registry.");
    }
    return Result::success();
}

Result MissionIssuerRegistry::setIssuerForLocation(const std::string& location,const std::string& issuer) {
    if (location.empty() || issuer.empty())
        return Result::failure("MISSION_ISSUER_ID","Canonical location and issuer IDs are required.");
    mappings_[location]=issuer;
    return Result::success();
}

std::string MissionIssuerRegistry::issuerForLocation(const std::string& location) const {
    if (location.empty()) return std::string();
    std::map<std::string,std::string>::const_iterator it=mappings_.find(location);
    return it==mappings_.end() ? "issuer:"+location : it->second;
}

const std::map<std::string,std::string>& MissionIssuerRegistry::mappings() const {
    return mappings_;
}

} // namespace oem
