#include "PropellantPlan.h"
#include <cmath>

namespace oem {

PropellantResourceState::PropellantResourceState()
    : currentMassKg(0.0), capacityKg(0.0) {}
PropellantResourceState::PropellantResourceState(double currentMass, double capacity)
    : currentMassKg(currentMass), capacityKg(capacity) {}
PropellantTotals::PropellantTotals()
    : currentMassKg(0.0), capacityKg(0.0), freeCapacityKg(0.0), resourceCount(0) {}
PropellantFillPlan::PropellantFillPlan()
    : targetMassKg(), requestedMassKg(0.0), plannedMassKg(0.0) {}

Result inspectPropellantResources(const std::vector<PropellantResourceState>& resources,
                                  PropellantTotals& totals) {
    if (resources.empty())
        return Result::failure("PROPELLANT_NO_RESOURCES",
            "The vessel has no propellant resources.");

    PropellantTotals candidate;
    candidate.resourceCount = resources.size();
    for (std::size_t i = 0; i < resources.size(); ++i) {
        const double current = resources[i].currentMassKg;
        const double capacity = resources[i].capacityKg;
        if (!std::isfinite(current) || !std::isfinite(capacity) ||
            current < 0.0 || capacity < 0.0 || current > capacity) {
            return Result::failure("PROPELLANT_INVALID_RESOURCE",
                "A propellant resource has invalid mass or capacity.");
        }
        candidate.currentMassKg += current;
        candidate.capacityKg += capacity;
        candidate.freeCapacityKg += capacity - current;
        if (!std::isfinite(candidate.currentMassKg) ||
            !std::isfinite(candidate.capacityKg) ||
            !std::isfinite(candidate.freeCapacityKg)) {
            return Result::failure("PROPELLANT_TOTAL_OVERFLOW",
                "The vessel propellant totals are not finite.");
        }
    }
    totals = candidate;
    return Result::success();
}

Result createPropellantFillPlan(const std::vector<PropellantResourceState>& resources,
                                double requestedMassKg,
                                PropellantFillPlan& plan) {
    if (!std::isfinite(requestedMassKg) || requestedMassKg <= 0.0)
        return Result::failure("PROPELLANT_INVALID_REQUEST",
            "Requested refuelling mass must be finite and greater than zero.");

    PropellantTotals totals;
    const Result inspected = inspectPropellantResources(resources, totals);
    if (!inspected.ok) return inspected;
    if (requestedMassKg > totals.freeCapacityKg)
        return Result::failure("PROPELLANT_OVER_CAPACITY",
            "Requested refuelling mass exceeds the vessel's free tank capacity.");

    PropellantFillPlan candidate;
    candidate.requestedMassKg = requestedMassKg;
    candidate.targetMassKg.reserve(resources.size());
    double remaining = requestedMassKg;
    for (std::size_t i = 0; i < resources.size(); ++i) {
        const double free = resources[i].capacityKg - resources[i].currentMassKg;
        const double addition = remaining < free ? remaining : free;
        candidate.targetMassKg.push_back(resources[i].currentMassKg + addition);
        remaining -= addition;
    }
    if (remaining > 1e-9)
        return Result::failure("PROPELLANT_PLAN_INCOMPLETE",
            "The refuelling plan could not allocate the full requested mass.");
    candidate.plannedMassKg = requestedMassKg - remaining;
    plan = candidate;
    return Result::success();
}

} // namespace oem
