#ifndef OEM_CORE_PRIMARY_RESOURCE_H
#define OEM_CORE_PRIMARY_RESOURCE_H
#include "CargoServiceInventory.h"
#include "Market.h"
#include <map>
#include <string>
#include <vector>
namespace oem {
struct StarterStockLine {
    std::string cargoId;
    long long quantity;
    double unitMassKg;
    StarterStockLine();
};
struct StarterStockProfile {
    std::string profileId;
    std::vector<StarterStockLine> lines;
};
Result seedStarterStock(const StarterStockProfile& profile, CargoServiceInventory& inventory);
struct PrimaryResourceSource {
    std::string sourceId;
    std::string locationId;
    std::string cargoId;
    double unitMassKg;
    long long unitsPerCycle;
    SimulationSeconds cycleSeconds;
    long long maximumStock;
    SimulationSeconds lastProductionTime;
    bool enabled;
    PrimaryResourceSource();
};
struct PrimaryResourceIntent {
    std::string operationId;
    std::string sourceId;
    std::string locationId;
    std::string cargoId;
    long long quantity;
    double unitMassKg;
    SimulationSeconds simulationTime;
    PrimaryResourceIntent();
};
class PrimaryResourceBook {
public:
    Result add(const PrimaryResourceSource& source);
    const std::vector<PrimaryResourceSource>& sources() const;
    Result planDue(SimulationSeconds now,
        const std::map<std::string,CargoServiceInventory>& inventories,
        std::vector<PrimaryResourceIntent>& intents) const;
    Result markCommitted(const std::string& sourceId, SimulationSeconds productionTime);
private:
    std::map<std::string,std::size_t> indexes_;
    std::vector<PrimaryResourceSource> sources_;
};
}
#endif