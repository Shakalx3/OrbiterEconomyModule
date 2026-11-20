#include "EconomyJournal.h"
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
const char* kHeader="OEM_ECONOMY_JOURNAL|1";
struct Entry{char type;std::string op,buyer,station,vessel,cargo;int side;long long qty;Credits unit,total;SimulationSeconds time;CargoTransfer transfer;};
std::string encodeX(const CargoTransfer&t){std::ostringstream o;o<<"X|"<<hx(t.transferId)<<'|'<<(int)t.direction<<'|'<<hx(t.vesselId)<<'|'<<hx(t.locationId)<<'|'<<hx(t.cargoId)<<'|'<<t.quantity<<'|'<<std::setprecision(17)<<t.unitMassKg<<'|'<<t.simulationTime<<'\n';return o.str();}
std::string encodeT(const Entry&r){std::ostringstream o;o<<"T|"<<hx(r.op)<<'|'<<hx(r.buyer)<<'|'<<hx(r.station)<<'|'<<hx(r.vessel)<<'|'<<hx(r.cargo)<<'|'<<r.side<<'|'<<r.qty<<'|'<<r.unit<<'|'<<r.total<<'|'<<r.time<<'\n';return o.str();}
bool decode(const std::string&s,Entry&r){std::vector<std::string>f=split(s);long long a=0,b=0,c=0,d=0,e=0;
 if(f.size()==9&&f[0]=="X"){r.type='X';if(!unhx(f[1],r.transfer.transferId)||!num(f[2],a)||(a!=CARGO_LOAD&&a!=CARGO_UNLOAD)||!unhx(f[3],r.transfer.vesselId)||!unhx(f[4],r.transfer.locationId)||!unhx(f[5],r.transfer.cargoId)||!num(f[6],b)||!dbl(f[7],r.transfer.unitMassKg)||!num(f[8],c))return false;r.transfer.direction=(CargoTransferDirection)a;r.transfer.quantity=b;r.transfer.simulationTime=c;return true;}
 if(f.size()!=11||f[0]!="T"||!unhx(f[1],r.op)||!unhx(f[2],r.buyer)||!unhx(f[3],r.station)||!unhx(f[4],r.vessel)||!unhx(f[5],r.cargo)||!num(f[6],a)||!num(f[7],b)||!num(f[8],c)||!num(f[9],d)||!num(f[10],e)||(a!=MARKET_BUY_FROM_STATION&&a!=MARKET_SELL_TO_STATION))return false;
 r.type='T';r.side=(int)a;r.qty=b;r.unit=c;r.total=d;r.time=e;return true;}
std::string opOf(const Entry&r){return r.type=='X'?r.transfer.transferId:r.op;}
bool readJournal(const std::string&p,bool&exists,std::string&data){std::ifstream in(p.c_str(),std::ios::binary);if(!in){exists=false;data.clear();return true;}exists=true;data.assign(std::istreambuf_iterator<char>(in),std::istreambuf_iterator<char>());if(!in.good()&&!in.eof())return false;if(!data.empty()&&data[data.size()-1]!='\n'){std::string::size_type n=data.find_last_of('\n');std::size_t keep=n==std::string::npos?0:n+1;
#ifdef _WIN32
 HANDLE h=CreateFileA(p.c_str(),GENERIC_WRITE,0,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);if(h==INVALID_HANDLE_VALUE)return false;LARGE_INTEGER q;q.QuadPart=keep;bool ok=SetFilePointerEx(h,q,0,FILE_BEGIN)&&SetEndOfFile(h)&&FlushFileBuffers(h);CloseHandle(h);if(!ok)return false;
#else
 int fd=open(p.c_str(),O_WRONLY);if(fd<0)return false;bool ok=ftruncate(fd,(off_t)keep)==0&&fsync(fd)==0;if(close(fd))ok=false;if(!ok)return false;
#endif
 data.resize(keep);}return true;}
