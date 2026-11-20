#include "CargoExchangeJournal.h"
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
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
    if(value.empty() || value.size()%2)return false;
    out.clear();
    for(std::size_t i=0;i<value.size();i+=2) {
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
    while(std::getline(in,field,'|'))fields.push_back(field);
    return fields;
}
bool parseLong(const std::string& s,long long& value) {
    if(s.empty())return false; char* end=0; const long long n=std::strtoll(s.c_str(),&end,10);
    if(!end||*end!='\\0')return false; value=n; return true;
}
bool parseDouble(const std::string& s,double& value) {
    if(s.empty())return false; char* end=0; const double n=std::strtod(s.c_str(),&end);
    if(!end||*end!='\\0'||!std::isfinite(n))return false; value=n; return true;
}
bool parseRecord(const std::string& line,CargoTransfer& t) {
    const std::vector<std::string> f=split(line); long long direction=0,quantity=0,time=0;
    if(f.size()!=9||f[0]!="X"||!hexDecode(f[1],t.transferId)||
       !parseLong(f[2],direction)||(direction!=CARGO_LOAD&&direction!=CARGO_UNLOAD)||
       !hexDecode(f[3],t.vesselId)||!hexDecode(f[4],t.locationId)||
       !hexDecode(f[5],t.cargoId)||!parseLong(f[6],quantity)||
       !parseDouble(f[7],t.unitMassKg)||!parseLong(f[8],time))return false;
    t.direction=static_cast<CargoTransferDirection>(direction);
    t.quantity=quantity;t.simulationTime=time;return true;
}
std::string serialize(const CargoTransfer& t) {
    std::ostringstream out;
    out<<"X|"<<hexEncode(t.transferId)<<'|'<<static_cast<int>(t.direction)<<'|'
       <<hexEncode(t.vesselId)<<'|'<<hexEncode(t.locationId)<<'|'<<hexEncode(t.cargoId)
       <<'|'<<t.quantity<<'|'<<std::setprecision(17)<<t.unitMassKg<<'|'<<t.simulationTime<<'\\n';
    return out.str();
}
bool durableAppend(const std::string& path,const std::string& bytes) {
#ifdef _WIN32
    HANDLE file=CreateFileA(path.c_str(),GENERIC_WRITE,FILE_SHARE_READ,0,OPEN_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL,0);
    if(file==INVALID_HANDLE_VALUE)return false;
    LARGE_INTEGER zero; zero.QuadPart=0;
    bool ok=SetFilePointerEx(file,zero,0,FILE_END)!=0;
    DWORD written=0;
    if(ok)ok=WriteFile(file,bytes.data(),(DWORD)bytes.size(),&written,0)!=0 && written==bytes.size();
    if(ok)ok=FlushFileBuffers(file)!=0;
    CloseHandle(file);return ok;
#else
    const int fd=open(path.c_str(),O_WRONLY|O_CREAT|O_APPEND,0666);
    if(fd<0)return false;
    std::size_t offset=0;bool ok=true;
    while(offset<bytes.size()) {
        const ssize_t n=write(fd,bytes.data()+offset,bytes.size()-offset);
        if(n<=0){ok=false;break;} offset+=(std::size_t)n;
    }
    if(ok)ok=fsync(fd)==0;
    if(close(fd)!=0)ok=false;
    return ok;
#endif
}
bool repairTornTail(const std::string& path,bool& exists,std::string& contents) {
    std::ifstream in(path.c_str(),std::ios::in|std::ios::binary);
    if(!in.is_open()){exists=false;contents.clear();return true;}
    exists=true;contents.assign(std::istreambuf_iterator<char>(in),std::istreambuf_iterator<char>());
    if(!in.good()&&!in.eof())return false;
    if(contents.empty()||contents[contents.size()-1]=='\\n')return true;
    const std::string::size_type last=contents.find_last_of('\\n');
    const std::size_t keep=last==std::string::npos?0:last+1;
#ifdef _WIN32
    HANDLE file=CreateFileA(path.c_str(),GENERIC_WRITE,0,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
    if(file==INVALID_HANDLE_VALUE)return false;
    LARGE_INTEGER pos;pos.QuadPart=(LONGLONG)keep;
    bool ok=SetFilePointerEx(file,pos,0,FILE_BEGIN)!=0 && SetEndOfFile(file)!=0 && FlushFileBuffers(file)!=0;
    CloseHandle(file);if(!ok)return false;
#else
    if(truncate(path.c_str(),(off_t)keep)!=0)return false;
#endif
    contents.resize(keep);return true;
}
std::string headerFor(const std::string& service) {
    return "OEM_CARGO_EXCHANGE|1|"+hexEncode(service);
}
} // namespace

