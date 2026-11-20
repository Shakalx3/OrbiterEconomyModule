#include "../../src/core/MarketTradeJournal.h"
#include <cassert>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <map>

static oem::CargoDefinition oxygen(){oem::CargoDefinition d;d.cargoId="oxygen";d.displayName="Oxygen";d.unitMassKg=10.0;d.buyPrice=125;d.sellPrice=80;return d;}
int main(){
 const std::string path="OEMMarketTradeJournalTests.journal";std::remove(path.c_str());
 oem::CargoCatalog cat;assert(cat.add(oxygen()).ok);
 oem::Ledger ledger;assert(ledger.createAccount("company",1000).ok);
 oem::CargoServiceInventory stock("ISS");assert(stock.setInitialStock("oxygen",8,10.0).ok);
 oem::CargoManifest ship("GL-01");oem::MarketQuoteEngine qe;oem::MarketQuote q;
 assert(qe.createQuote(cat,stock,"quote-1","oxygen",oem::MARKET_BUY_FROM_STATION,3,100,q).ok);
 oem::MarketTradeJournalStore store;
 assert(store.apply(path,cat,q,"trade-1","company",ledger,stock,ship).ok);
 oem::Credits balance=0;assert(ledger.getBalance("company",balance)&&balance==625);
 assert(stock.quantity("oxygen")==5&&ship.quantity("oxygen")==3);
 // Replay same operation in the live process is idempotent.
 assert(store.apply(path,cat,q,"trade-1","company",ledger,stock,ship).ok);
 assert(ledger.getBalance("company",balance)&&balance==625&&stock.quantity("oxygen")==5&&ship.quantity("oxygen")==3);
 // Rebuild projections from explicit opening balances and stock.
 oem::Ledger recoveredLedger;assert(recoveredLedger.createAccount("company",1000).ok);
 oem::CargoServiceInventory recoveredStock("ISS");assert(recoveredStock.setInitialStock("oxygen",8,10.0).ok);
 std::map<std::string,oem::CargoManifest> recovered;
 assert(store.load(path,cat,recoveredLedger,recoveredStock,recovered).ok);
 assert(recoveredLedger.getBalance("company",balance)&&balance==625);
 assert(!recoveredLedger.getBalance("ISS",balance));
 assert(recoveredStock.quantity("oxygen")==5&&recovered["GL-01"].quantity("oxygen")==3);
 // Append a second sale and verify full replay.
 oem::MarketQuote sell;assert(qe.createQuote(cat,stock,"quote-2","oxygen",oem::MARKET_SELL_TO_STATION,1,110,sell).ok);
 assert(store.apply(path,cat,sell,"trade-2","company",ledger,stock,ship).ok);
 oem::CargoManifest secondShip("SH-02");oem::MarketQuote buySecond;
 assert(qe.createQuote(cat,stock,"quote-3","oxygen",oem::MARKET_BUY_FROM_STATION,1,120,buySecond).ok);
 assert(store.apply(path,cat,buySecond,"trade-3","company",ledger,stock,secondShip).ok);
 oem::Ledger l2;assert(l2.createAccount("company",1000).ok);
 oem::CargoServiceInventory s2("ISS");assert(s2.setInitialStock("oxygen",8,10.0).ok);std::map<std::string,oem::CargoManifest> recoveredManifests;
 assert(store.load(path,cat,l2,s2,recoveredManifests).ok);
 assert(l2.getBalance("company",balance)&&balance==580);
 assert(s2.quantity("oxygen")==5&&recoveredManifests["GL-01"].quantity("oxygen")==2&&recoveredManifests["SH-02"].quantity("oxygen")==1);
 std::remove(path.c_str());return 0;
}