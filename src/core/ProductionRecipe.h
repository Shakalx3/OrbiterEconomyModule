#ifndef OEM_CORE_PRODUCTION_RECIPE_H
#define OEM_CORE_PRODUCTION_RECIPE_H
#include "CargoServiceInventory.h"
#include "Types.h"
#include <map>
#include <string>
#include <vector>
namespace oem {
struct ProductionMaterial {
    std::string cargoId;
    long long quantity;
    double unitMassKg;
    ProductionMaterial();
};
struct ProductionRecipe {
    std::string recipeId;
    std::string locationId;
    SimulationSeconds cycleSeconds;
    SimulationSeconds lastProductionTime;
    bool enabled;
    std::vector<ProductionMaterial> inputs;
    std::vector<ProductionMaterial> outputs;
    ProductionRecipe();
};
struct ProductionRecipeIntent {
    std::string operationId;
    std::string recipeId;
    std::string locationId;
    SimulationSeconds simulationTime;
    std::vector<ProductionMaterial> inputs;
    std::vector<ProductionMaterial> outputs;
};
class ProductionRecipeBook {
public:
    Result add(const ProductionRecipe& recipe);
    const std::vector<ProductionRecipe>& recipes() const;
    Result initializeUnsetTimes(SimulationSeconds now, bool& changed);
    Result planDue(SimulationSeconds now,
        const std::map<std::string,CargoServiceInventory>& inventories,
        std::vector<ProductionRecipeIntent>& intents) const;
    Result markCommitted(const std::string& recipeId, SimulationSeconds time);
    Result restoreCommitted(const std::string& recipeId, const std::string& locationId,
        SimulationSeconds time);
private:
    std::map<std::string,std::size_t> indexes_;
    std::vector<ProductionRecipe> recipes_;
};
}
#endif
