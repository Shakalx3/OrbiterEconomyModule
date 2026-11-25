#include "../../src/core/EconomicAgent.h"
#include <cassert>
#include <cstdio>
#include <fstream>
#include <string>

int main() {
    const std::string path="OEMEconomicAgentTests.cfg";
    std::remove(path.c_str());
    oem::EconomicAgentRegistry agents;
    oem::EconomicAgent supplier;
    supplier.agentId="supplier-earth";
    supplier.displayName="Earth Supply Cooperative";
    supplier.role=oem::ECONOMIC_AGENT_SUPPLIER;
    supplier.homeLocationId="surface:Earth/Canaveral";
    supplier.openingBalance=50000;
    supplier.currentBalance=50000;
    supplier.tradeIntervalSeconds=3600;
    supplier.nextTradeAt=7200;
    supplier.operatingLocationIds.push_back("station:ISS");
    supplier.inventory["oxygen"]=100;
    supplier.buyTargets["water"]=25;
    supplier.sellTargets["oxygen"]=200;
    assert(agents.add(supplier).ok);
    assert(!agents.add(supplier).ok);
    oem::EconomicAgent found;
    assert(agents.find("supplier-earth",found));
    assert(found.inventory["oxygen"]==100);
    assert(!agents.find("missing",found));

    oem::EconomicAgent invalid=supplier;invalid.agentId="invalid-inventory";
    invalid.inventory["water"]=-1;
    assert(!agents.add(invalid).ok);
    invalid=supplier;invalid.agentId="invalid-schedule";invalid.tradeIntervalSeconds=0;
    assert(!agents.add(invalid).ok);

    oem::EconomicAgentConfigStore store;
    assert(store.save(path,agents).ok);
    oem::EconomicAgentRegistry loaded;
    assert(store.load(path,loaded).ok);
    assert(loaded.find("supplier-earth",found));
    assert(found.displayName==supplier.displayName);
    assert(found.currentBalance==50000);
    assert(found.nextTradeAt==7200);
    assert(found.operatingLocationIds.size()==1 && found.operatingLocationIds[0]=="station:ISS");
    assert(found.inventory["oxygen"]==100);
    assert(found.buyTargets["water"]==25);
    assert(found.sellTargets["oxygen"]==200);
    {
        std::ofstream bad(path.c_str(),std::ios::binary|std::ios::trunc);
        bad<<"OEM_ECONOMIC_AGENTS|99\n";
    }
    assert(!store.load(path,loaded).ok);
    std::remove(path.c_str());
    return 0;
}
