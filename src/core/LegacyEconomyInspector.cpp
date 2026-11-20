#include "LegacyEconomyInspector.h"
#include "CargoManifest.h"
#include "Market.h"
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <map>
namespace oem { namespace {
std::string hex(const std::string&s){static const char d[]="0123456789ABCDEF";std::string o;for(std::size_t i=0;i<s.size();++i){unsigned char c=(unsigned char)s[i];o+=d[c>>4];o+=d[c&15];}return o;}
bool unhex(const std::string&s,std::string&o){if(s.empty()||s.size()%2)return false;o.clear();for(std::size_t i=0;i<s.size();i+=2){char a=s[i],b=s[i+1];int x=a>='0'&&a<='9'?a-'0':a>='A'&&a<='F'?a-'A'+10:a>='a'&&a<='f'?a-'a'+10:-1;int y=b>='0'&&b<='9'?b-'0':b>='A'&&b<='F'?b-'A'+10:b>='a'&&b<='f'?b-'a'+10:-1;if(x<0||y<0)return false;o+=(char)((x<<4)|y);}return true;}
std::vector<std::string> split(const std::string&s){std::vector<std::string>v;std::istringstream in(s);std::string x;while(std::getline(in,x,'|'))v.push_back(x);return v;}
bool integer(const std::string&s){if(s.empty())return false;char*e=0;std::strtoll(s.c_str(),&e,10);return e&&*e=='\0';}
bool real(const std::string&s){if(s.empty())return false;char*e=0;double v=std::strtod(s.c_str(),&e);return e&&*e=='\0'&&std::isfinite(v)&&v>0.0;}
bool record(const std::string&line,LegacyJournalKind kind,std::string&id){
 std::vector<std::string>f=split(line);std::string decoded;
 if(kind==LEGACY_CARGO_EXCHANGE){long long direction=0;if(f.size()!=9||f[0]!="X"||!unhex(f[1],id)||!integer(f[2])||!unhex(f[3],decoded)||!unhex(f[4],decoded)||!unhex(f[5],decoded)||!integer(f[6])||!real(f[7])||!integer(f[8]))return false;direction=std::strtoll(f[2].c_str(),0,10);long long quantity=std::strtoll(f[6].c_str(),0,10),time=std::strtoll(f[8].c_str(),0,10);return !id.empty()&&quantity>0&&time>=0&&(direction==(long long)CARGO_LOAD||direction==(long long)CARGO_UNLOAD);}
 if(f.size()!=11||f[0]!="T"||!unhex(f[1],id)||!unhex(f[2],decoded)||!unhex(f[3],decoded)||!unhex(f[4],decoded)||!unhex(f[5],decoded)||!integer(f[6])||!integer(f[7])||!integer(f[8])||!integer(f[9])||!integer(f[10]))return false;
 const long long side=std::strtoll(f[6].c_str(),0,10),qty=std::strtoll(f[7].c_str(),0,10),unit=std::strtoll(f[8].c_str(),0,10),total=std::strtoll(f[9].c_str(),0,10),time=std::strtoll(f[10].c_str(),0,10);return !id.empty()&&qty>0&&unit>=0&&total>=0&&time>=0&&(side==MARKET_BUY_FROM_STATION||side==MARKET_SELL_TO_STATION);
}
}
Result LegacyEconomyJournalInspector::inspect(const std::vector<LegacyJournalInput>&inputs,LegacyEconomyMigrationReport&report)const{
 LegacyEconomyMigrationReport next;std::map<std::string,std::string> ids;bool invalid=false;
 for(std::size_t i=0;i<inputs.size();++i){const LegacyJournalInput&in=inputs[i];LegacyJournalFinding f;f.path=in.path;f.stationId=in.stationId;f.kind=in.kind;
  std::ifstream file(in.path.c_str(),std::ios::binary);if(!file){f.exists=false;next.findings.push_back(f);continue;}f.exists=true;
  std::string data((std::istreambuf_iterator<char>(file)),std::istreambuf_iterator<char>());if(!file.good()&&!file.eof()){f.valid=false;f.code="LEGACY_READ";f.message="Could not read the complete legacy file.";invalid=true;next.findings.push_back(f);continue;}
  if(!data.empty()&&data[data.size()-1]!='\n'){f.hasIncompleteTail=true;std::string::size_type n=data.find_last_of('\n');data.resize(n==std::string::npos?0:n+1);}
  std::istringstream stream(data);std::string line;std::getline(stream,line);if(!line.empty()&&line[line.size()-1]=='\r')line.erase(line.size()-1);
  const std::string expected=in.kind==LEGACY_CARGO_EXCHANGE?"OEM_CARGO_EXCHANGE|1|"+hex(in.stationId):"OEM_MARKET_TRADES|1|"+hex(in.stationId);
  if(line!=expected){f.valid=false;f.code="LEGACY_HEADER";f.message="Legacy journal header or station identity does not match.";invalid=true;next.findings.push_back(f);continue;}
  while(std::getline(stream,line)){if(!line.empty()&&line[line.size()-1]=='\r')line.erase(line.size()-1);if(line.empty())continue;std::string op;if(!record(line,in.kind,op)||op.empty()){f.valid=false;f.code="LEGACY_RECORD";f.message="A complete legacy record is malformed.";invalid=true;break;}
   std::map<std::string,std::string>::const_iterator prior=ids.find(op);if(prior!=ids.end()&&prior->second!=line){f.valid=false;f.code="LEGACY_OPERATION_CONFLICT";f.message="A legacy operation ID is reused with different record facts.";invalid=true;break;}ids[op]=line;++f.completeRecords;
  }
  if(f.completeRecords>0){++next.activeStreams;next.totalRecords+=f.completeRecords;}
  next.findings.push_back(f);
 }
 next.safeSingleStream=!invalid&&next.activeStreams<=1;
 if(invalid)next.summary="Migration is blocked: at least one legacy file is unreadable, malformed, or has a conflicting operation ID.";
 else if(next.activeStreams>1)next.summary="Migration is blocked: multiple non-empty legacy journals have no provable global chronology.";
 else if(next.activeStreams==1)next.summary="One non-empty legacy stream was found. Its internal order is preserved, but manifests and explicit opening-state compatibility must still be verified before migration.";
 else next.summary="No non-empty legacy streams were found. Explicit opening state is required before creating the global journal.";
 report=next;return invalid?Result::failure("LEGACY_ECONOMY_INVALID",next.summary):Result::success();
}
}
