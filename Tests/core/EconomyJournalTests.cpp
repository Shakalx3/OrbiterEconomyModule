#include "../../src/core/EconomyJournal.h"
#include <cassert>
#include <cstdio>
#include <fstream>
#include <iostream>
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
 // Contract delivery removes the cargo, credits destination stock and pays once.
 const std::string contractPath="OEMContractEconomyJournalTests.journal";std::remove(contractPath.c_str());
 oem::Ledger contractLedger;assert(contractLedger.createAccount("company",1000).ok);
 std::map<std::string,oem::CargoServiceInventory> contractStocks;
 contractStocks.insert(std::make_pair("ISS",oem::CargoServiceInventory("ISS")));
 assert(contractStocks["ISS"].setInitialStock("oxygen",5,10.0).ok);
 contractStocks.insert(std::make_pair("MARS",oem::CargoServiceInventory("MARS")));
 assert(contractStocks["MARS"].setInitialStock("oxygen",4,10.0).ok);
 oem::CargoManifest contractManifest("GL-01");
 oem::DeliveryContractBook contractBook;oem::DeliveryContract contract;
 contract.contractId="delivery-001";contract.vesselId="GL-01";contract.cargoId="oxygen";
 contract.quantity=2;contract.originLocationId="ISS";contract.destinationLocationId="MARS";
 contract.destinationBodyId="Mars";
 contract.missionBriefing="A shortage has been reported. Deliver oxygen to MARS before the deadline.";
 contract.reward=250;contract.acceptedAt=20;contract.deadline=100;
 assert(store.acceptContract(contractPath,contractBook,contract).ok);
 assert(store.applyExchange(contractPath,contractStocks,contractManifest,
     transfer("contract-load","GL-01","ISS",oem::CARGO_LOAD,2,21)).ok);
 assert(store.completeContract(contractPath,contractBook,contract.contractId,"landing-event-50",50,
     contractStocks,contractManifest).ok);
 // Cargo is already gone before the player claims the reward.
 assert(contractManifest.quantity("oxygen")==0);
 assert(contractStocks["MARS"].quantity("oxygen")==6);
 assert(store.settleContract(contractPath,contractBook,contract.contractId,"company",
     contractLedger).ok);
 assert(contractBook.find(contract.contractId)->status==oem::CONTRACT_SETTLED);
 assert(contractLedger.getBalance("company",balance)&&balance==1250);
 assert(contractManifest.quantity("oxygen")==0);
 assert(contractStocks["MARS"].quantity("oxygen")==6);
 // The same journal replays both the cargo transfer and payment exactly once.
 oem::Ledger contractRecoveredLedger;assert(contractRecoveredLedger.createAccount("company",1000).ok);
 oem::DeliveryContractBook recoveredContracts;
 std::map<std::string,oem::CargoServiceInventory> recoveredContractStocks;
 recoveredContractStocks.insert(std::make_pair("ISS",oem::CargoServiceInventory("ISS")));
 assert(recoveredContractStocks["ISS"].setInitialStock("oxygen",5,10.0).ok);
 recoveredContractStocks.insert(std::make_pair("MARS",oem::CargoServiceInventory("MARS")));
 assert(recoveredContractStocks["MARS"].setInitialStock("oxygen",4,10.0).ok);
 std::map<std::string,oem::CargoManifest> recoveredContractManifests;
 oem::Result contractReplay=store.load(contractPath,cat,contractRecoveredLedger,
     recoveredContractStocks,recoveredContractManifests,&recoveredContracts);
 if(!contractReplay.ok)std::cerr<<"Contract replay failed: "<<contractReplay.code<<" "<<contractReplay.message<<"\\n";
 assert(contractReplay.ok);
 assert(recoveredContracts.find(contract.contractId)!=0);
 assert(recoveredContracts.find(contract.contractId)->status==oem::CONTRACT_SETTLED);
 assert(recoveredContracts.find(contract.contractId)->destinationBodyId=="Mars");
  assert(recoveredContracts.find(contract.contractId)->missionType=="PRIORITY");
  assert(recoveredContracts.find(contract.contractId)->missionTitle=="Priority cargo delivery");
  assert(recoveredContracts.find(contract.contractId)->missionBriefing=="A reported shortage requires this oxygen shipment.");
 assert(recoveredContracts.find(contract.contractId)->missionType=="PRIORITY");
 assert(recoveredContracts.find(contract.contractId)->missionTitle=="Priority cargo delivery");
 assert(recoveredContracts.find(contract.contractId)->missionBriefing==contract.missionBriefing);
 assert(contractRecoveredLedger.getBalance("company",balance)&&balance==1250);
 assert(recoveredContractManifests["GL-01"].quantity("oxygen")==0);
 assert(recoveredContractStocks["MARS"].quantity("oxygen")==6);
 assert(store.load(contractPath,cat,contractRecoveredLedger,recoveredContractStocks,
     recoveredContractManifests,&recoveredContracts).ok);
 assert(contractRecoveredLedger.getBalance("company",balance)&&balance==1250);
 assert(recoveredContractManifests["GL-01"].quantity("oxygen")==0);
 assert(recoveredContractStocks["MARS"].quantity("oxygen")==6);
 // Cancellation is durable, idempotent, and does not affect funds or cargo.
 const std::string cancelPath="OEMContractCancelJournalTests.journal";std::remove(cancelPath.c_str());
 oem::DeliveryContractBook cancelBook;oem::DeliveryContract cancellable=contract;
 cancellable.contractId="cancel-001";cancellable.vesselId="GL-01";cancellable.originLocationId="ISS";
 cancellable.destinationLocationId="MARS";cancellable.reward=900;
 assert(store.acceptContract(cancelPath,cancelBook,cancellable).ok);
 assert(store.cancelContract(cancelPath,cancelBook,cancellable.contractId).ok);
 assert(cancelBook.find(cancellable.contractId)->status==oem::CONTRACT_CANCELLED);
 assert(store.cancelContract(cancelPath,cancelBook,cancellable.contractId).ok);
 oem::Ledger cancelLedger;assert(cancelLedger.createAccount("company",1000).ok);
 oem::DeliveryContractBook cancelRecovered;
 std::map<std::string,oem::CargoServiceInventory> cancelStocks;
 cancelStocks.insert(std::make_pair("ISS",oem::CargoServiceInventory("ISS")));assert(cancelStocks["ISS"].setInitialStock("oxygen",5,10.0).ok);
 cancelStocks.insert(std::make_pair("MARS",oem::CargoServiceInventory("MARS")));assert(cancelStocks["MARS"].setInitialStock("oxygen",4,10.0).ok);
 std::map<std::string,oem::CargoManifest> cancelManifests;
 assert(store.load(cancelPath,cat,cancelLedger,cancelStocks,cancelManifests,&cancelRecovered).ok);
 assert(cancelRecovered.find(cancellable.contractId)->status==oem::CONTRACT_CANCELLED);
 assert(cancelLedger.getBalance("company",balance)&&balance==1000);
 assert(cancelStocks["ISS"].quantity("oxygen")==5&&cancelStocks["MARS"].quantity("oxygen")==4);
 // Propellant charges share the global journal and replay exactly once.
 const std::string propellantPath="OEMPropellantEconomyJournalTests.journal";std::remove(propellantPath.c_str());
 oem::Ledger fuelLedger;assert(fuelLedger.createAccount("company",1000).ok);
 assert(store.chargePropellant(propellantPath,"fill-001","company",100,77,fuelLedger).ok);
 assert(fuelLedger.getBalance("company",balance)&&balance==900);
 assert(store.chargePropellant(propellantPath,"fill-001","company",100,77,fuelLedger).ok);
 assert(fuelLedger.getBalance("company",balance)&&balance==900);
 oem::Ledger fuelRecovered;assert(fuelRecovered.createAccount("company",1000).ok);
 std::map<std::string,oem::CargoServiceInventory> noStocks;
 std::map<std::string,oem::CargoManifest> noManifests;
 assert(store.load(propellantPath,cat,fuelRecovered,noStocks,noManifests).ok);
 assert(fuelRecovered.getBalance("company",balance)&&balance==900);
 assert(store.load(propellantPath,cat,fuelRecovered,noStocks,noManifests).ok);
 assert(fuelRecovered.getBalance("company",balance)&&balance==900);
 const std::string insufficientPath="OEMPropellantInsufficientTests.journal";std::remove(insufficientPath.c_str());
 oem::Ledger poorLedger;assert(poorLedger.createAccount("company",50).ok);
 assert(!store.chargePropellant(insufficientPath,"fill-poor","company",100,78,poorLedger).ok);
 assert(poorLedger.getBalance("company",balance)&&balance==50);
 {std::ifstream absent(insufficientPath.c_str(),std::ios::binary);assert(!absent.good());}
 std::remove(insufficientPath.c_str());
 std::remove(propellantPath.c_str());
 std::remove(cancelPath.c_str());
 std::remove(contractPath.c_str());
 std::remove(path.c_str());return 0;
}
