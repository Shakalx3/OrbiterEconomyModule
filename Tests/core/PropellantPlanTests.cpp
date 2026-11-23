#include "../../src/core/PropellantPlan.h"
#include <cassert>
#include <cmath>
#include <limits>

int main() {
    std::vector<oem::PropellantResourceState> tanks;
    tanks.push_back(oem::PropellantResourceState(20.0, 100.0));
    tanks.push_back(oem::PropellantResourceState(5.0, 25.0));
    tanks.push_back(oem::PropellantResourceState(0.0, 50.0));

    oem::PropellantTotals totals;
    assert(oem::inspectPropellantResources(tanks, totals).ok);
    assert(totals.resourceCount == 3);
    assert(std::fabs(totals.currentMassKg - 25.0) < 1e-9);
    assert(std::fabs(totals.capacityKg - 175.0) < 1e-9);
    assert(std::fabs(totals.freeCapacityKg - 150.0) < 1e-9);

    oem::PropellantFillPlan plan;
    assert(oem::createPropellantFillPlan(tanks, 110.0, plan).ok);
    assert(plan.targetMassKg.size() == 3);
    // Stable index-order fill: first tank, then second, then third.
    assert(std::fabs(plan.targetMassKg[0] - 100.0) < 1e-9);
    assert(std::fabs(plan.targetMassKg[1] - 25.0) < 1e-9);
    assert(std::fabs(plan.targetMassKg[2] - 10.0) < 1e-9);
    assert(std::fabs(plan.plannedMassKg - 110.0) < 1e-9);

    // Zero-capacity resources are valid and skipped.
    std::vector<oem::PropellantResourceState> mixed;
    mixed.push_back(oem::PropellantResourceState(10.0, 10.0));
    mixed.push_back(oem::PropellantResourceState(0.0, 0.0));
    mixed.push_back(oem::PropellantResourceState(2.0, 12.0));
    assert(oem::createPropellantFillPlan(mixed, 5.0, plan).ok);
    assert(plan.targetMassKg[0] == 10.0);
    assert(plan.targetMassKg[1] == 0.0);
    assert(std::fabs(plan.targetMassKg[2] - 7.0) < 1e-9);

    const std::vector<double> oldPlan = plan.targetMassKg;
    assert(!oem::createPropellantFillPlan(tanks, 151.0, plan).ok);
    assert(plan.targetMassKg == oldPlan); // failure never overwrites the prior plan
    assert(!oem::createPropellantFillPlan(tanks, 0.0, plan).ok);
    assert(!oem::createPropellantFillPlan(tanks, -1.0, plan).ok);
    assert(!oem::createPropellantFillPlan(tanks,
        std::numeric_limits<double>::infinity(), plan).ok);

    std::vector<oem::PropellantResourceState> invalid;
    invalid.push_back(oem::PropellantResourceState(11.0, 10.0));
    assert(!oem::inspectPropellantResources(invalid, totals).ok);
    invalid[0] = oem::PropellantResourceState(
        std::numeric_limits<double>::quiet_NaN(), 10.0);
    assert(!oem::inspectPropellantResources(invalid, totals).ok);
    assert(!oem::inspectPropellantResources(
        std::vector<oem::PropellantResourceState>(), totals).ok);
    return 0;
}
