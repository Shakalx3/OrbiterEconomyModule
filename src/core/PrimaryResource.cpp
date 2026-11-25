#include "PrimaryResource.h"
#include <algorithm>
#include <cmath>
#include <limits>
namespace oem {
StarterStockLine::StarterStockLine():cargoId(),quantity(0),unitMassKg(0.0){}
PrimaryResourceSource::PrimaryResourceSource():sourceId(),locationId(),cargoId(),unitMassKg(0.0),unitsPerCycle(0),cycleSeconds(86400),maximumStock(0),openingStock(0),lastProductionTime(0),enabled(true){}
PrimaryResourceIntent::PrimaryResourceIntent():operationId(),sourceId(),locationId(),cargoId(),quantity(0),unitMassKg(0.0),simulationTime(0){}
Result seedStarterStock(const StarterStockProfile& profile,CargoServiceInventory& inventory){
 if(profile.profileId.empty()||inventory.serviceVesselId().empty()||!inventory.history().empty()||!inventory.lines().empty())
  return Result::failure("STARTER_STOCK_NOT_EMPTY","Starter stock can only initialize a named, untouched, empty inventory.");
 std::map<std::string,bool> seen;
 for(std::size_t i=0;i<profile.lines.size();++i){const StarterStockLine& l=profile.lines[i];
  if(l.cargoId.empty()||l.quantity<=0||!(l.unitMassKg>0.0)||!std::isfinite(l.unitMassKg)||seen.find(l.cargoId)!=seen.end())
   return Result::failure("STARTER_STOCK_PROFILE","Starter stock profile contains an invalid or duplicate line.");
  seen[l.cargoId]=true;
 }
 CargoServiceInventory next(inventory);
 for(std::size_t i=0;i<profile.lines.size();++i){const StarterStockLine& l=profile.lines[i];Result r=next.setInitialStock(l.cargoId,l.quantity,l.unitMassKg);if(!r.ok)return r;}
 inventory=next;return Result::success();
}
Result PrimaryResourceBook::add(const PrimaryResourceSource& s){
 if(s.sourceId.empty()||s.locationId.empty()||s.cargoId.empty()||!(s.unitMassKg>0.0)||!std::isfinite(s.unitMassKg)||s.unitsPerCycle<=0||s.cycleSeconds<=0||s.maximumStock<=0||s.openingStock<0||s.openingStock>s.maximumStock||s.lastProductionTime<0)
  return Result::failure("PRIMARY_SOURCE_INVALID","Primary resource source requires IDs, positive mass, bounded output, cycle, capacity and non-negative time.");
 if(indexes_.find(s.sourceId)!=indexes_.end())return Result::failure("PRIMARY_SOURCE_DUPLICATE","Primary resource source ID already exists.");
 for(std::size_t i=0;i<sources_.size();++i)
  if(sources_[i].locationId==s.locationId&&sources_[i].cargoId==s.cargoId)
   return Result::failure("PRIMARY_SOURCE_DUPLICATE_LOCATION_CARGO","Only one primary source per cargo and location is allowed.");
 sources_.push_back(s);indexes_[s.sourceId]=sources_.size()-1;return Result::success();
}
const std::vector<PrimaryResourceSource>& PrimaryResourceBook::sources()const{return sources_;}
const PrimaryResourceSource* PrimaryResourceBook::find(const std::string& id)const{
 std::map<std::string,std::size_t>::const_iterator it=indexes_.find(id);
 return it==indexes_.end()?0:&sources_[it->second];
}
Result PrimaryResourceBook::restoreCommitted(const std::string& id,const std::string& location,
 const std::string& cargo,double mass,SimulationSeconds time){
 std::map<std::string,std::size_t>::iterator it=indexes_.find(id);
 if(it==indexes_.end())return Result::success();
 PrimaryResourceSource& s=sources_[it->second];
 if(s.locationId!=location||s.cargoId!=cargo||!std::isfinite(mass)||!(mass>0.0)||
    std::fabs(s.unitMassKg-mass)>1e-9||time<0)
  return Result::failure("PRIMARY_SOURCE_HISTORY_CONFLICT","A journaled production record conflicts with the current source definition.");
 if(time>s.lastProductionTime)s.lastProductionTime=time;
 return Result::success();
}
Result PrimaryResourceBook::initializeUnsetTimes(SimulationSeconds now,bool&changed){
 if(now<0)return Result::failure("PRIMARY_SOURCE_TIME","Simulation time cannot be negative.");
 changed=false;
 if(now==0)return Result::success();
 for(std::size_t i=0;i<sources_.size();++i)
  if(sources_[i].lastProductionTime==0){sources_[i].lastProductionTime=now;changed=true;}
 return Result::success();
}
Result PrimaryResourceBook::consumeOpeningStock(const std::string&location,bool&changed){
 if(location.empty())return Result::failure("PRIMARY_SOURCE_LOCATION","Location ID is required to consume opening stock.");
 changed=false;
 for(std::size_t i=0;i<sources_.size();++i)
  if(sources_[i].locationId==location&&sources_[i].openingStock>0){sources_[i].openingStock=0;changed=true;}
 return Result::success();
}
Result PrimaryResourceBook::planDue(SimulationSeconds now,const std::map<std::string,CargoServiceInventory>& inventories,std::vector<PrimaryResourceIntent>& out)const{
 if(now<0)return Result::failure("PRIMARY_SOURCE_TIME","Simulation time cannot be negative.");
 std::vector<PrimaryResourceIntent> pending;
 for(std::size_t i=0;i<sources_.size();++i){const PrimaryResourceSource&s=sources_[i];if(!s.enabled||now<s.lastProductionTime||now-s.lastProductionTime<s.cycleSeconds)continue;
  std::map<std::string,CargoServiceInventory>::const_iterator loc=inventories.find(s.locationId);if(loc==inventories.end())continue;
  const long long current=loc->second.quantity(s.cargoId);if(current>=s.maximumStock)continue;
  const long long amount=std::min(s.unitsPerCycle,s.maximumStock-current);if(amount<=0)continue;
  PrimaryResourceIntent p;p.sourceId=s.sourceId;p.locationId=s.locationId;p.cargoId=s.cargoId;p.quantity=amount;p.unitMassKg=s.unitMassKg;p.simulationTime=now;
  p.operationId="primary-"+s.sourceId+"-"+std::to_string((long long)now);pending.push_back(p);
 }
 out.insert(out.end(),pending.begin(),pending.end());return Result::success();
}
Result PrimaryResourceBook::markCommitted(const std::string& id,SimulationSeconds time){
 std::map<std::string,std::size_t>::iterator it=indexes_.find(id);if(it==indexes_.end())return Result::failure("PRIMARY_SOURCE_UNKNOWN","Primary resource source does not exist.");
 PrimaryResourceSource& s=sources_[it->second];if(time<s.lastProductionTime||time<0||time-s.lastProductionTime<s.cycleSeconds)return Result::failure("PRIMARY_SOURCE_NOT_DUE","Production timestamp is not due for this source.");
 s.lastProductionTime=time;return Result::success();
}
}