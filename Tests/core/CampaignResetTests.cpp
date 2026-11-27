#include "../../src/core/CampaignReset.h"
#include <cassert>
#include <string>

int main() {
    using oem::CampaignResetPolicy;

    assert(CampaignResetPolicy::isCampaignOwnedFileName("economy.journal"));
    assert(CampaignResetPolicy::isCampaignOwnedFileName("events.journal"));
    assert(CampaignResetPolicy::isCampaignOwnedFileName("economic_stations.registry"));
    assert(CampaignResetPolicy::isCampaignOwnedFileName("economic_agents.cfg"));
    assert(CampaignResetPolicy::isCampaignOwnedFileName("cargo_stock_495353.cfg"));
    assert(CampaignResetPolicy::isCampaignOwnedFileName("cargo_stock_4561727468.cfg"));
    assert(CampaignResetPolicy::isCampaignOwnedFileName("cargo_manifest_ISS.journal"));
    assert(CampaignResetPolicy::isCampaignOwnedFileName("cargo_exchange_ISS.journal"));
    assert(CampaignResetPolicy::isCampaignOwnedFileName("contract_state.journal"));
    assert(CampaignResetPolicy::isCampaignOwnedFileName("contract_x.cfg"));
    assert(CampaignResetPolicy::isCampaignOwnedFileName("cargo_manifest_x.cfg"));
    assert(CampaignResetPolicy::isCampaignOwnedFileName("missions_history.journal"));

    // Configuration and the Orbiter-derived catalogue survive a new game.
    assert(!CampaignResetPolicy::isCampaignOwnedFileName("accounts.cfg"));
    assert(!CampaignResetPolicy::isCampaignOwnedFileName("cargo_catalog.cfg"));
    assert(CampaignResetPolicy::isCampaignOwnedFileName("cargo_services.cfg"));
    assert(!CampaignResetPolicy::isCampaignOwnedFileName("spaceports.registry"));
    assert(!CampaignResetPolicy::isCampaignOwnedFileName("user_notes.txt"));
    assert(!CampaignResetPolicy::isCampaignOwnedFileName("events.journal.bak"));
    assert(!CampaignResetPolicy::isCampaignOwnedFileName("../events.journal"));
    assert(!CampaignResetPolicy::isCampaignOwnedFileName("cargo_stock_.cfg"));
    assert(!CampaignResetPolicy::isCampaignOwnedFileName("cargo_stock_123.txt"));
    assert(!CampaignResetPolicy::isCampaignOwnedFileName("contract_notes.txt"));

    return 0;
}
