#include "../../src/core/EconomyJournal.h"
#include <cassert>
#include <cstdio>
#include <fstream>
#include <map>
#include <string>
static oem::CargoDefinition oxygen(){oem::CargoDefinition d;d.cargoId="oxygen";d.displayName="Oxygen";d.unitMassKg=10.0;d.buyPrice=100;d.sellPrice=60;return d;}
static oem::CargoTransfer transfer(const std::string&id,const std::string&ship,const std::string&station,oem::CargoTransferDirection dir,long long qty,oem::SimulationSeconds t){oem::CargoTransfer x;x.transferId=id;x.vesselId=ship;x.locationId=station;x.cargoId="oxygen";x.direction=dir;x.quantity=qty;x.unitMassKg=10.0;x.simulationTime=t;return x;}
int main(){
 const std::string path="OEMEconomyJournalTests.journal";std::remove(path.c_str());
 oem::CargoCatalog cat;assert(cat.add(oxygen()).ok);
 oem::Ledger ledger;assert(ledger.createAccount("company",1000).ok);
 std::map<std::string,oem::CargoServiceInventory> stocks;
 stocks.insert(std::make_pair("ISS",oem::CargoServiceInventory("ISS")));assert(stocks["ISS"].setInitialStock("oxygen",5,10.0).ok);
 stocks.insert(std::make_pair("MARS",oem::CargoServiceInventory("MARS")));assert(stocks["MARS"].setInitialStock("oxygen",4,10.0).ok);
 oem::CargoManifest a("GL-01"),b("GL-02");oem::EconomyJournalStore store;
 const oem::CargoTransfer firstExchange=transfer("x-1","GL-01","ISS",oem::CARGO_LOAD,2,10);
 assert(store.applyExchange(path,stocks,a,firstExchange).ok);
 assert(store.applyExchange(path,stocks,a,firstExchange).ok);
 oem::MarketQuoteEngine qe;oem::MarketQuote q;
 assert(qe.createQuote(cat,stocks["ISS"],"q-1","oxygen",oem::MARKET_SELL_TO_STATION,1,11,q).ok);
 assert(store.applyTrade(path,cat,q,"t-1","company",ledger,stocks,a).ok);
 assert(store.applyTrade(path,cat,q,"t-1","company",ledger,stocks,a).ok);
 assert(qe.createQuote(cat,stocks["MARS"],"q-2","oxygen",oem::MARKET_BUY_FROM_STATION,2,12,q).ok);
 assert(store.applyTrade(path,cat,q,"t-2","company",ledger,stocks,b).ok);
 assert(store.applyExchange(path,stocks,b,transfer("x-2","GL-02","MARS",oem::CARGO_UNLOAD,1,13)).ok);
 oem::Credits balance=0;assert(ledger.getBalance("company",balance)&&balance==860);
 assert(stocks["ISS"].quantity("oxygen")==4&&stocks["MARS"].quantity("oxygen")==3);
 assert(a.quantity("oxygen")==1&&b.quantity("oxygen")==1);
 // A cross-type reuse of an existing operation ID must conflict.
 oem::MarketQuote conflict;assert(qe.createQuote(cat,stocks["ISS"],"q-3","oxygen",oem::MARKET_BUY_FROM_STATION,1,14,conflict).ok);
 assert(!store.applyTrade(path,cat,conflict,"x-1","company",ledger,stocks,a).ok);
 // A torn final fragment is discarded; completed transactions remain recoverable.
 {std::ofstream tail(path.c_str(),std::ios::binary|std::ios::app);tail<<"T|incomplete";}
 oem::Ledger recoveredLedger;assert(recoveredLedger.createAccount("company",1000).ok);
 std::map<std::string,oem::CargoServiceInventory> recoveredStocks;
 recoveredStocks.insert(std::make_pair("ISS",oem::CargoServiceInventory("ISS")));assert(recoveredStocks["ISS"].setInitialStock("oxygen",5,10.0).ok);
 recoveredStocks.insert(std::make_pair("MARS",oem::CargoServiceInventory("MARS")));assert(recoveredStocks["MARS"].setInitialStock("oxygen",4,10.0).ok);
 std::map<std::string,oem::CargoManifest> manifests;
 assert(store.load(path,cat,recoveredLedger,recoveredStocks,manifests).ok);
 assert(recoveredLedger.getBalance("company",balance)&&balance==860);
 assert(recoveredStocks["ISS"].quantity("oxygen")==4&&recoveredStocks["MARS"].quantity("oxygen")==3);
 assert(manifests["GL-01"].quantity("oxygen")==1&&manifests["GL-02"].quantity("oxygen")==1);
 // A malformed complete record rejects recovery transactionally.
 {std::ofstream bad(path.c_str(),std::ios::binary|std::ios::app);bad<<"BROKEN\n";}
 oem::Ledger untouched;assert(untouched.createAccount("company",1000).ok);
 std::map<std::string,oem::CargoServiceInventory> untouchedStocks;
 untouchedStocks.insert(std::make_pair("ISS",oem::CargoServiceInventory("ISS")));assert(untouchedStocks["ISS"].setInitialStock("oxygen",5,10.0).ok);
 untouchedStocks.insert(std::make_pair("MARS",oem::CargoServiceInventory("MARS")));assert(untouchedStocks["MARS"].setInitialStock("oxygen",4,10.0).ok);
 std::map<std::string,oem::CargoManifest> untouchedManifests;
 assert(!store.load(path,cat,untouched,untouchedStocks,untouchedManifests).ok);
 assert(untouched.getBalance("company",balance)&&balance==1000);
 assert(untouchedStocks["ISS"].quantity("oxygen")==5&&untouchedStocks["MARS"].quantity("oxygen")==4);
 // Contract acceptance, completion and payout are one replayable lifecycle.
 const std::string contractPath="OEMContractEconomyJournalTests.journal";std::remove(contractPath.c_str());
 oem::Ledger contractLedger;assert(contractLedger.createAccount("company",1000).ok);
 oem::DeliveryContractBook contractBook;oem::DeliveryContract contract;
 contract.contractId="delivery-001";contract.vesselId="GL-01";contract.cargoId="oxygen";
 contract.quantity=2;contract.originLocationId="ISS";contract.destinationLocationId="MARS";
 contract.reward=250;contract.acceptedAt=20;contract.deadline=100;
 assert(store.acceptContract(contractPath,contractBook,contract).ok);
 assert(store.completeContract(contractPath,contractBook,contract.contractId,"landing-event-50",50).ok);
 assert(store.settleContract(contractPath,contractBook,contract.contractId,"company",contractLedger).ok);
 assert(contractBook.find(contract.contractId)->status==oem::CONTRACT_SETTLED);
 assert(contractLedger.getBalance("company",balance)&&balance==1250);
 oem::Ledger contractRecoveredLedger;assert(contractRecoveredLedger.createAccount("company",1000).ok);
 oem::DeliveryContractBook recoveredContracts;
 std::map<std::string,oem::CargoServiceInventory> noStocks;
 std::map<std::string,oem::CargoManifest> noManifests;
 assert(store.load(contractPath,cat,contractRecoveredLedger,noStocks,noManifests,&recoveredContracts).ok);
 assert(recoveredContracts.find(contract.contractId)!=0);
 assert(recoveredContracts.find(contract.contractId)->status==oem::CONTRACT_SETTLED);
 assert(contractRecoveredLedger.getBalance("company",balance)&&balance==1250);
 // Replaying the same committed settlement again must not pay twice.
 assert(store.load(contractPath,cat,contractRecoveredLedger,noStocks,noManifests,&recoveredContracts).ok);
 assert(contractRecoveredLedger.getBalance("company",balance)&&balance==1250);
 std::remove(contractPath.c_str());
 std::remove(path.c_str());return 0;
}
