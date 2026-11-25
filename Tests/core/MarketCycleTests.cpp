#include "../../src/core/EconomyJournal.h"
#include <cassert>
#include <cstdio>
#include <map>
#include <vector>

static oem::CargoDefinition testCargo() {
    oem::CargoDefinition item;
    item.cargoId = "TEST";
    item.displayName = "Test cargo";
    item.unitMassKg = 1.0;
    item.buyPrice = 100;
    item.sellPrice = 60;
    return item;
}

static oem::MarketQuote makeQuote(const oem::CargoCatalog& catalog,
                                  const oem::CargoServiceInventory& stock,
                                  const std::string& id,
                                  oem::MarketSide side,
                                  long long quantity,
                                  long long capacity,
                                  oem::SimulationSeconds time) {
    oem::MarketQuote quote;
    oem::MarketQuoteEngine engine;
    const oem::Result result = engine.createQuote(
        catalog, stock, id, "TEST", side, quantity, time, quote, capacity);
    assert(result.ok);
    return quote;
}

int main() {
    const std::string journalPath = "OEMMarketCycleTests.journal";
    std::remove(journalPath.c_str());

    oem::CargoCatalog catalog;
    assert(catalog.add(testCargo()).ok);

    // Opening stock matches the configured ISS starter inventory.
    std::map<std::string, oem::CargoServiceInventory> inventories;
    inventories.insert(std::make_pair("ISS", oem::CargoServiceInventory("ISS")));
    assert(inventories["ISS"].setInitialStock("TEST", 120, 1.0).ok);

    oem::Ledger ledger;
    assert(ledger.createAccount("company", 10000).ok);
    oem::CargoManifest ship("GL-NT");
    oem::EconomyJournalStore journal;

    // Renewable production adds stock in simulation-time cycles and stops at 150.
    oem::PrimaryResourceBook resources;
    oem::PrimaryResourceSource source;
    source.sourceId = "ISS-TEST-SOURCE";
    source.locationId = "ISS";
    source.cargoId = "TEST";
    source.unitMassKg = 1.0;
    source.unitsPerCycle = 20;
    source.cycleSeconds = 86400;
    source.maximumStock = 150;
    source.lastProductionTime = 0;
    source.enabled = true;
    assert(resources.add(source).ok);

    std::vector<oem::PrimaryResourceIntent> intents;
    assert(resources.planDue(86400, inventories, intents).ok);
    assert(intents.size() == 1 && intents[0].quantity == 20);
    assert(journal.applyPrimaryProduction(journalPath, intents[0].operationId,
        intents[0].sourceId, intents[0].locationId, intents[0].cargoId,
        intents[0].quantity, intents[0].unitMassKg, intents[0].simulationTime,
        inventories).ok);
    assert(resources.markCommitted(source.sourceId, 86400).ok);
    assert(inventories["ISS"].quantity("TEST") == 140);

    intents.clear();
    assert(resources.planDue(172800, inventories, intents).ok);
    assert(intents.size() == 1 && intents[0].quantity == 10);
    assert(journal.applyPrimaryProduction(journalPath, intents[0].operationId,
        intents[0].sourceId, intents[0].locationId, intents[0].cargoId,
        intents[0].quantity, intents[0].unitMassKg, intents[0].simulationTime,
        inventories).ok);
    assert(resources.markCommitted(source.sourceId, 172800).ok);
    assert(inventories["ISS"].quantity("TEST") == 150);

    // Buy ten units, then sell five back. Money, station stock and ship manifest
    // must move together in each durable market transaction.
    oem::MarketQuote buy = makeQuote(catalog, inventories["ISS"], "BUY-10",
        oem::MARKET_BUY_FROM_STATION, 10, 150, 172801);
    assert(journal.applyTrade(journalPath, catalog, buy, "trade-buy-10",
        "company", ledger, inventories, ship).ok);
    oem::Credits balance = 0;
    assert(ledger.getBalance("company", balance) && balance == 9000);
    assert(inventories["ISS"].quantity("TEST") == 140);
    assert(ship.quantity("TEST") == 10);

    oem::MarketQuote sellFive = makeQuote(catalog, inventories["ISS"], "SELL-5",
        oem::MARKET_SELL_TO_STATION, 5, 150, 172802);
    assert(journal.applyTrade(journalPath, catalog, sellFive, "trade-sell-5",
        "company", ledger, inventories, ship).ok);
    assert(ledger.getBalance("company", balance) && balance == 9300);
    assert(inventories["ISS"].quantity("TEST") == 145);
    assert(ship.quantity("TEST") == 5);

    // A sale that would exceed the source cap is rejected before any projection changes.
    oem::MarketQuote tooMuch;
    oem::MarketQuoteEngine quoteEngine;
    assert(!quoteEngine.createQuote(catalog, inventories["ISS"], "SELL-6",
        "TEST", oem::MARKET_SELL_TO_STATION, 6, 172803, tooMuch, 150).ok);
    assert(inventories["ISS"].quantity("TEST") == 145);
    assert(ship.quantity("TEST") == 5);
    assert(ledger.getBalance("company", balance) && balance == 9300);

    oem::MarketQuote sellRemaining = makeQuote(catalog, inventories["ISS"], "SELL-5-REST",
        oem::MARKET_SELL_TO_STATION, 5, 150, 172804);
    assert(journal.applyTrade(journalPath, catalog, sellRemaining, "trade-sell-rest",
        "company", ledger, inventories, ship).ok);
    assert(inventories["ISS"].quantity("TEST") == 150);
    assert(ship.quantity("TEST") == 0);
    assert(ledger.getBalance("company", balance) && balance == 9600);

    // Create a valid quote for the last free unit, then let production fill that
    // unit before execution. Execution must re-check the cap and reject the stale quote.
    oem::MarketQuote buyOne = makeQuote(catalog, inventories["ISS"], "BUY-1",
        oem::MARKET_BUY_FROM_STATION, 1, 150, 172805);
    assert(journal.applyTrade(journalPath, catalog, buyOne, "trade-buy-one",
        "company", ledger, inventories, ship).ok);
    assert(inventories["ISS"].quantity("TEST") == 149);
    assert(ship.quantity("TEST") == 1);

    oem::MarketQuote staleSell = makeQuote(catalog, inventories["ISS"], "STALE-SELL",
        oem::MARKET_SELL_TO_STATION, 1, 150, 172806);
    intents.clear();
    assert(resources.planDue(259200, inventories, intents).ok);
    assert(intents.size() == 1 && intents[0].quantity == 1);
    assert(journal.applyPrimaryProduction(journalPath, intents[0].operationId,
        intents[0].sourceId, intents[0].locationId, intents[0].cargoId,
        intents[0].quantity, intents[0].unitMassKg, intents[0].simulationTime,
        inventories).ok);
    assert(resources.markCommitted(source.sourceId, 259200).ok);
    assert(inventories["ISS"].quantity("TEST") == 150);
    assert(!journal.applyTrade(journalPath, catalog, staleSell, "trade-stale-sell",
        "company", ledger, inventories, ship).ok);
    assert(inventories["ISS"].quantity("TEST") == 150);
    assert(ship.quantity("TEST") == 1);
    assert(ledger.getBalance("company", balance) && balance == 9500);

    // Recover the complete economy from the same opening baseline and journal.
    std::map<std::string, oem::CargoServiceInventory> recoveredInventories;
    recoveredInventories.insert(std::make_pair("ISS", oem::CargoServiceInventory("ISS")));
    assert(recoveredInventories["ISS"].setInitialStock("TEST", 120, 1.0).ok);
    std::map<std::string, oem::CargoManifest> recoveredManifests;
    oem::Ledger recoveredLedger;
    assert(recoveredLedger.createAccount("company", 10000).ok);
    assert(journal.load(journalPath, catalog, recoveredLedger,
        recoveredInventories, recoveredManifests).ok);
    assert(recoveredLedger.getBalance("company", balance) && balance == 9500);
    assert(recoveredInventories["ISS"].quantity("TEST") == 150);
    assert(recoveredManifests.find("GL-NT") != recoveredManifests.end());
    assert(recoveredManifests["GL-NT"].quantity("TEST") == 1);

    // Replaying the already committed buy operation is harmless.
    assert(journal.applyTrade(journalPath, catalog, buyOne, "trade-buy-one",
        "company", recoveredLedger, recoveredInventories,
        recoveredManifests["GL-NT"]).ok);
    assert(recoveredLedger.getBalance("company", balance) && balance == 9500);
    assert(recoveredInventories["ISS"].quantity("TEST") == 150);
    assert(recoveredManifests["GL-NT"].quantity("TEST") == 1);

    std::remove(journalPath.c_str());
    return 0;
}
