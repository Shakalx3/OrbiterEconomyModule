#include "../../src/core/EconomicAgentTrade.h"
#include <cassert>
#include <limits>

int main() {
    oem::CargoCatalog catalog;
    oem::CargoDefinition oxygen; oxygen.cargoId="oxygen"; oxygen.displayName="Oxygen";
    oxygen.unitMassKg=1.0; oxygen.buyPrice=12; oxygen.sellPrice=8;
    assert(catalog.add(oxygen).ok);
    oem::CargoDefinition water; water.cargoId="water"; water.displayName="Water";
    water.unitMassKg=2.0; water.buyPrice=5; water.sellPrice=3;
    assert(catalog.add(water).ok);

    oem::EconomicAgentRegistry agents;
    oem::EconomicAgent supplier;
    supplier.agentId="supplier"; supplier.displayName="Earth Supplier";
    supplier.role=oem::ECONOMIC_AGENT_SUPPLIER; supplier.homeLocationId="Earth";
    supplier.openingBalance=1000; supplier.currentBalance=1000;
    supplier.tradeIntervalSeconds=100; supplier.nextTradeAt=100;
    supplier.operatingLocationIds.push_back("station:ISS");
    supplier.inventory["oxygen"]=150; supplier.sellTargets["oxygen"]=100;
    assert(agents.add(supplier).ok);

    oem::EconomicAgent customer;
    customer.agentId="customer"; customer.displayName="Lunar Customer";
    customer.role=oem::ECONOMIC_AGENT_CUSTOMER; customer.homeLocationId="Moon";
    customer.openingBalance=1000; customer.currentBalance=1000;
    customer.tradeIntervalSeconds=100; customer.nextTradeAt=100;
    customer.operatingLocationIds.push_back("station:ISS");
    customer.buyTargets["water"]=10;
    assert(agents.add(customer).ok);

    std::vector<oem::EconomicLocationStock> locations(1);
    locations[0].locationId="station:ISS";
    locations[0].quantities["water"]=20;
    std::vector<oem::EconomicAgentTrade> trades;
    oem::EconomicAgentTradeEngine engine;
    assert(engine.advance(agents,catalog,locations,100,1,trades).ok);
    oem::EconomicAgent updated;
    assert(agents.find("supplier",updated));
    assert(updated.inventory["oxygen"]==100);
    assert(updated.currentBalance==1400);
    assert(locations[0].quantities["oxygen"]==50);
    assert(agents.find("customer",updated));
    assert(updated.inventory["water"]==10);
    assert(updated.currentBalance==950);
    assert(locations[0].quantities["water"]==10);
    assert(trades.size()==2);
    assert(trades[0].operationId!=trades[1].operationId);

    // Catch-up is bounded; a large time jump cannot run unbounded work in one step.
    assert(engine.advance(agents,catalog,locations,10000,2,trades).ok);
    assert(agents.find("supplier",updated));
    assert(updated.nextTradeAt==400);

    // Invalid trade input is transactional: no partial mutation escapes.
    oem::EconomicAgentRegistry broken=agents;
    oem::EconomicAgent bad;
    bad.agentId="bad"; bad.displayName="Bad"; bad.homeLocationId="Earth";
    bad.tradeIntervalSeconds=1; bad.nextTradeAt=0;
    bad.operatingLocationIds.push_back("station:ISS"); bad.buyTargets["unknown"]=1;
    assert(broken.add(bad).ok);
    const long long oldStock=locations[0].quantities["water"];
    const std::size_t oldTrades=trades.size();
    assert(!engine.advance(broken,catalog,locations,0,1,trades).ok);
    assert(locations[0].quantities["water"]==oldStock);
    assert(trades.size()==oldTrades);
    return 0;
}
