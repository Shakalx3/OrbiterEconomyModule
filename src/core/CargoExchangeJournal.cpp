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
    static const char digits[]="0123456789ABCDEF"; std::string out;
    for(std::size_t i=0;i<value.size();++i){const unsigned char c=(unsigned char)value[i];out.push_back(digits[c>>4]);out.push_back(digits[c&15]);}
    return out;
}
bool hexDecode(const std::string& value,std::string& out) {
    if(value.empty()||value.size()%2)return false;out.clear();
    for(std::size_t i=0;i<value.size();i+=2){
        const char a=value[i],b=value[i+1];
        const int hi=a>='0'&&a<='9'?a-'0':a>='A'&&a<='F'?a-'A'+10:a>='a'&&a<='f'?a-'a'+10:-1;
        const int lo=b>='0'&&b<='9'?b-'0':b>='A'&&b<='F'?b-'A'+10:b>='a'&&b<='f'?b-'a'+10:-1;
        if(hi<0||lo<0)return false;out.push_back((char)((hi<<4)|lo));
    } return true;
}
std::vector<std::string> split(const std::string& line) {
    std::vector<std::string> f;std::istringstream in(line);std::string s;while(std::getline(in,s,'|'))f.push_back(s);return f;
}
bool parseLong(const std::string& s,long long& v) {
    if(s.empty())return false;char* end=0;long long n=std::strtoll(s.c_str(),&end,10);if(!end||*end!='\0')return false;v=n;return true;
}
bool parseDouble(const std::string& s,double& v) {
    if(s.empty())return false;char* end=0;double n=std::strtod(s.c_str(),&end);if(!end||*end!='\0'||!std::isfinite(n))return false;v=n;return true;
}
bool parseRecord(const std::string& line,CargoTransfer& t) {
    const std::vector<std::string> f=split(line);long long d=0,q=0,time=0;
    if(f.size()!=9||f[0]!="X"||!hexDecode(f[1],t.transferId)||!parseLong(f[2],d)||(d!=CARGO_LOAD&&d!=CARGO_UNLOAD)||
       !hexDecode(f[3],t.vesselId)||!hexDecode(f[4],t.locationId)||!hexDecode(f[5],t.cargoId)||
       !parseLong(f[6],q)||!parseDouble(f[7],t.unitMassKg)||!parseLong(f[8],time))return false;
    t.direction=(CargoTransferDirection)d;t.quantity=q;t.simulationTime=time;return true;
}
std::string serialize(const CargoTransfer& t) {
    std::ostringstream out;out<<"X|"<<hexEncode(t.transferId)<<'|'<<(int)t.direction<<'|'<<hexEncode(t.vesselId)<<'|'
        <<hexEncode(t.locationId)<<'|'<<hexEncode(t.cargoId)<<'|'<<t.quantity<<'|'<<std::setprecision(17)<<t.unitMassKg<<'|'<<t.simulationTime<<'\n';
    return out.str();
}
bool durableAppend(const std::string& path,const std::string& bytes) {
#ifdef _WIN32
    HANDLE h=CreateFileA(path.c_str(),GENERIC_WRITE,FILE_SHARE_READ,0,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,0);
    if(h==INVALID_HANDLE_VALUE)return false;LARGE_INTEGER zero;zero.QuadPart=0;
    bool ok=SetFilePointerEx(h,zero,0,FILE_END)!=0;DWORD written=0;
    if(ok)ok=WriteFile(h,bytes.data(),(DWORD)bytes.size(),&written,0)!=0&&written==bytes.size();
    if(ok)ok=FlushFileBuffers(h)!=0;CloseHandle(h);return ok;
#else
    int fd=open(path.c_str(),O_WRONLY|O_CREAT|O_APPEND,0666);if(fd<0)return false;
    std::size_t off=0;bool ok=true;while(off<bytes.size()){ssize_t n=write(fd,bytes.data()+off,bytes.size()-off);if(n<=0){ok=false;break;}off+=(std::size_t)n;}
    if(ok)ok=fsync(fd)==0;if(close(fd)!=0)ok=false;return ok;
#endif
}
bool inspectAndRepairTail(const std::string& path,bool& exists,std::string& contents) {
    std::ifstream in(path.c_str(),std::ios::binary);if(!in.is_open()){exists=false;contents.clear();return true;}
    exists=true;contents.assign(std::istreambuf_iterator<char>(in),std::istreambuf_iterator<char>());if(!in.good()&&!in.eof())return false;
    if(contents.empty()||contents[contents.size()-1]=='\n')return true;
    std::string::size_type pos=contents.find_last_of('\n');std::size_t keep=pos==std::string::npos?0:pos+1;
#ifdef _WIN32
    HANDLE h=CreateFileA(path.c_str(),GENERIC_WRITE,0,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);if(h==INVALID_HANDLE_VALUE)return false;
    LARGE_INTEGER p;p.QuadPart=(LONGLONG)keep;bool ok=SetFilePointerEx(h,p,0,FILE_BEGIN)!=0&&SetEndOfFile(h)!=0&&FlushFileBuffers(h)!=0;CloseHandle(h);if(!ok)return false;
#else
    if(truncate(path.c_str(),(off_t)keep)!=0)return false;
#endif
    contents.resize(keep);return true;
}
std::string headerFor(const std::string& id){return "OEM_CARGO_EXCHANGE|1|"+hexEncode(id);}
} // namespace
Result CargoExchangeJournalStore::load(const std::string& path,CargoServiceInventory& inventory,std::map<std::string,CargoManifest>& manifests) const {
    bool exists=false;std::string contents;if(!inspectAndRepairTail(path,exists,contents))return Result::failure("CARGO_EXCHANGE_READ","Could not read or repair exchange journal tail.");
    if(!exists)return Result::success();
    std::istringstream in(contents);std::string line;if(!std::getline(in,line))return Result::failure("CARGO_EXCHANGE_EMPTY","Exchange journal is empty.");
    if(line.size()>=3&&(unsigned char)line[0]==0xEF&&(unsigned char)line[1]==0xBB&&(unsigned char)line[2]==0xBF)line.erase(0,3);
    if(!line.empty()&&line[line.size()-1]=='\r')line.erase(line.size()-1);
    if(line!=headerFor(inventory.serviceVesselId()))return Result::failure("CARGO_EXCHANGE_HEADER","Exchange journal version or service vessel does not match.");
    CargoServiceInventory nextInventory(inventory);std::map<std::string,CargoManifest> nextManifests(manifests);
    while(std::getline(in,line)){
        if(!line.empty()&&line[line.size()-1]=='\r')line.erase(line.size()-1);if(line.empty())continue;
        CargoTransfer t;if(!parseRecord(line,t))return Result::failure("CARGO_EXCHANGE_CORRUPT","A complete exchange journal record is invalid.");
        if(t.locationId!=nextInventory.serviceVesselId())return Result::failure("CARGO_EXCHANGE_SERVICE_MISMATCH","Exchange record names a different service vessel.");
        std::map<std::string,CargoManifest>::iterator it=nextManifests.find(t.vesselId);
        if(it==nextManifests.end())it=nextManifests.insert(std::make_pair(t.vesselId,CargoManifest(t.vesselId))).first;
        Result a=nextInventory.apply(t);if(!a.ok)return Result::failure(a.code,a.message);
        Result b=t.direction==CARGO_LOAD?it->second.load(t):it->second.unload(t);if(!b.ok)return Result::failure(b.code,b.message);
    }
    inventory=nextInventory;manifests.swap(nextManifests);return Result::success();
}
Result CargoExchangeJournalStore::apply(const std::string& path,CargoServiceInventory& inventory,CargoManifest& manifest,const CargoTransfer& transfer) const {
    if(transfer.locationId!=inventory.serviceVesselId()||transfer.vesselId!=manifest.vesselId())return Result::failure("CARGO_EXCHANGE_IDENTITY","Transfer must match the selected service and vessel manifests.");
    if(inventory.hasTransfer(transfer.transferId)!=manifest.hasTransfer(transfer.transferId))return Result::failure("CARGO_EXCHANGE_SPLIT_STATE","Only one accounting projection contains this transfer ID.");
    CargoServiceInventory nextInventory(inventory);CargoManifest nextManifest(manifest);
    Result a=nextInventory.apply(transfer);if(!a.ok)return a;
    Result b=transfer.direction==CARGO_LOAD?nextManifest.load(transfer):nextManifest.unload(transfer);if(!b.ok)return b;
    if(inventory.hasTransfer(transfer.transferId))return Result::success();
    bool exists=false;std::string contents;if(!inspectAndRepairTail(path,exists,contents))return Result::failure("CARGO_EXCHANGE_TAIL","Could not inspect or repair exchange journal before append.");
    if(exists){std::string::size_type end=contents.find('\n');if(end==std::string::npos||contents.substr(0,end)!=headerFor(inventory.serviceVesselId()))return Result::failure("CARGO_EXCHANGE_HEADER","Existing exchange journal has an invalid header.");}
    const std::string bytes=(exists?std::string():headerFor(inventory.serviceVesselId())+"\n")+serialize(transfer);
    if(!durableAppend(path,bytes))return Result::failure("CARGO_EXCHANGE_COMMIT","Could not durably append the exchange commit record.");
    inventory=nextInventory;manifest=nextManifest;return Result::success();
}
}
