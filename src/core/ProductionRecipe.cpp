#include "ProductionRecipe.h"
#include <cmath>
#include <algorithm>
#include <limits>
namespace oem {
ProductionMaterial::ProductionMaterial():cargoId(),quantity(0),unitMassKg(0.0){}
ProductionRecipe::ProductionRecipe():recipeId(),locationId(),cycleSeconds(86400),lastProductionTime(0),enabled(true),inputs(),outputs(){}
namespace {
bool validMaterials(const std::vector<ProductionMaterial>& lines) {
    if(lines.empty()) return false;
    for(std::size_t i=0;i<lines.size();++i) {
        const ProductionMaterial& m=lines[i];
        if(m.cargoId.empty()||m.quantity<=0||!(m.unitMassKg>0.0)||!std::isfinite(m.unitMassKg)) return false;
        for(std::size_t j=0;j<i;++j) if(lines[j].cargoId==m.cargoId) return false;
    }
    return true;
}
}
Result ProductionRecipeBook::add(const ProductionRecipe& r) {
    if(r.recipeId.empty()||r.locationId.empty()||r.cycleSeconds<=0||r.lastProductionTime<0||
       !validMaterials(r.inputs)||!validMaterials(r.outputs))
        return Result::failure("PRODUCTION_RECIPE_INVALID","Recipe requires IDs, a positive cycle, and unique positive input/output materials with valid masses.");
    if(indexes_.find(r.recipeId)!=indexes_.end())
        return Result::failure("PRODUCTION_RECIPE_DUPLICATE","Recipe ID is already registered.");
    recipes_.push_back(r);indexes_[r.recipeId]=recipes_.size()-1;return Result::success();
}
const std::vector<ProductionRecipe>& ProductionRecipeBook::recipes() const { return recipes_; }
Result ProductionRecipeBook::initializeUnsetTimes(SimulationSeconds now,bool& changed) {
    if(now<0)return Result::failure("PRODUCTION_RECIPE_TIME","Simulation time cannot be negative.");
    changed=false;if(now==0)return Result::success();
    for(std::size_t i=0;i<recipes_.size();++i){
        const SimulationSeconds cycle=recipes_[i].cycleSeconds;
        if(recipes_[i].lastProductionTime==0||recipes_[i].lastProductionTime%cycle!=0){
            recipes_[i].lastProductionTime=(now/cycle)*cycle;changed=true;
        }
    }
    return Result::success();
}
Result ProductionRecipeBook::planDue(SimulationSeconds now,
 const std::map<std::string,CargoServiceInventory>& inventories,
 std::vector<ProductionRecipeIntent>& out) const {
    if(now<0)return Result::failure("PRODUCTION_RECIPE_TIME","Simulation time cannot be negative.");
    std::vector<ProductionRecipeIntent> planned;
    for(std::size_t i=0;i<recipes_.size();++i) {
        const ProductionRecipe& r=recipes_[i];
        if(!r.enabled||r.lastProductionTime>(std::numeric_limits<SimulationSeconds>::max)()-r.cycleSeconds)continue;
        const SimulationSeconds dueTime=r.lastProductionTime+r.cycleSeconds;
        if(now<dueTime)continue;
        std::map<std::string,CargoServiceInventory>::const_iterator loc=inventories.find(r.locationId);
        if(loc==inventories.end())continue;
        bool ready=true;
        for(std::size_t j=0;j<r.inputs.size();++j)
            if(loc->second.quantity(r.inputs[j].cargoId)<r.inputs[j].quantity){ready=false;break;}
        if(!ready)continue;
        ProductionRecipeIntent intent;intent.recipeId=r.recipeId;intent.locationId=r.locationId;
        intent.simulationTime=dueTime;intent.inputs=r.inputs;intent.outputs=r.outputs;
        intent.operationId="recipe-"+r.recipeId+"-"+std::to_string((long long)dueTime);
        planned.push_back(intent);
    }
    out.insert(out.end(),planned.begin(),planned.end());return Result::success();
}
Result ProductionRecipeBook::markCommitted(const std::string& id,SimulationSeconds time) {
    std::map<std::string,std::size_t>::iterator it=indexes_.find(id);
    if(it==indexes_.end())return Result::failure("PRODUCTION_RECIPE_UNKNOWN","Recipe does not exist.");
    ProductionRecipe& r=recipes_[it->second];
    if(time<0||r.lastProductionTime>(std::numeric_limits<SimulationSeconds>::max)()-r.cycleSeconds||
       time!=r.lastProductionTime+r.cycleSeconds)
        return Result::failure("PRODUCTION_RECIPE_NOT_DUE","Production timestamp must equal the next scheduled cycle boundary.");
    r.lastProductionTime=time;return Result::success();
}
Result ProductionRecipeBook::restoreCommitted(const std::string& id,const std::string& location,SimulationSeconds time) {
    std::map<std::string,std::size_t>::iterator it=indexes_.find(id);
    if(it==indexes_.end())return Result::success();
    ProductionRecipe& r=recipes_[it->second];
    if(r.locationId!=location||time<0)return Result::failure("PRODUCTION_RECIPE_HISTORY_CONFLICT","Journaled recipe production conflicts with the configured recipe.");
    if(time>r.lastProductionTime)r.lastProductionTime=time;return Result::success();
}
}
