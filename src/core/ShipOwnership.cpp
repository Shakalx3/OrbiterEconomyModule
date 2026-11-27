#include "ShipOwnership.h"
#include <cstdio>
#include <fstream>
#include <sstream>
#include <locale>
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
    if (value.empty() || value.size()%2) return false;
    out.clear();
    for (std::size_t i=0;i<value.size();i+=2) {
        const char a=value[i],b=value[i+1];
        const int hi=a>='0'&&a<='9'?a-'0':a>='A'&&a<='F'?a-'A'+10:a>='a'&&a<='f'?a-'a'+10:-1;
        const int lo=b>='0'&&b<='9'?b-'0':b>='A'&&b<='F'?b-'A'+10:b>='a'&&b<='f'?b-'a'+10:-1;
        if(hi<0||lo<0)return false;
        out.push_back(static_cast<char>((hi<<4)|lo));
    }
    return !out.empty();
}
bool parseNumber(const std::string& s,long long& value) {
    if(s.empty())return false;
    char* end=0; const long long parsed=std::strtoll(s.c_str(),&end,10);
    if(!end||*end)return false; value=parsed; return true;
}
std::vector<std::string> split(const std::string& line) {
    std::vector<std::string> fields; std::istringstream in(line); std::string field;
    while(std::getline(in,field,'|'))fields.push_back(field);
    if(!line.empty()&&line[line.size()-1]=='|')fields.push_back(std::string());
    return fields;
}
bool replaceFile(const std::string& from,const std::string& to) {
#ifdef _WIN32
    return MoveFileExA(from.c_str(),to.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
#else
    return std::rename(from.c_str(),to.c_str())==0;
#endif
}
bool sameRecord(const OwnedVesselRecord&a,const OwnedVesselRecord&b) {
    return a.purchaseId==b.purchaseId&&a.vesselId==b.vesselId&&a.className==b.className&&
        a.acquisitionLocationId==b.acquisitionLocationId&&a.acquisitionPrice==b.acquisitionPrice&&
        a.acquiredAt==b.acquiredAt;
}
}

ShipDefinition::ShipDefinition():price(0),orbitalDelivery(false),surfaceDelivery(false){}
ShipPurchaseQuote::ShipPurchaseQuote():deliveryKind(SHIP_DELIVERY_ORBITAL),price(0),simulationTime(0){}

Result ShipCatalog::add(const ShipDefinition& d) {
    if(d.className.empty()||d.displayName.empty()||d.price<=0||
       (!d.orbitalDelivery&&!d.surfaceDelivery))
        return Result::failure("SHIP_CATALOG_DEFINITION","Ship class, display name, positive price and at least one delivery mode are required.");
    if(index_.find(d.className)!=index_.end())
        return Result::failure("SHIP_CATALOG_DUPLICATE","Ship class is already present in the catalogue.");
    index_[d.className]=entries_.size(); entries_.push_back(d); return Result::success();
}
bool ShipCatalog::find(const std::string& name,ShipDefinition& d)const{
    std::map<std::string,std::size_t>::const_iterator it=index_.find(name);
    if(it==index_.end())return false; d=entries_[it->second]; return true;
}
const std::vector<ShipDefinition>& ShipCatalog::entries()const{return entries_;}
void ShipCatalog::clear(){entries_.clear();index_.clear();}

Result createShipPurchaseQuote(const ShipCatalog& catalog,const std::string& purchaseId,
        const std::string& className,const std::string& locationId,ShipDeliveryKind kind,
        Credits balance,SimulationSeconds time,ShipPurchaseQuote& quote) {
    if(purchaseId.empty()||className.empty()||locationId.empty()||time<0||balance<0)
        return Result::failure("SHIP_PURCHASE_INPUT","Purchase ID, class, location and valid account/time values are required.");
    ShipDefinition definition;
    if(!catalog.find(className,definition))
        return Result::failure("SHIP_CLASS_UNAVAILABLE","Requested vessel class is not in the available ship catalogue.");
    if((kind==SHIP_DELIVERY_ORBITAL&&!definition.orbitalDelivery)||
       (kind==SHIP_DELIVERY_SURFACE&&!definition.surfaceDelivery)||
       (kind!=SHIP_DELIVERY_ORBITAL&&kind!=SHIP_DELIVERY_SURFACE))
        return Result::failure("SHIP_DELIVERY_UNAVAILABLE","This vessel class is not compatible with the selected delivery location.");
    if(balance<definition.price)
        return Result::failure("SHIP_INSUFFICIENT_FUNDS","Company account has insufficient funds for this vessel.");
    ShipPurchaseQuote candidate; candidate.purchaseId=purchaseId; candidate.className=className;
    candidate.displayName=definition.displayName; candidate.locationId=locationId;
    candidate.deliveryKind=kind; candidate.price=definition.price; candidate.simulationTime=time;
    quote=candidate; return Result::success();
}

