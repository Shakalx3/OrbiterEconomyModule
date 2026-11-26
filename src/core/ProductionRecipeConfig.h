#ifndef OEM_CORE_PRODUCTION_RECIPE_CONFIG_H
#define OEM_CORE_PRODUCTION_RECIPE_CONFIG_H
#include "ProductionRecipe.h"
#include <string>
namespace oem {
class ProductionRecipeConfigStore {
public:
    Result load(const std::string& path, ProductionRecipeBook& book) const;
    Result save(const std::string& path, const ProductionRecipeBook& book) const;
};
}
#endif