bool appendDurable(const std::string&p,const std::string&s){
#ifdef _WIN32
 HANDLE h=CreateFileA(p.c_str(),GENERIC_WRITE,FILE_SHARE_READ,0,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,0);if(h==INVALID_HANDLE_VALUE)return false;LARGE_INTEGER z;z.QuadPart=0;bool ok=SetFilePointerEx(h,z,0,FILE_END)!=0;DWORD n=0;if(ok)ok=WriteFile(h,s.data(),(DWORD)s.size(),&n,0)&&n==s.size();if(ok)ok=FlushFileBuffers(h)!=0;CloseHandle(h);return ok;
#else
 int fd=open(p.c_str(),O_WRONLY|O_CREAT|O_APPEND,0666);if(fd<0)return false;std::size_t n=0;bool ok=true;while(n<s.size()){ssize_t k=write(fd,s.data()+n,s.size()-n);if(k<=0){ok=false;break;}n+=(std::size_t)k;}if(ok)ok=fsync(fd)==0;if(close(fd))ok=false;return ok;
#endif
}
Result scanIds(const std::string&data,std::map<std::string,std::string>&ids){std::istringstream in(data);std::string line;if(!std::getline(in,line)||line!=kHeader)return Result::failure("ECONOMY_JOURNAL_HEADER","Global economy journal header or version is invalid.");while(std::getline(in,line)){if(!line.empty()&&line[line.size()-1]=='\r')line.erase(line.size()-1);if(line.empty())continue;Entry r;if(!decode(line,r)||opOf(r).empty())return Result::failure("ECONOMY_JOURNAL_CORRUPT","A complete global economy record is invalid.");std::map<std::string,std::string>::const_iterator it=ids.find(opOf(r));if(it!=ids.end()){if(it->second!=line)return Result::failure("ECONOMY_OPERATION_CONFLICT","An operation ID is reused by different global journal records.");}else ids[opOf(r)]=line;}return Result::success();}
}
Result EconomyJournalStore::load(const std::string&p,const CargoCatalog&cat,Ledger&ledger,std::map<std::string,CargoServiceInventory>&inventories,std::map<std::string,CargoManifest>&manifests)const{
 bool exists=false;std::string data;if(!readJournal(p,exists,data))return Result::failure("ECONOMY_JOURNAL_READ","Could not read or repair the global journal tail.");if(!exists)return Result::success();
 std::map<std::string,std::string> ids;Result valid=scanIds(data,ids);if(!valid.ok)return valid;
 Ledger nextLedger(ledger);std::map<std::string,CargoServiceInventory> nextInventories(inventories);std::map<std::string,CargoManifest> nextManifests(manifests);
 std::istringstream in(data);std::string line;std::getline(in,line);std::map<std::string,std::string> applied;
 while(std::getline(in,line)){if(!line.empty()&&line[line.size()-1]=='\r')line.erase(line.size()-1);if(line.empty())continue;Entry r;if(!decode(line,r))return Result::failure("ECONOMY_JOURNAL_CORRUPT","Invalid complete record.");const std::string op=opOf(r);if(applied.find(op)!=applied.end())continue;applied[op]=line;
 std::map<std::string,CargoServiceInventory>::iterator si=nextInventories.find(r.type=='X'?r.transfer.locationId:r.station);if(si==nextInventories.end())return Result::failure("ECONOMY_UNKNOWN_STATION","Journal references a station without explicit opening inventory.");
 const std::string vessel=r.type=='X'?r.transfer.vesselId:r.vessel;std::map<std::string,CargoManifest>::iterator mi=nextManifests.find(vessel);if(mi==nextManifests.end())mi=nextManifests.insert(std::make_pair(vessel,CargoManifest(vessel))).first;
 Result appliedResult;
 if(r.type=='X'){if(r.transfer.locationId!=si->first||r.transfer.vesselId!=vessel)return Result::failure("ECONOMY_EXCHANGE_IDENTITY","Exchange identity does not match its projections.");appliedResult=si->second.apply(r.transfer);if(appliedResult.ok)appliedResult=r.transfer.direction==CARGO_LOAD?mi->second.load(r.transfer):mi->second.unload(r.transfer);}
 else {MarketQuote q;q.quoteId="replay-"+r.op;q.stationId=r.station;q.cargoId=r.cargo;q.side=(MarketSide)r.side;q.quantity=r.qty;q.unitPrice=r.unit;q.totalPrice=r.total;q.simulationTime=r.time;MarketTradeEngine engine;appliedResult=engine.execute(cat,q,r.op,r.buyer,nextLedger,si->second,mi->second);}
 if(!appliedResult.ok)return Result::failure(appliedResult.code,appliedResult.message);
 }
 ledger=nextLedger;inventories.swap(nextInventories);manifests.swap(nextManifests);return Result::success();}