Result PlayerFleetRegistry::registerPurchase(const OwnedVesselRecord& r) {
    if(r.purchaseId.empty()||r.vesselId.empty()||r.className.empty()||
       r.acquisitionLocationId.empty()||r.acquisitionPrice<=0||r.acquiredAt<0)
        return Result::failure("PLAYER_FLEET_RECORD","A complete purchase record with positive price is required.");
    std::map<std::string,std::size_t>::const_iterator byPurchase=purchaseIndex_.find(r.purchaseId);
    if(byPurchase!=purchaseIndex_.end())
        return sameRecord(vessels_[byPurchase->second],r)?Result::success():
            Result::failure("PLAYER_FLEET_PURCHASE_CONFLICT","Purchase ID is already associated with different vessel data.");
    if(vesselIndex_.find(r.vesselId)!=vesselIndex_.end())
        return Result::failure("PLAYER_FLEET_VESSEL_CONFLICT","Vessel is already registered to another purchase.");
    purchaseIndex_[r.purchaseId]=vessels_.size(); vesselIndex_[r.vesselId]=vessels_.size();
    vessels_.push_back(r); return Result::success();
}
bool PlayerFleetRegistry::isOwned(const std::string&id)const{return vesselIndex_.find(id)!=vesselIndex_.end();}
bool PlayerFleetRegistry::findByVessel(const std::string&id,OwnedVesselRecord&r)const{
    std::map<std::string,std::size_t>::const_iterator it=vesselIndex_.find(id);
    if(it==vesselIndex_.end())return false;r=vessels_[it->second];return true;
}
bool PlayerFleetRegistry::findByPurchase(const std::string&id,OwnedVesselRecord&r)const{
    std::map<std::string,std::size_t>::const_iterator it=purchaseIndex_.find(id);
    if(it==purchaseIndex_.end())return false;r=vessels_[it->second];return true;
}
const std::vector<OwnedVesselRecord>& PlayerFleetRegistry::vessels()const{return vessels_;}
void PlayerFleetRegistry::clear(){vessels_.clear();vesselIndex_.clear();purchaseIndex_.clear();}

Result PlayerFleetRegistry::load(const std::string& path) {
    std::ifstream in(path.c_str(),std::ios::in|std::ios::binary);
    if(!in.is_open()){clear();return Result::success();}
    std::string raw((std::istreambuf_iterator<char>(in)),std::istreambuf_iterator<char>());
    std::string normalized;
    if(raw.size()>=2&&static_cast<unsigned char>(raw[0])==0xFF&&static_cast<unsigned char>(raw[1])==0xFE){
        if((raw.size()-2)%2)return Result::failure("PLAYER_FLEET_ENCODING","Invalid UTF-16LE player fleet registry.");
        for(std::size_t i=2;i<raw.size();i+=2){const unsigned char lo=static_cast<unsigned char>(raw[i]),hi=static_cast<unsigned char>(raw[i+1]);if(hi!=0)return Result::failure("PLAYER_FLEET_ENCODING","Player fleet registry must contain ASCII-compatible text.");normalized.push_back(static_cast<char>(lo));}
    }else{normalized=raw;if(normalized.size()>=3&&static_cast<unsigned char>(normalized[0])==0xEF&&static_cast<unsigned char>(normalized[1])==0xBB&&static_cast<unsigned char>(normalized[2])==0xBF)normalized.erase(0,3);}
    std::istringstream text(normalized);std::string header;
    if(!std::getline(text,header))return Result::failure("PLAYER_FLEET_HEADER","Player fleet registry is empty.");
    if(!header.empty()&&header[header.size()-1]=='\r')header.erase(header.size()-1);
    if(header!="OEM_PLAYER_FLEET|1")return Result::failure("PLAYER_FLEET_HEADER","Unsupported player fleet registry header or version.");
    PlayerFleetRegistry candidate;std::string line;
    while(std::getline(text,line)){
        if(!line.empty()&&line[line.size()-1]=='\r')line.erase(line.size()-1);if(line.empty())continue;
        const std::vector<std::string> f=split(line);long long price=0,time=0;OwnedVesselRecord r;
        if(f.size()!=7||f[0]!="V"||!hexDecode(f[1],r.purchaseId)||!hexDecode(f[2],r.vesselId)||
           !hexDecode(f[3],r.className)||!hexDecode(f[4],r.acquisitionLocationId)||
           !parseNumber(f[5],price)||!parseNumber(f[6],time))
            return Result::failure("PLAYER_FLEET_RECORD","Invalid player fleet record.");
        r.acquisitionPrice=price;r.acquiredAt=time;const Result added=candidate.registerPurchase(r);
        if(!added.ok)return Result::failure("PLAYER_FLEET_RECORD",added.message);
    }
    *this=candidate;return Result::success();
}
Result PlayerFleetRegistry::save(const std::string& path)const{
    const std::string temporary=path+".tmp";
    {std::ofstream out(temporary.c_str(),std::ios::out|std::ios::binary|std::ios::trunc);
     if(!out.is_open())return Result::failure("PLAYER_FLEET_OPEN","Could not create temporary player fleet registry.");
     out<<"OEM_PLAYER_FLEET|1\n";
     for(std::size_t i=0;i<vessels_.size();++i){const OwnedVesselRecord&r=vessels_[i];
        out<<"V|"<<hexEncode(r.purchaseId)<<"|"<<hexEncode(r.vesselId)<<"|"<<hexEncode(r.className)<<"|"<<hexEncode(r.acquisitionLocationId)<<"|"<<r.acquisitionPrice<<"|"<<r.acquiredAt<<"\n";}
     out.flush();if(!out.good())return Result::failure("PLAYER_FLEET_WRITE","Could not write player fleet registry.");
     out.close();if(out.fail())return Result::failure("PLAYER_FLEET_CLOSE","Could not close player fleet registry.");}
    if(!replaceFile(temporary,path)){std::remove(temporary.c_str());return Result::failure("PLAYER_FLEET_REPLACE","Could not atomically replace player fleet registry.");}
    return Result::success();
}

} // namespace oem
