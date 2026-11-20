#include "MarketTradeJournal.h"
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
std::string hx(const std::string& s){static const char d[]="0123456789ABCDEF";std::string o;for(std::size_t i=0;i<s.size();++i){unsigned char c=(unsigned char)s[i];o+=d[c>>4];o+=d[c&15];}return o;}
bool unhx(const std::string&s,std::string&o){if(s.empty()||s.size()%2)return false;o.clear();for(std::size_t i=0;i<s.size();i+=2){char a=s[i],b=s[i+1];int x=a>='0'&&a<='9'?a-'0':a>='A'&&a<='F'?a-'A'+10:a>='a'&&a<='f'?a-'a'+10:-1;int y=b>='0'&&b<='9'?b-'0':b>='A'&&b<='F'?b-'A'+10:b>='a'&&b<='f'?b-'a'+10:-1;if(x<0||y<0)return false;o+=(char)((x<<4)|y);}return true;}
std::vector<std::string> split(const std::string&s){std::vector<std::string>v;std::istringstream i(s);std::string x;while(std::getline(i,x,'|'))v.push_back(x);return v;}
bool num(const std::string&s,long long&v){if(s.empty())return false;char*e=0;long long n=std::strtoll(s.c_str(),&e,10);if(!e||*e)return false;v=n;return true;}
bool dbl(const std::string&s,double&v){if(s.empty())return false;char*e=0;double n=std::strtod(s.c_str(),&e);if(!e||*e||!std::isfinite(n))return false;v=n;return true;}
std::string header(const std::string&s,const std::string&v){return "OEM_MARKET_TRADES|1|"+hx(s)+"|"+hx(v);}
struct Record{std::string op,buyer,station,vessel,cargo;MarketSide side;long long qty;Credits unit,total;SimulationSeconds time;};
std::string encode(const Record&r){std::ostringstream o;o<<"T|"<<hx(r.op)<<'|'<<hx(r.buyer)<<'|'<<hx(r.station)<<'|'<<hx(r.vessel)<<'|'<<hx(r.cargo)<<'|'<<(int)r.side<<'|'<<r.qty<<'|'<<r.unit<<'|'<<r.total<<'|'<<r.time<<'\n';return o.str();}
bool decode(const std::string&s,Record&r){std::vector<std::string>f=split(s);long long side=0;if(f.size()!=11||f[0]!="T"||!unhx(f[1],r.op)||!unhx(f[2],r.buyer)||!unhx(f[3],r.station)||!unhx(f[4],r.vessel)||!unhx(f[5],r.cargo)||!num(f[6],side)||!num(f[7],r.qty)||!num(f[8],r.unit)||!num(f[9],r.total)||!num(f[10],r.time))return false;if(side!=1&&side!=2)return false;r.side=(MarketSide)side;return true;}
bool readFile(const std::string&p,bool&exists,std::string&data){std::ifstream in(p.c_str(),std::ios::binary);if(!in){exists=false;data.clear();return true;}exists=true;data.assign(std::istreambuf_iterator<char>(in),std::istreambuf_iterator<char>());if(!in.good()&&!in.eof())return false;if(!data.empty()&&data[data.size()-1]!='\n'){std::string::size_type n=data.find_last_of('\n');std::size_t keep=n==std::string::npos?0:n+1;
#ifdef _WIN32
HANDLE h=CreateFileA(p.c_str(),GENERIC_WRITE,0,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);if(h==INVALID_HANDLE_VALUE)return false;LARGE_INTEGER q;q.QuadPart=keep;bool ok=SetFilePointerEx(h,q,0,FILE_BEGIN)&&SetEndOfFile(h)&&FlushFileBuffers(h);CloseHandle(h);if(!ok)return false;
#else
if(truncate(p.c_str(),(off_t)keep))return false;
#endif
data.resize(keep);}return true;}
bool append(const std::string&p,const std::string&s){
#ifdef _WIN32
HANDLE h=CreateFileA(p.c_str(),GENERIC_WRITE,FILE_SHARE_READ,0,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,0);if(h==INVALID_HANDLE_VALUE)return false;LARGE_INTEGER z;z.QuadPart=0;bool ok=SetFilePointerEx(h,z,0,FILE_END)!=0;DWORD n=0;if(ok)ok=WriteFile(h,s.data(),(DWORD)s.size(),&n,0)&&n==s.size();if(ok)ok=FlushFileBuffers(h)!=0;CloseHandle(h);return ok;
#else
int fd=open(p.c_str(),O_WRONLY|O_CREAT|O_APPEND,0666);if(fd<0)return false;std::size_t n=0;bool ok=true;while(n<s.size()){ssize_t k=write(fd,s.data()+n,s.size()-n);if(k<=0){ok=false;break;}n+=(std::size_t)k;}if(ok)ok=fsync(fd)==0;if(close(fd))ok=false;return ok;
#endif
}
Result replay(const Record&r,const CargoCatalog&cat,Ledger&l,CargoServiceInventory&i,CargoManifest&m){MarketQuote q;q.quoteId="replay-"+r.op;q.stationId=r.station;q.cargoId=r.cargo;q.side=r.side;q.quantity=r.qty;q.unitPrice=r.unit;q.totalPrice=r.total;q.simulationTime=r.time;MarketTradeEngine e;return e.execute(cat,q,r.op,r.buyer,r.station,l,i,m);}
}
Result MarketTradeJournalStore::load(const std::string&p,const CargoCatalog&cat,Ledger&ledger,CargoServiceInventory&inventory,CargoManifest&manifest)const{
bool exists=false;std::string data;if(!readFile(p,exists,data))return Result::failure("MARKET_JOURNAL_READ","Could not read or repair journal tail.");if(!exists)return Result::success();std::istringstream in(data);std::string line;if(!std::getline(in,line)||line!=header(inventory.serviceVesselId(),manifest.vesselId()))return Result::failure("MARKET_JOURNAL_HEADER","Market journal header or service ID is invalid.");Ledger l(ledger);CargoServiceInventory i(inventory);CargoManifest m(manifest);while(std::getline(in,line)){if(!line.empty()&&line[line.size()-1]=='\r')line.erase(line.size()-1);if(line.empty())continue;Record r;if(!decode(line,r))return Result::failure("MARKET_JOURNAL_CORRUPT","A complete market journal record is invalid.");if(r.station!=i.serviceVesselId()||r.vessel!=m.vesselId())return Result::failure("MARKET_JOURNAL_IDENTITY","Record belongs to another station or vessel; use a per-vessel journal.");Result x=replay(r,cat,l,i,m);if(!x.ok)return Result::failure(x.code,x.message);}ledger=l;inventory=i;manifest=m;return Result::success();}
Result MarketTradeJournalStore::apply(const std::string&p,const CargoCatalog&cat,const MarketQuote&q,const std::string&op,const std::string&buyer,const std::string&station,Ledger&ledger,CargoServiceInventory&inventory,CargoManifest&manifest)const{
if(q.stationId!=inventory.serviceVesselId())return Result::failure("MARKET_JOURNAL_SERVICE","Quote station does not match inventory.");Ledger l(ledger);CargoServiceInventory i(inventory);CargoManifest m(manifest);MarketTradeEngine e;Result x=e.execute(cat,q,op,buyer,station,l,i,m);if(!x.ok)return x;if(ledger.hasOperation(op))return Result::success();bool exists=false;std::string data;if(!readFile(p,exists,data))return Result::failure("MARKET_JOURNAL_TAIL","Could not inspect or repair market journal.");if(exists){std::string::size_type n=data.find('\n');if(n==std::string::npos||data.substr(0,n)!=header(inventory.serviceVesselId(),manifest.vesselId()))return Result::failure("MARKET_JOURNAL_HEADER","Existing market journal header is invalid.");}
Record r;r.op=op;r.buyer=buyer;r.station=station;r.vessel=manifest.vesselId();r.cargo=q.cargoId;r.side=q.side;r.qty=q.quantity;r.unit=q.unitPrice;r.total=q.totalPrice;r.time=q.simulationTime;std::string bytes=(exists?std::string():header(inventory.serviceVesselId(),manifest.vesselId())+"\n")+encode(r);if(!append(p,bytes))return Result::failure("MARKET_JOURNAL_COMMIT","Could not durably append market trade.");ledger=l;inventory=i;manifest=m;return Result::success();}
}
