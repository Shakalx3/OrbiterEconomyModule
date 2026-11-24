#include "../../src/core/EconomyJournal.h"
#include <cassert>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
static oem::CargoDefinition oxygen(){oem::CargoDefinition d;d.cargoId="oxygen";d.displayName="Oxygen";d.unitMassKg=10.0;d.buyPrice=100;d.sellPrice=60;return d;}
static oem::CargoTransfer transfer(const std::string&id,const std::string&ship,const std::string&station,oem::CargoTransferDirection dir,long long qty,oem::SimulationSeconds t){oem::CargoTransfer x;x.transferId=id;x.vesselId=ship;x.locationId=station;x.cargoId="oxygen";x.direction=dir;x.quantity=qty;x.unitMassKg=10.0;x.simulationTime=t;return x;}
static std::string hex(const std::string& value) {
 static const char digits[] = "0123456789ABCDEF";
 std::string encoded;
 for (std::size_t i = 0; i < value.size(); ++i) {
  const unsigned char c = (unsigned char)value[i];
  encoded += digits[c >> 4];
  encoded += digits[c & 15];
 }
 return encoded;
}
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
 contract.missionType="PRIORITY";contract.missionTitle="Priority cargo delivery";
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
 assert(recoveredContracts.find(contract.contractId)->missionBriefing==contract.missionBriefing);
 assert(contractRecoveredLedger.getBalance("company",balance)&&balance==1250);
 assert(recoveredContractManifests["GL-01"].quantity("oxygen")==0);
 assert(recoveredContractStocks["MARS"].quantity("oxygen")==6);
 assert(store.load(contractPath,cat,contractRecoveredLedger,recoveredContractStocks,
     recoveredContractManifests,&recoveredContracts).ok);
 assert(contractRecoveredLedger.getBalance("company",balance)&&balance==1250);
 assert(recoveredContractManifests["GL-01"].quantity("oxygen")==0);
 assert(recoveredContractStocks["MARS"].quantity("oxygen")==6);
 // Delivery acceptance atomically loads the contracted cargo from origin stock.
 const std::string autoLoadPath="OEMContractAutoLoadTests.journal";std::remove(autoLoadPath.c_str());
 oem::DeliveryContractBook autoLoadBook;
 std::map<std::string,oem::CargoServiceInventory> autoLoadStocks;
 autoLoadStocks.insert(std::make_pair("ISS",oem::CargoServiceInventory("ISS")));
 assert(autoLoadStocks["ISS"].setInitialStock("oxygen",5,10.0).ok);
 oem::CargoManifest autoLoadManifest("GL-AUTO");
 oem::DeliveryContract autoLoadContract=contract;
 autoLoadContract.contractId="auto-load-001";autoLoadContract.vesselId="GL-AUTO";
 autoLoadContract.originLocationId="ISS";autoLoadContract.destinationLocationId="MARS";
 autoLoadContract.quantity=2;autoLoadContract.acceptedAt=30;autoLoadContract.deadline=200;
 assert(store.acceptContractWithCargo(autoLoadPath,autoLoadBook,autoLoadStocks,
     autoLoadManifest,autoLoadContract,10.0).ok);
 assert(autoLoadStocks["ISS"].quantity("oxygen")==3);
 assert(autoLoadManifest.quantity("oxygen")==2);
 assert(autoLoadManifest.history().size()==1);
 assert(store.acceptContractWithCargo(autoLoadPath,autoLoadBook,autoLoadStocks,
     autoLoadManifest,autoLoadContract,10.0).ok);
 assert(autoLoadStocks["ISS"].quantity("oxygen")==3);
 assert(autoLoadManifest.quantity("oxygen")==2);
 oem::Ledger autoLoadLedger;assert(autoLoadLedger.createAccount("company",1000).ok);
 oem::DeliveryContractBook autoLoadRecoveredBook;
 std::map<std::string,oem::CargoServiceInventory> autoLoadRecoveredStocks;
 autoLoadRecoveredStocks.insert(std::make_pair("ISS",oem::CargoServiceInventory("ISS")));
 assert(autoLoadRecoveredStocks["ISS"].setInitialStock("oxygen",5,10.0).ok);
 std::map<std::string,oem::CargoManifest> autoLoadRecoveredManifests;
 assert(store.load(autoLoadPath,cat,autoLoadLedger,autoLoadRecoveredStocks,
     autoLoadRecoveredManifests,&autoLoadRecoveredBook).ok);
 assert(autoLoadRecoveredBook.find("auto-load-001")!=0);
 assert(autoLoadRecoveredStocks["ISS"].quantity("oxygen")==3);
 assert(autoLoadRecoveredManifests["GL-AUTO"].quantity("oxygen")==2);
 assert(store.load(autoLoadPath,cat,autoLoadLedger,autoLoadRecoveredStocks,
     autoLoadRecoveredManifests,&autoLoadRecoveredBook).ok);
 assert(autoLoadRecoveredStocks["ISS"].quantity("oxygen")==3);
 assert(autoLoadRecoveredManifests["GL-AUTO"].quantity("oxygen")==2);
 const std::string noStockPath="OEMContractAutoLoadNoStockTests.journal";std::remove(noStockPath.c_str());
 oem::DeliveryContractBook noStockBook;
 std::map<std::string,oem::CargoServiceInventory> noStock;
 noStock.insert(std::make_pair("ISS",oem::CargoServiceInventory("ISS")));
 assert(noStock["ISS"].setInitialStock("oxygen",1,10.0).ok);
 oem::CargoManifest noStockManifest("GL-POOR");
 oem::DeliveryContract tooLarge=autoLoadContract;tooLarge.contractId="auto-load-too-large";
 tooLarge.vesselId="GL-POOR";tooLarge.quantity=2;
 assert(!store.acceptContractWithCargo(noStockPath,noStockBook,noStock,
     noStockManifest,tooLarge,10.0).ok);
 assert(noStockBook.find("auto-load-too-large")==0);
 assert(noStock["ISS"].quantity("oxygen")==1&&noStockManifest.quantity("oxygen")==0);
 std::remove(noStockPath.c_str());std::remove(autoLoadPath.c_str());
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
 int cancelledReputation=0;assert(!cancelLedger.getReputation("company",cancelledReputation));
 assert(cancelStocks["ISS"].quantity("oxygen")==5&&cancelStocks["MARS"].quantity("oxygen")==4);
 // Expired delivery contracts debit the configured fee and reduce reputation once.
 const std::string failurePath="OEMContractFailureJournalTests.journal";std::remove(failurePath.c_str());
 oem::DeliveryContractBook failureBook;
 oem::DeliveryContract failedContract=contract;
 failedContract.contractId="failure-delivery-001";failedContract.acceptedAt=10;failedContract.deadline=20;failedContract.reward=250;
 assert(store.acceptContract(failurePath,failureBook,failedContract).ok);
 oem::Ledger failureLedger;assert(failureLedger.createAccount("company",500).ok);
 assert(store.expireContract(failurePath,failureBook,failedContract.contractId,failureLedger,"company",21).ok);
 assert(failureBook.find(failedContract.contractId)->status==oem::CONTRACT_EXPIRED);
 assert(failureLedger.getBalance("company",balance)&&balance==475);
 int failureReputation=0;assert(failureLedger.getReputation("company",failureReputation)&&failureReputation==45);
 assert(store.expireContract(failurePath,failureBook,failedContract.contractId,failureLedger,"company",21).ok);
 assert(failureLedger.getBalance("company",balance)&&balance==475);
 oem::Ledger failureRecoveredLedger;assert(failureRecoveredLedger.createAccount("company",500).ok);
 oem::DeliveryContractBook failureRecoveredBook;
 std::map<std::string,oem::CargoServiceInventory> failureStocks;
 std::map<std::string,oem::CargoManifest> failureManifests;
 assert(store.load(failurePath,cat,failureRecoveredLedger,failureStocks,failureManifests,&failureRecoveredBook).ok);
 assert(failureRecoveredBook.find(failedContract.contractId)->status==oem::CONTRACT_EXPIRED);
 assert(failureRecoveredLedger.getBalance("company",balance)&&balance==475);
 assert(failureRecoveredLedger.getReputation("company",failureReputation)&&failureReputation==45);
 assert(store.load(failurePath,cat,failureRecoveredLedger,failureStocks,failureManifests,&failureRecoveredBook).ok);
 assert(failureRecoveredLedger.getBalance("company",balance)&&balance==475);
 std::remove(failurePath.c_str());

 // Failure fees are capped at the available balance; expiry still commits.
 const std::string poorFailurePath="OEMContractPoorFailureTests.journal";std::remove(poorFailurePath.c_str());
 oem::DeliveryContractBook poorFailureBook;
 oem::DeliveryContract poorContract=failedContract;
 poorContract.contractId="failure-delivery-poor";poorContract.reward=100;
 assert(store.acceptContract(poorFailurePath,poorFailureBook,poorContract).ok);
 oem::Ledger poorFailureLedger;assert(poorFailureLedger.createAccount("company",5).ok);
 assert(store.expireContract(poorFailurePath,poorFailureBook,poorContract.contractId,poorFailureLedger,"company",21).ok);
 assert(store.expireContract(poorFailurePath,poorFailureBook,poorContract.contractId,poorFailureLedger,"company",21).ok);
 assert(poorFailureBook.find(poorContract.contractId)->status==oem::CONTRACT_EXPIRED);
 assert(poorFailureLedger.getBalance("company",balance)&&balance==0);
 assert(poorFailureLedger.getReputation("company",failureReputation)&&failureReputation==45);
 oem::Ledger poorRecoveredLedger;assert(poorRecoveredLedger.createAccount("company",5).ok);
 oem::DeliveryContractBook poorRecoveredBook;
 std::map<std::string,oem::CargoServiceInventory> poorStocks;
 std::map<std::string,oem::CargoManifest> poorManifests;
 assert(store.load(poorFailurePath,cat,poorRecoveredLedger,poorStocks,poorManifests,&poorRecoveredBook).ok);
 assert(poorRecoveredLedger.getBalance("company",balance)&&balance==0);
 assert(poorRecoveredLedger.getReputation("company",failureReputation)&&failureReputation==45);
 std::remove(poorFailurePath.c_str());
 // A late destination event must flow through durable expiry, not silently
 // mark the contract expired before its configured fee and reputation loss commit.
 const std::string lateFailurePath="OEMContractLateFailureTests.journal";std::remove(lateFailurePath.c_str());
 oem::DeliveryContractBook lateFailureBook;
 oem::DeliveryContract lateFailure;
 lateFailure.contractId="late-failure-001";lateFailure.vesselId="GL-01";
 lateFailure.cargoId="oxygen";lateFailure.quantity=1;
 lateFailure.originLocationId="ISS";lateFailure.destinationLocationId="MARS";
 lateFailure.reward=200;lateFailure.acceptedAt=10;lateFailure.deadline=20;
 assert(store.acceptContract(lateFailurePath,lateFailureBook,lateFailure).ok);
 oem::CargoManifest lateManifest("GL-01");
 assert(lateManifest.load(transfer("late-failure-load","GL-01","ISS",oem::CARGO_LOAD,1,15)).ok);
 oem::SimulationEvent lateArrival;
 lateArrival.eventId="late-failure-arrival";lateArrival.type=oem::SIM_EVENT_LANDING;
 lateArrival.vesselId="GL-01";lateArrival.simulationTime=21;
 lateArrival.locationKnown=true;lateArrival.baseId="MARS";
 assert(!lateFailureBook.observe(lateArrival,lateManifest).ok);
 assert(lateFailureBook.find(lateFailure.contractId)->status==oem::CONTRACT_ACCEPTED);
 oem::MissionFailurePolicy customFailurePolicy;
 customFailurePolicy.penaltyBasisPoints=2500;customFailurePolicy.reputationDelta=-8;
 oem::Ledger lateFailureLedger;assert(lateFailureLedger.createAccount("company",1000).ok);
 assert(store.expireContract(lateFailurePath,lateFailureBook,lateFailure.contractId,
     lateFailureLedger,"company",21,customFailurePolicy).ok);
 assert(store.expireContract(lateFailurePath,lateFailureBook,lateFailure.contractId,
     lateFailureLedger,"company",21,customFailurePolicy).ok);
 assert(lateFailureLedger.getBalance("company",balance)&&balance==950);
 int lateFailureReputation=0;
 assert(lateFailureLedger.getReputation("company",lateFailureReputation)&&lateFailureReputation==42);
 oem::Ledger lateRecoveredLedger;assert(lateRecoveredLedger.createAccount("company",1000).ok);
 oem::DeliveryContractBook lateRecoveredBook;
 std::map<std::string,oem::CargoServiceInventory> lateStocks;
 std::map<std::string,oem::CargoManifest> lateManifests;
 assert(store.load(lateFailurePath,cat,lateRecoveredLedger,lateStocks,lateManifests,
     &lateRecoveredBook).ok);
 assert(lateRecoveredBook.find(lateFailure.contractId)->status==oem::CONTRACT_EXPIRED);
 assert(lateRecoveredLedger.getBalance("company",balance)&&balance==950);
 assert(lateRecoveredLedger.getReputation("company",lateFailureReputation)&&lateFailureReputation==42);
 assert(store.load(lateFailurePath,cat,lateRecoveredLedger,lateStocks,lateManifests,
     &lateRecoveredBook).ok);
 assert(lateRecoveredLedger.getBalance("company",balance)&&balance==950);
 assert(lateRecoveredLedger.getReputation("company",lateFailureReputation)&&lateFailureReputation==42);
 std::remove(lateFailurePath.c_str());
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
 // Legacy three-field Q/F expiry records remain readable and must not
 // retroactively charge the new penalty policy when an old save is loaded.
 const std::string legacyObjectivePath="OEMLegacyObjectiveExpiryTests.journal";
 std::remove(legacyObjectivePath.c_str());
 oem::MissionObjectiveBook legacyObjectiveBook;
 oem::MissionObjective legacyObjective;
 legacyObjective.missionId="legacy-objective-expired";
 legacyObjective.vesselId="GL-LEGACY";
 legacyObjective.kind=oem::MISSION_OBJECTIVE_VISIT_SPACEPORT;
 legacyObjective.targetLocationId="MoonBase";
 legacyObjective.targetBodyId="Moon";
 legacyObjective.acceptedAt=10;
 legacyObjective.deadline=20;
 assert(store.acceptMissionObjective(legacyObjectivePath,legacyObjectiveBook,
     legacyObjective,300).ok);
 {
  std::ofstream oldRecord(legacyObjectivePath.c_str(),std::ios::binary|std::ios::app);
  oldRecord << "Q|" << hex("mission-objective-expire:"+legacyObjective.missionId)
            << "|" << hex(legacyObjective.missionId) << "\\n";
 }
 oem::Ledger legacyObjectiveLedger;
 assert(legacyObjectiveLedger.createAccount("company",500).ok);
 oem::MissionObjectiveBook legacyObjectiveRecovered;
 oem::DeliveryContractBook legacyObjectiveContracts;
 std::map<std::string,oem::CargoServiceInventory> legacyObjectiveStocks;
 std::map<std::string,oem::CargoManifest> legacyObjectiveManifests;
 assert(store.load(legacyObjectivePath,cat,legacyObjectiveLedger,
     legacyObjectiveStocks,legacyObjectiveManifests,&legacyObjectiveContracts,
     &legacyObjectiveRecovered).ok);
 assert(legacyObjectiveRecovered.find(legacyObjective.missionId)->status==
     oem::MISSION_OBJECTIVE_EXPIRED);
 assert(legacyObjectiveLedger.getBalance("company",balance)&&balance==500);
 int legacyReputation=0;
 assert(!legacyObjectiveLedger.getReputation("company",legacyReputation));
 assert(store.load(legacyObjectivePath,cat,legacyObjectiveLedger,
     legacyObjectiveStocks,legacyObjectiveManifests,&legacyObjectiveContracts,
     &legacyObjectiveRecovered).ok);
 assert(legacyObjectiveLedger.getBalance("company",balance)&&balance==500);
 assert(!legacyObjectiveLedger.getReputation("company",legacyReputation));
 std::remove(legacyObjectivePath.c_str());

 const std::string legacyContractPath="OEMLegacyContractExpiryTests.journal";
 std::remove(legacyContractPath.c_str());
 oem::DeliveryContract legacyContract;
 legacyContract.contractId="legacy-contract-expired";
 legacyContract.vesselId="GL-LEGACY";
 legacyContract.cargoId="oxygen";
 legacyContract.quantity=1;
 legacyContract.originLocationId="ISS";
 legacyContract.destinationLocationId="MARS";
 legacyContract.reward=300;
 legacyContract.acceptedAt=10;
 legacyContract.deadline=20;
 oem::DeliveryContractBook legacyContractBook;
 assert(store.acceptContract(legacyContractPath,legacyContractBook,legacyContract).ok);
 {
  std::ofstream oldRecord(legacyContractPath.c_str(),std::ios::binary|std::ios::app);
  oldRecord << "F|" << hex("contract-expire:"+legacyContract.contractId)
            << "|" << hex(legacyContract.contractId) << "\\n";
 }
 oem::Ledger legacyContractLedger;
 assert(legacyContractLedger.createAccount("company",500).ok);
 oem::DeliveryContractBook legacyContractRecovered;
 std::map<std::string,oem::CargoServiceInventory> legacyContractStocks;
 std::map<std::string,oem::CargoManifest> legacyContractManifests;
 assert(store.load(legacyContractPath,cat,legacyContractLedger,
     legacyContractStocks,legacyContractManifests,&legacyContractRecovered).ok);
 assert(legacyContractRecovered.find(legacyContract.contractId)->status==
     oem::CONTRACT_EXPIRED);
 assert(legacyContractLedger.getBalance("company",balance)&&balance==500);
 assert(!legacyContractLedger.getReputation("company",legacyReputation));
 assert(store.load(legacyContractPath,cat,legacyContractLedger,
     legacyContractStocks,legacyContractManifests,&legacyContractRecovered).ok);
 assert(legacyContractLedger.getBalance("company",balance)&&balance==500);
 assert(!legacyContractLedger.getReputation("company",legacyReputation));
 std::remove(legacyContractPath.c_str());

 std::remove(cancelPath.c_str());
 std::remove(contractPath.c_str());
 std::remove(path.c_str());return 0;
}
