#include "CargoManifestJournal.h"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#endif

namespace oem { namespace {
std::string hexEncode(const std::string& value) {
    static const char digits[] = "0123456789ABCDEF";
    std::string out;
    for (std::size_t i=0;i<value.size();++i) {
        const unsigned char c=static_cast<unsigned char>(value[i]);
        out.push_back(digits[c>>4]); out.push_back(digits[c&15]);
    }
    return out;
}
bool hexDecode(const std::string& value,std::string& out) {
    if (value.size()%2) return false;
    out.clear();
    for (std::size_t i=0;i<value.size();i+=2) {
        const char a=value[i],b=value[i+1];
        const int hi=a>='0'&&a<='9'?a-'0':a>='A'&&a<='F'?a-'A'+10:a>='a'&&a<='f'?a-'a'+10:-1;
        const int lo=b>='0'&&b<='9'?b-'0':b>='A'&&b<='F'?b-'A'+10:b>='a'&&b<='f'?b-'a'+10:-1;
        if(hi<0||lo<0)return false;
        out.push_back(static_cast<char>((hi<<4)|lo));
    }
    return true;
}
std::vector<std::string> split(const std::string& line) {
    std::vector<std::string> fields; std::istringstream in(line); std::string field;
    while(std::getline(in,field,'|')) fields.push_back(field);
    return fields;
}
bool parseLong(const std::string& s,long long& value) {
    if(s.empty())return false; char* end=0; const long long n=std::strtoll(s.c_str(),&end,10);
    if(!end||*end!='\0')return false; value=n; return true;
}
bool parseDouble(const std::string& s,double& value) {
    if(s.empty())return false; char* end=0; const double n=std::strtod(s.c_str(),&end);
    if(!end||*end!='\0')return false; value=n; return true;
}
bool replaceFile(const std::string& temporary,const std::string& target) {
#ifdef _WIN32
    return MoveFileExA(temporary.c_str(),target.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
#else
    return std::rename(temporary.c_str(),target.c_str())==0;
#endif
}
bool parseTransfer(const std::string& line,CargoTransfer& t) {
    const std::vector<std::string> f=split(line);
    long long direction=0,quantity=0,time=0;
    if(f.size()!=9||f[0]!="T"||!hexDecode(f[1],t.transferId)||
       !parseLong(f[2],direction)||(direction!=CARGO_LOAD&&direction!=CARGO_UNLOAD)||
       !hexDecode(f[3],t.vesselId)||!hexDecode(f[4],t.locationId)||
       !hexDecode(f[5],t.cargoId)||!parseLong(f[6],quantity)||
       !parseDouble(f[7],t.unitMassKg)||!parseLong(f[8],time)) return false;
    t.direction=static_cast<CargoTransferDirection>(direction);
    t.quantity=quantity;t.simulationTime=time;
    return true;
}
void writeTransfer(std::ostream& out,const CargoTransfer& t) {
    out<<"T|"<<hexEncode(t.transferId)<<'|'<<static_cast<int>(t.direction)<<'|'
       <<hexEncode(t.vesselId)<<'|'<<hexEncode(t.locationId)<<'|'<<hexEncode(t.cargoId)
       <<'|'<<t.quantity<<'|'<<std::setprecision(17)<<t.unitMassKg<<'|'<<t.simulationTime<<'\n';
}
} // namespace

Result CargoManifestJournalStore::load(const std::string& path,CargoManifest& manifest) const {
    std::ifstream in(path.c_str(),std::ios::in|std::ios::binary);
    if(!in.is_open())return Result::success();
    std::string header;
    if(!std::getline(in,header))return Result::failure("CARGO_JOURNAL_EMPTY","Cargo journal is empty.");
    const std::string expected="OEM_CARGO_MANIFEST|1|"+hexEncode(manifest.vesselId());
    if(header!=expected)return Result::failure("CARGO_JOURNAL_HEADER","Cargo journal version or vessel ID does not match.");
    CargoManifest candidate(manifest);
    std::string line;
    while(std::getline(in,line)) {
        if(line.empty())continue;
        CargoTransfer transfer;
        if(!parseTransfer(line,transfer)) {
            if(in.eof())break; // tolerate only a torn final record
            return Result::failure("CARGO_JOURNAL_CORRUPT","A complete cargo journal record is invalid.");
        }
        Result applied=transfer.direction==CARGO_LOAD?candidate.load(transfer):candidate.unload(transfer);
        if(!applied.ok)return Result::failure(applied.code,applied.message);
    }
    manifest=candidate;
    return Result::success();
}

Result CargoManifestJournalStore::save(const std::string& path,const CargoManifest& manifest) const {
    const std::string temporary=path+".tmp";
    {
        std::ofstream out(temporary.c_str(),std::ios::out|std::ios::binary|std::ios::trunc);
        if(!out.is_open())return Result::failure("CARGO_JOURNAL_OPEN","Could not create temporary cargo journal.");
        out<<"OEM_CARGO_MANIFEST|1|"<<hexEncode(manifest.vesselId())<<'\n';
        const std::vector<CargoTransfer>& history=manifest.history();
        for(std::size_t i=0;i<history.size();++i)writeTransfer(out,history[i]);
        out.flush();
        if(!out.good())return Result::failure("CARGO_JOURNAL_WRITE","Could not write complete cargo journal.");
        out.close();
        if(out.fail())return Result::failure("CARGO_JOURNAL_CLOSE","Could not close cargo journal.");
    }
    if(!replaceFile(temporary,path)) {
        std::remove(temporary.c_str());
        return Result::failure("CARGO_JOURNAL_REPLACE","Could not atomically replace cargo journal.");
    }
    return Result::success();
}

} // namespace oem