Result CargoExchangeJournalStore::load(const std::string& path,CargoServiceInventory& inventory,
                                       std::map<std::string,CargoManifest>& manifests) const {
    bool exists=false;std::string contents;
    if(!repairTornTail(path,exists,contents))
        return Result::failure("CARGO_EXCHANGE_READ","Could not read or repair exchange journal tail.");
    if(!exists)return Result::success();
    std::istringstream in(contents);std::string line;
    if(!std::getline(in,line))return Result::failure("CARGO_EXCHANGE_EMPTY","Exchange journal is empty.");
    if(!line.empty()&&line[0]=='\\xEF'&&line.size()>=3&&
       (unsigned char)line[1]==0xBB&&(unsigned char)line[2]==0xBF)line.erase(0,3);
    if(!line.empty()&&line[line.size()-1]=='\\r')line.erase(line.size()-1);
    if(line!=headerFor(inventory.serviceVesselId()))
        return Result::failure("CARGO_EXCHANGE_HEADER","Exchange journal version or service vessel does not match.");
    CargoServiceInventory candidateInventory(inventory);
    std::map<std::string,CargoManifest> candidateManifests(manifests);
    while(std::getline(in,line)) {
        if(!line.empty()&&line[line.size()-1]=='\\r')line.erase(line.size()-1);
        if(line.empty())continue;
        CargoTransfer t;
        if(!parseRecord(line,t))return Result::failure("CARGO_EXCHANGE_CORRUPT","A complete exchange journal record is invalid.");
        if(t.locationId!=candidateInventory.serviceVesselId())
            return Result::failure("CARGO_EXCHANGE_SERVICE_MISMATCH","Exchange record names a different service vessel.");
        std::map<std::string,CargoManifest>::iterator it=candidateManifests.find(t.vesselId);
        if(it==candidateManifests.end())
            it=candidateManifests.insert(std::make_pair(t.vesselId,CargoManifest(t.vesselId))).first;
        Result station=candidateInventory.apply(t);
        if(!station.ok)return Result::failure(station.code,station.message);
        Result vessel=t.direction==CARGO_LOAD?it->second.load(t):it->second.unload(t);
        if(!vessel.ok)return Result::failure(vessel.code,vessel.message);
    }
    inventory=candidateInventory;manifests.swap(candidateManifests);
    return Result::success();
}

Result CargoExchangeJournalStore::apply(const std::string& path,CargoServiceInventory& inventory,
                                        CargoManifest& manifest,const CargoTransfer& transfer) const {
    if(transfer.locationId!=inventory.serviceVesselId()||transfer.vesselId!=manifest.vesselId())
        return Result::failure("CARGO_EXCHANGE_IDENTITY","Transfer must match the selected service and vessel manifests.");
    if(inventory.hasTransfer(transfer.transferId)!=manifest.hasTransfer(transfer.transferId))
        return Result::failure("CARGO_EXCHANGE_SPLIT_STATE","Only one accounting projection contains this transfer ID.");
    CargoServiceInventory candidateInventory(inventory);
    CargoManifest candidateManifest(manifest);
    Result station=candidateInventory.apply(transfer);if(!station.ok)return station;
    Result vessel=transfer.direction==CARGO_LOAD?candidateManifest.load(transfer):candidateManifest.unload(transfer);
    if(!vessel.ok)return vessel;
    if(inventory.hasTransfer(transfer.transferId))return Result::success();
    bool exists=false;std::string contents;
    if(!repairTornTail(path,exists,contents))
        return Result::failure("CARGO_EXCHANGE_TAIL","Could not inspect or repair exchange journal before append.");
    const std::string header=headerFor(inventory.serviceVesselId())+"\\n";
    if(exists) {
        const std::string::size_type end=contents.find('\\n');
        if(end==std::string::npos||contents.substr(0,end)!=headerFor(inventory.serviceVesselId()))
            return Result::failure("CARGO_EXCHANGE_HEADER","Existing exchange journal has an invalid header.");
    }
    const std::string bytes=(exists?std::string():header)+serialize(transfer);
    if(!durableAppend(path,bytes))
        return Result::failure("CARGO_EXCHANGE_COMMIT","Could not durably append the exchange commit record.");
    inventory=candidateInventory;manifest=candidateManifest;
    return Result::success();
}
}
