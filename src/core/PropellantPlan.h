#ifndef OEM_CORE_PROPELLANT_PLAN_H
#define OEM_CORE_PROPELLANT_PLAN_H

#include "Types.h"
#include <vector>

namespace oem {

// Snapshot of one Orbiter propellant resource. Values are always kilograms.
struct PropellantResourceState {
    double currentMassKg;
    double capacityKg;
    PropellantResourceState();
    PropellantResourceState(double currentMass, double capacity);
};

struct PropellantTotals {
    double currentMassKg;
    double capacityKg;
    double freeCapacityKg;
    std::size_t resourceCount;
    PropellantTotals();
};

// A deterministic target mass for each resource, in the same order as the snapshot.
struct PropellantFillPlan {
    std::vector<double> targetMassKg;
    double requestedMassKg;
    double plannedMassKg;
    PropellantFillPlan();
};

// Validates every resource and sums all masses/capacities. Empty resource sets fail.
Result inspectPropellantResources(const std::vector<PropellantResourceState>& resources,
                                  PropellantTotals& totals);

// Fills resources in stable index order. It never overfills or silently truncates
// a request; failure leaves the output plan unchanged.
Result createPropellantFillPlan(const std::vector<PropellantResourceState>& resources,
                                double requestedMassKg,
                                PropellantFillPlan& plan);

} // namespace oem

#endif