Result EconomyJournalStore::applyExchange(const std::string&p,std::map<std::string,CargoServiceInventory>&inventories,CargoManifest&manifest,const CargoTransfer&t)const{
 std::map<std::string,CargoServiceInventory>::iterator it=inventories.find(t.locationId);if(it==inventories.end()||t.vesselId!=manifest.vesselId())return Result::failure("ECONOMY_EXCHANGE_IDENTITY","Transfer must reference a configured station and the selected vessel.");
 CargoServiceInventory nextStock(it->second);CargoManifest nextManifest(manifest);Result x=nextStock.apply(t);if(!x.ok)return x;x=t.direction==CARGO_LOAD?nextManifest.load(t):nextManifest.unload(t);if(!x.ok)return x;
 bool exists=false;std::string data;if(!readJournal(p,exists,data))return Result::failure("ECONOMY_JOURNAL_READ","Could not inspect global journal before append.");std::map<std::string,std::string>ids;if(exists){x=scanIds(data,ids);if(!x.ok)return x;}const std::string record=encodeX(t);std::map<std::string,std::string>::const_iterator prior=ids.find(t.transferId);if(prior!=ids.end())return prior->second+"\n"==record?Result::success():Result::failure("ECONOMY_OPERATION_CONFLICT","Transfer ID already belongs to a different global operation.");
 if(!appendDurable(p,(exists?std::string():std::string(kHeader)+"\n")+record))return Result::failure("ECONOMY_JOURNAL_COMMIT","Could not durably append global exchange.");it->second=nextStock;manifest=nextManifest;return Result::success();}
Result EconomyJournalStore::applyTrade(const std::string&p,const CargoCatalog&cat,const MarketQuote&q,const std::string&op,const std::string&buyer,Ledger&ledger,std::map<std::string,CargoServiceInventory>&inventories,CargoManifest&manifest)const{
 std::map<std::string,CargoServiceInventory>::iterator it=inventories.find(q.stationId);if(it==inventories.end())return Result::failure("ECONOMY_UNKNOWN_STATION","Trade station has no configured inventory.");Ledger nextLedger(ledger);CargoServiceInventory nextStock(it->second);CargoManifest nextManifest(manifest);MarketTradeEngine engine;Result x=engine.execute(cat,q,op,buyer,nextLedger,nextStock,nextManifest);if(!x.ok)return x;
 bool exists=false;std::string data;if(!readJournal(p,exists,data))return Result::failure("ECONOMY_JOURNAL_READ","Could not inspect global journal before append.");std::map<std::string,std::string>ids;if(exists){x=scanIds(data,ids);if(!x.ok)return x;}
 Entry r;r.type='T';r.op=op;r.buyer=buyer;r.station=q.stationId;r.vessel=manifest.vesselId();r.cargo=q.cargoId;r.side=(int)q.side;r.qty=q.quantity;r.unit=q.unitPrice;r.total=q.totalPrice;r.time=q.simulationTime;const std::string record=encodeT(r);std::map<std::string,std::string>::const_iterator prior=ids.find(op);if(prior!=ids.end())return prior->second+"\n"==record?Result::success():Result::failure("ECONOMY_OPERATION_CONFLICT","Operation ID already belongs to a different global operation.");
 if(!appendDurable(p,(exists?std::string():std::string(kHeader)+"\n")+record))return Result::failure("ECONOMY_JOURNAL_COMMIT","Could not durably append global market trade.");ledger=nextLedger;it->second=nextStock;manifest=nextManifest;return Result::success();}
}
