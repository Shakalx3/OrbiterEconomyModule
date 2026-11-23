#include "PropellantTransactionJournal.h"
#include <cmath>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <locale>
#include <sstream>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

namespace oem { namespace {
const char* HEADER = "OEM_PROPELLANT_TX_JOURNAL|1";
std::string hex(const std::string& s) {
    static const char d[]="0123456789ABCDEF"; std::string out;
    for(std::size_t i=0;i<s.size();++i){unsigned char c=(unsigned char)s[i];out+=d[c>>4];out+=d[c&15];}
    return out;
}
bool unhex(const std::string& s,std::string& out) {
    if(s.size()%2)return false;out.clear();
    for(std::size_t i=0;i<s.size();i+=2){
        const char* d="0123456789ABCDEF";std::string::size_type a=std::string(d).find((char)std::toupper((unsigned char)s[i]));
        std::string::size_type b=std::string(d).find((char)std::toupper((unsigned char)s[i+1]));
        if(a==std::string::npos||b==std::string::npos)return false;out+=(char)((a<<4)|b);
    } return true;
}
std::vector<std::string> split(const std::string& s) {
    std::vector<std::string> v;std::istringstream in(s);std::string x;
    while(std::getline(in,x,'|'))v.push_back(x);return v;
}
bool integer(const std::string&s,long long&v){if(s.empty())return false;char*e=0;long long n=std::strtoll(s.c_str(),&e,10);if(!e||*e)return false;v=n;return true;}
bool real(const std::string&s,double&v){if(s.empty())return false;std::istringstream in(s);in.imbue(std::locale::classic());double n=0;in>>n;if(!in||in.peek()!=std::char_traits<char>::eof()||!std::isfinite(n))return false;v=n;return true;}
std::string encode(const PropellantTransaction& t) {
    std::ostringstream o;o.imbue(std::locale::classic());
    o<<"P|"<<(int)t.state<<'|'<<hex(t.operationId)<<'|'<<hex(t.vesselId)<<'|'
     <<hex(t.quote.quoteId)<<'|'<<hex(t.quote.context.locationId)<<'|'
     <<(int)t.quote.context.locationKind<<'|'<<hex(t.quote.context.celestialBody)<<'|'
     <<std::setprecision(17)<<t.quote.requestedMassKg<<'|'<<t.quote.unitPriceCreditsPerKg<<'|'
     <<t.quote.totalCostCredits<<'|'<<t.quote.balanceAtQuote<<'|'
     <<t.quote.fillPlan.plannedMassKg<<'|'<<t.resources.size();
    for(std::size_t i=0;i<t.resources.size();++i)o<<'|'<<t.resources[i].capacityKg<<'|'<<t.resources[i].beforeMassKg<<'|'<<t.resources[i].afterMassKg;
    o<<'|'<<t.simulationTime<<'\n';return o.str();
}
bool decode(const std::string& line,PropellantTransaction& t) {
    std::vector<std::string> f=split(line);long long state=0,kind=0,cost=0,balance=0,count=0;
    if(f.size()<14||f[0]!="P"||!integer(f[1],state)||state<PROPELLANT_TX_PREPARED||state>PROPELLANT_TX_ABORTED||
       !unhex(f[2],t.operationId)||!unhex(f[3],t.vesselId)||!unhex(f[4],t.quote.quoteId)||
       !unhex(f[5],t.quote.context.locationId)||!integer(f[6],kind)||
       (kind!=PROPELLANT_LOCATION_ORBITAL_STATION&&kind!=PROPELLANT_LOCATION_PLANETARY_SPACEPORT)||
       !unhex(f[7],t.quote.context.celestialBody)||!real(f[8],t.quote.requestedMassKg)||
       !real(f[9],t.quote.unitPriceCreditsPerKg)||!integer(f[10],cost)||!integer(f[11],balance)||
       !real(f[12],t.quote.fillPlan.plannedMassKg)||!integer(f[13],count)||count<=0||
       count>(long long)((f.size()-14)/3)||(f.size()!=14+(std::size_t)count*3&&f.size()!=15+(std::size_t)count*3))return false;
    t.state=(PropellantTransactionState)state;t.quote.context.locationKind=(PropellantLocationKind)kind;
    t.quote.totalCostCredits=cost;t.quote.balanceAtQuote=balance;
    t.simulationTime=0;
    if(f.size()==15+(std::size_t)count*3){long long sim=0;if(!integer(f.back(),sim))return false;t.simulationTime=sim;}
    t.quote.fillPlan.requestedMassKg=t.quote.requestedMassKg;
    t.quote.fillPlan.targetMassKg.clear();t.resources.clear();
    for(long long i=0;i<count;++i){double cap=0,before=0,after=0;std::size_t n=14+(std::size_t)i*3;
        if(!real(f[n],cap)||!real(f[n+1],before)||!real(f[n+2],after))return false;
        PropellantResourceChange c;c.resourceIndex=(std::size_t)i;c.capacityKg=cap;c.beforeMassKg=before;c.afterMassKg=after;
        t.resources.push_back(c);t.quote.fillPlan.targetMassKg.push_back(after);
    }
    return !t.operationId.empty()&&!t.vesselId.empty()&&!t.quote.quoteId.empty()&&
        !t.quote.context.locationId.empty()&&!t.quote.context.celestialBody.empty()&&
        t.quote.requestedMassKg>0&&t.quote.unitPriceCreditsPerKg>0&&t.quote.totalCostCredits>0;
}
bool sameImmutable(const PropellantTransaction&a,const PropellantTransaction&b){
    PropellantTransaction x=a,y=b;x.state=y.state=PROPELLANT_TX_PREPARED;
    return encode(x)==encode(y);
}
bool appendDurable(const std::string&p,const std::string&s){
#ifdef _WIN32
    HANDLE h=CreateFileA(p.c_str(),GENERIC_WRITE,FILE_SHARE_READ,0,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,0);
    if(h==INVALID_HANDLE_VALUE)return false;LARGE_INTEGER z;z.QuadPart=0;
    bool ok=SetFilePointerEx(h,z,0,FILE_END)!=0;DWORD n=0;
    if(ok)ok=WriteFile(h,s.data(),(DWORD)s.size(),&n,0)&&n==s.size();
    if(ok)ok=FlushFileBuffers(h)!=0;CloseHandle(h);return ok;
#else
    int fd=open(p.c_str(),O_WRONLY|O_CREAT|O_APPEND,0666);if(fd<0)return false;
    std::size_t n=0;bool ok=true;while(n<s.size()){ssize_t k=write(fd,s.data()+n,s.size()-n);if(k<=0){ok=false;break;}n+=(std::size_t)k;}
    if(ok)ok=fsync(fd)==0;if(close(fd))ok=false;return ok;
#endif
}
} // namespace

Result PropellantTransactionJournal::load(const std::string& path,
    std::map<std::string,PropellantTransaction>& transactions) const {
    std::ifstream in(path.c_str(),std::ios::binary);if(!in)return Result::success();
    std::string data((std::istreambuf_iterator<char>(in)),std::istreambuf_iterator<char>());
    if(!in.good()&&!in.eof())return Result::failure("PROPELLANT_TX_READ","Could not read refuelling transaction journal.");
    if(data.empty())return Result::failure("PROPELLANT_TX_HEADER","Refuelling transaction journal is empty.");
    if(data[data.size()-1]!='\n')return Result::failure("PROPELLANT_TX_TORN_TAIL","Refuelling transaction journal has an incomplete final record; refusing automatic repair.");
    std::istringstream lines(data);std::string line;
    if(!std::getline(lines,line)||line!=HEADER)return Result::failure("PROPELLANT_TX_HEADER","Refuelling transaction journal header/version is invalid.");
    std::map<std::string,PropellantTransaction> next;
    while(std::getline(lines,line)){if(line.empty())continue;PropellantTransaction t;
        if(!decode(line,t))return Result::failure("PROPELLANT_TX_CORRUPT","A complete refuelling transaction record is invalid.");
        std::map<std::string,PropellantTransaction>::iterator it=next.find(t.operationId);
        if(it==next.end()){if(t.state!=PROPELLANT_TX_PREPARED)return Result::failure("PROPELLANT_TX_TRANSITION","Terminal transaction has no PREPARED record.");next[t.operationId]=t;continue;}
        if(!sameImmutable(it->second,t))return Result::failure("PROPELLANT_TX_CONFLICT","Operation ID was reused with different transaction data.");
        if(it->second.state!=PROPELLANT_TX_PREPARED||t.state==PROPELLANT_TX_PREPARED)
            return Result::failure("PROPELLANT_TX_TRANSITION","Invalid or duplicate terminal transaction transition.");
        it->second.state=t.state;
    }
    transactions.swap(next);return Result::success();
}
Result PropellantTransactionJournal::prepare(const std::string& path,const PropellantTransaction& t) const {
    if(t.state!=PROPELLANT_TX_PREPARED||t.operationId.empty())return Result::failure("PROPELLANT_TX_PREPARE","Only a valid PREPARED transaction can be persisted.");
    std::map<std::string,PropellantTransaction> all;Result r=load(path,all);
    if(!r.ok)return r;
    std::map<std::string,PropellantTransaction>::const_iterator it=all.find(t.operationId);
    if(it!=all.end())return sameImmutable(it->second,t)?Result::success():Result::failure("PROPELLANT_TX_CONFLICT","Operation ID already exists with different transaction data.");
    std::ifstream check(path.c_str(),std::ios::binary);bool exists=(bool)check;
    if(!appendDurable(path,(exists?std::string():std::string(HEADER)+"\n")+encode(t)))
        return Result::failure("PROPELLANT_TX_APPEND","Could not durably append PREPARED refuelling intent.");
    return Result::success();
}
Result PropellantTransactionJournal::setTerminalState(const std::string& path,
    const std::string& operationId,PropellantTransactionState state) const {
    if(state!=PROPELLANT_TX_COMMITTED&&state!=PROPELLANT_TX_ABORTED)
        return Result::failure("PROPELLANT_TX_STATE","Only COMMITTED or ABORTED are valid terminal states.");
    std::map<std::string,PropellantTransaction> all;Result r=load(path,all);if(!r.ok)return r;
    std::map<std::string,PropellantTransaction>::const_iterator it=all.find(operationId);
    if(it==all.end())return Result::failure("PROPELLANT_TX_NOT_FOUND","No PREPARED refuelling transaction exists for this operation.");
    if(it->second.state==state)return Result::success();
    if(it->second.state!=PROPELLANT_TX_PREPARED)return Result::failure("PROPELLANT_TX_TRANSITION","A terminal refuelling transaction cannot change state.");
    PropellantTransaction terminal=it->second;terminal.state=state;
    if(!appendDurable(path,encode(terminal)))return Result::failure("PROPELLANT_TX_APPEND","Could not durably append terminal refuelling state.");
    return Result::success();
}
} // namespace oem
