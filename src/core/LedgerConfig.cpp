#include "LedgerConfig.h"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#endif
namespace oem { namespace {
std::string hex(const std::string&s){static const char d[]="0123456789ABCDEF";std::string o;for(std::size_t i=0;i<s.size();++i){unsigned char c=(unsigned char)s[i];o+=d[c>>4];o+=d[c&15];}return o;}
bool unhex(const std::string&s,std::string&o){if(s.empty()||s.size()%2)return false;o.clear();for(std::size_t i=0;i<s.size();i+=2){int a=s[i]>='0'&&s[i]<='9'?s[i]-'0':s[i]>='A'&&s[i]<='F'?s[i]-'A'+10:s[i]>='a'&&s[i]<='f'?s[i]-'a'+10:-1;int b=s[i+1]>='0'&&s[i+1]<='9'?s[i+1]-'0':s[i+1]>='A'&&s[i+1]<='F'?s[i+1]-'A'+10:s[i+1]>='a'&&s[i+1]<='f'?s[i+1]-'a'+10:-1;if(a<0||b<0)return false;o+=(char)((a<<4)|b);}return !o.empty();}
bool number(const std::string&s,long long&v){if(s.empty())return false;char*e=0;long long n=std::strtoll(s.c_str(),&e,10);if(!e||*e)return false;v=n;return true;}
std::vector<std::string> split(const std::string&s){std::vector<std::string>v;std::istringstream in(s);std::string x;while(std::getline(in,x,'|'))v.push_back(x);return v;}
bool replaceFile(const std::string&a,const std::string&b){
#ifdef _WIN32
return MoveFileExA(a.c_str(),b.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
#else
return std::rename(a.c_str(),b.c_str())==0;
#endif
}
}
Result LedgerConfigStore::load(const std::string&p,Ledger&ledger)const{
 std::ifstream in(p.c_str(),std::ios::binary);if(!in)return Result::failure("LEDGER_CONFIG_MISSING","Opening account configuration is missing; no default balances are created.");
 std::string line;if(!std::getline(in,line))return Result::failure("LEDGER_CONFIG_HEADER","Account configuration is empty.");if(!line.empty()&&line.back()=='\r')line.pop_back();if(line!="OEM_ACCOUNTS|1")return Result::failure("LEDGER_CONFIG_HEADER","Account configuration header or version is invalid.");
 Ledger candidate;while(std::getline(in,line)){if(!line.empty()&&line.back()=='\r')line.pop_back();if(line.empty())continue;std::vector<std::string> f=split(line);std::string id;long long balance=0;if(f.size()!=3||f[0]!="A"||!unhex(f[1],id)||!number(f[2],balance))return Result::failure("LEDGER_CONFIG_RECORD","Account configuration contains an invalid record.");Result r=candidate.createAccount(id,balance);if(!r.ok)return Result::failure("LEDGER_CONFIG_RECORD",r.message);}
 ledger=candidate;return Result::success();
}
Result LedgerConfigStore::save(const std::string&p,const Ledger&ledger)const{
 if(!ledger.entries().empty())return Result::failure("LEDGER_CONFIG_ACTIVE","Opening balances cannot be saved from a ledger that already contains transactions.");
 const std::string tmp=p+".tmp";std::ofstream out(tmp.c_str(),std::ios::binary|std::ios::trunc);if(!out)return Result::failure("LEDGER_CONFIG_OPEN","Could not create temporary account configuration.");
 out<<"OEM_ACCOUNTS|1\n";
 const std::map<std::string, Credits>& accounts=ledger.accounts();
 for(std::map<std::string, Credits>::const_iterator it=accounts.begin();it!=accounts.end();++it)out<<"A|"<<hex(it->first)<<'|'<<it->second<<"\\n";
 out.flush();if(!out.good()){out.close();std::remove(tmp.c_str());return Result::failure("LEDGER_CONFIG_WRITE","Could not write account configuration.");}
 out.close();if(out.fail()){std::remove(tmp.c_str());return Result::failure("LEDGER_CONFIG_CLOSE","Could not close account configuration.");}
 if(!replaceFile(tmp,p)){std::remove(tmp.c_str());return Result::failure("LEDGER_CONFIG_REPLACE","Could not atomically replace account configuration.");}
 return Result::success();
}
}
