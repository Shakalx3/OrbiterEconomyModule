#include "MissionObjectiveBook.h"
namespace oem {
MissionObjectiveContract::MissionObjectiveContract() : objective(), reward(0), status(MISSION_OBJECTIVE_ACCEPTED), completionEventId(), completedAt(0) {}
MissionObjectiveBook::MissionObjectiveBook() : indexes_(), missions_() {}
Result MissionObjectiveBook::accept(const MissionObjective& objective, Credits reward) {
    const Result valid=validateMissionObjective(objective); if(!valid.ok)return valid;
    if(objective.kind==MISSION_OBJECTIVE_DELIVER_CARGO)return Result::failure("MISSION_OBJECTIVE_KIND","Cargo delivery remains managed by the delivery-contract book.");
    if(reward<=0)return Result::failure("MISSION_OBJECTIVE_REWARD","Mission reward must be positive.");
    std::map<std::string,std::size_t>::const_iterator prior=indexes_.find(objective.missionId);
    if(prior!=indexes_.end()){const MissionObjectiveContract& old=missions_[prior->second];const MissionObjective& a=old.objective;
        const bool same=a.vesselId==objective.vesselId&&a.kind==objective.kind&&a.targetLocationId==objective.targetLocationId&&a.targetBodyId==objective.targetBodyId&&a.acceptedAt==objective.acceptedAt&&a.deadline==objective.deadline&&old.reward==reward;
        return same?Result::success():Result::failure("MISSION_OBJECTIVE_CONFLICT","Mission ID already belongs to different terms.");}
    MissionObjectiveContract item;item.objective=objective;item.reward=reward;item.status=MISSION_OBJECTIVE_ACCEPTED;
    missions_.push_back(item);indexes_[objective.missionId]=missions_.size()-1;return Result::success();
}
Result MissionObjectiveBook::observe(const SimulationEvent& event,const CargoManifest& manifest){
    if(event.eventId.empty()||event.vesselId.empty())return Result::failure("MISSION_OBJECTIVE_EVENT","Verified event ID and vessel ID are required.");
    bool completed=false;
    for(std::size_t i=0;i<missions_.size();++i){MissionObjectiveContract& item=missions_[i];if(item.status!=MISSION_OBJECTIVE_ACCEPTED)continue;
        if(missionObjectiveMatchesEvent(item.objective,event,manifest)){item.status=MISSION_OBJECTIVE_COMPLETED;item.completionEventId=event.eventId;item.completedAt=event.simulationTime;completed=true;}
        else if(item.objective.vesselId==event.vesselId&&item.objective.deadline>0&&event.simulationTime>item.objective.deadline)item.status=MISSION_OBJECTIVE_EXPIRED;}
    return completed?Result::success():Result::failure("MISSION_OBJECTIVE_NO_MATCH","Event does not complete an accepted objective.");
}
Result MissionObjectiveBook::settle(const std::string& id,const std::string& accountId,Ledger& ledger){
    std::map<std::string,std::size_t>::const_iterator it=indexes_.find(id);if(it==indexes_.end())return Result::failure("MISSION_OBJECTIVE_NOT_FOUND","Mission does not exist.");
    MissionObjectiveContract& item=missions_[it->second];if(item.status==MISSION_OBJECTIVE_SETTLED)return Result::success();
    if(item.status!=MISSION_OBJECTIVE_COMPLETED)return Result::failure("MISSION_OBJECTIVE_NOT_COMPLETE","Only a completed mission can be settled.");
    Result paid=ledger.applyExternal("mission-objective-payout:"+id,accountId,item.reward,item.completedAt,"Mission objective "+id);
    if(!paid.ok)return paid;item.status=MISSION_OBJECTIVE_SETTLED;return Result::success();
}
Result MissionObjectiveBook::restoreCompleted(const std::string& id,const std::string& eventId,SimulationSeconds time){
    std::map<std::string,std::size_t>::iterator it=indexes_.find(id);if(it==indexes_.end())return Result::failure("MISSION_OBJECTIVE_NOT_FOUND","Mission does not exist.");
    if(eventId.empty()||time<0)return Result::failure("MISSION_OBJECTIVE_COMPLETION","Completion event and time are required.");
    MissionObjectiveContract& item=missions_[it->second];
    if(item.status==MISSION_OBJECTIVE_COMPLETED||item.status==MISSION_OBJECTIVE_SETTLED)return item.completionEventId==eventId&&item.completedAt==time?Result::success():Result::failure("MISSION_OBJECTIVE_COMPLETION_CONFLICT","Mission has different completion evidence.");
    if(item.status!=MISSION_OBJECTIVE_ACCEPTED||time<item.objective.acceptedAt||(item.objective.deadline>0&&time>item.objective.deadline))return Result::failure("MISSION_OBJECTIVE_COMPLETION_INVALID","Completion is inconsistent with mission terms.");
    item.status=MISSION_OBJECTIVE_COMPLETED;item.completionEventId=eventId;item.completedAt=time;return Result::success();
}
Result MissionObjectiveBook::restoreSettled(const std::string& id){
    std::map<std::string,std::size_t>::iterator it=indexes_.find(id);if(it==indexes_.end())return Result::failure("MISSION_OBJECTIVE_NOT_FOUND","Mission does not exist.");
    MissionObjectiveContract& item=missions_[it->second];if(item.status==MISSION_OBJECTIVE_SETTLED)return Result::success();
    if(item.status!=MISSION_OBJECTIVE_COMPLETED)return Result::failure("MISSION_OBJECTIVE_NOT_COMPLETE","Only a completed mission can be restored as settled.");
    item.status=MISSION_OBJECTIVE_SETTLED;return Result::success();
}
Result MissionObjectiveBook::restoreExpired(const std::string& id){
    std::map<std::string,std::size_t>::iterator it=indexes_.find(id);if(it==indexes_.end())return Result::failure("MISSION_OBJECTIVE_NOT_FOUND","Mission does not exist.");
    MissionObjectiveContract& item=missions_[it->second];if(item.status==MISSION_OBJECTIVE_EXPIRED)return Result::success();
    if(item.status!=MISSION_OBJECTIVE_ACCEPTED)return Result::failure("MISSION_OBJECTIVE_STATE","Only an accepted mission can expire.");
    item.status=MISSION_OBJECTIVE_EXPIRED;return Result::success();
}
Result MissionObjectiveBook::cancel(const std::string& id){
    std::map<std::string,std::size_t>::iterator it=indexes_.find(id);if(it==indexes_.end())return Result::failure("MISSION_OBJECTIVE_NOT_FOUND","Mission does not exist.");
    MissionObjectiveContract& item=missions_[it->second];if(item.status==MISSION_OBJECTIVE_CANCELLED)return Result::success();
    if(item.status!=MISSION_OBJECTIVE_ACCEPTED)return Result::failure("MISSION_OBJECTIVE_STATE","Only an accepted mission can be cancelled.");
    item.status=MISSION_OBJECTIVE_CANCELLED;return Result::success();
}
std::vector<std::string> MissionObjectiveBook::expireDue(SimulationSeconds now){
    std::vector<std::string> out;if(now<0)return out;for(std::size_t i=0;i<missions_.size();++i){MissionObjectiveContract& m=missions_[i];
        if(m.status==MISSION_OBJECTIVE_ACCEPTED&&m.objective.deadline>0&&now>m.objective.deadline){m.status=MISSION_OBJECTIVE_EXPIRED;out.push_back(m.objective.missionId);}}
    return out;
}
const MissionObjectiveContract* MissionObjectiveBook::find(const std::string& id)const{std::map<std::string,std::size_t>::const_iterator i=indexes_.find(id);return i==indexes_.end()?0:&missions_[i->second];}
const std::vector<MissionObjectiveContract>& MissionObjectiveBook::missions()const{return missions_;}
}
