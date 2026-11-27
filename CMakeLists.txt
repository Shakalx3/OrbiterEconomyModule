cmake_minimum_required(VERSION 3.10)
project(OrbiterEconomyModule LANGUAGES CXX)
add_library(OEMCore
    src/core/Ledger.cpp
    src/core/LedgerConfig.cpp
    src/core/CargoCatalogConfig.cpp
    src/core/Market.cpp
    src/core/MarketTrade.cpp
    src/core/MarketTradeJournal.cpp
    src/core/EconomyJournal.cpp
    src/core/LegacyEconomyInspector.cpp
    src/core/CargoManifest.cpp
    src/core/CargoTransferGate.cpp
    src/core/CargoServiceRegistry.cpp
    src/core/CargoServiceInventory.cpp
    src/core/CargoExchangeJournal.cpp
    src/core/CargoServiceInventoryConfig.cpp
    src/core/CargoManifestJournal.cpp
    src/core/SpaceportLanding.cpp
    src/core/SimulationEvent.cpp
    src/core/SimulationEventJournal.cpp
    src/core/FlightTransition.cpp
    src/core/DockingTransition.cpp
    src/core/DeliveryContract.cpp
    src/core/MissionBoard.cpp
    src/core/MissionObjective.cpp
    src/core/MissionObjectiveBook.cpp
    src/core/MissionObjectiveBoard.cpp
    src/core/MissionIssuerRegistry.cpp
    src/core/MissionEconomics.cpp
    src/core/SpaceportRegistry.cpp
    src/core/EconomicLocationRegistry.cpp
    src/core/CampaignReset.cpp
    src/core/EconomicStationRegistry.cpp
    src/core/PropellantPlan.cpp
    src/core/PropellantQuote.cpp
    src/core/PropellantTransaction.cpp
    src/core/PropellantTransactionJournal.cpp
    src/core/EconomicAgent.cpp
    src/core/EconomicAgentConfig.cpp
    src/core/PrimaryResource.cpp
    src/core/PrimaryResourceConfig.cpp
    src/core/ProductionRecipe.cpp
    src/core/ProductionRecipeConfig.cpp
    src/core/ShipOwnership.cpp
)
target_include_directories(OEMCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/core)
add_executable(OEMCoreTests Tests/core/LedgerTests.cpp)
add_executable(OEMLedgerConfigTests Tests/core/LedgerConfigTests.cpp)
target_link_libraries(OEMLedgerConfigTests PRIVATE OEMCore)
add_executable(OEMCargoCatalogConfigTests Tests/core/CargoCatalogConfigTests.cpp)
target_link_libraries(OEMCargoCatalogConfigTests PRIVATE OEMCore)
add_executable(OEMMarketTests Tests/core/MarketTests.cpp)
add_executable(OEMMarketPricingTests Tests/core/MarketPricingTests.cpp)
target_link_libraries(OEMMarketPricingTests PRIVATE OEMCore)
target_link_libraries(OEMMarketTests PRIVATE OEMCore)
add_executable(OEMMarketTradeTests Tests/core/MarketTradeTests.cpp)
target_link_libraries(OEMMarketTradeTests PRIVATE OEMCore)
add_executable(OEMMarketCycleTests Tests/core/MarketCycleTests.cpp)
target_link_libraries(OEMMarketCycleTests PRIVATE OEMCore)
add_executable(OEMMarketTradeJournalTests Tests/core/MarketTradeJournalTests.cpp)
add_executable(OEMEconomyJournalTests Tests/core/EconomyJournalTests.cpp)
add_executable(OEMLegacyEconomyInspectorTests Tests/core/LegacyEconomyInspectorTests.cpp)
target_link_libraries(OEMLegacyEconomyInspectorTests PRIVATE OEMCore)
target_link_libraries(OEMEconomyJournalTests PRIVATE OEMCore)
target_link_libraries(OEMMarketTradeJournalTests PRIVATE OEMCore)
add_executable(OEMCargoManifestTests Tests/core/CargoManifestTests.cpp)
add_executable(OEMCargoManifestJournalTests Tests/core/CargoManifestJournalTests.cpp)
add_executable(OEMCargoTransferGateTests Tests/core/CargoTransferGateTests.cpp)
add_executable(OEMCargoServiceRegistryTests Tests/core/CargoServiceRegistryTests.cpp)
add_executable(OEMCargoServiceInventoryTests Tests/core/CargoServiceInventoryTests.cpp)
add_executable(OEMCargoExchangeJournalTests Tests/core/CargoExchangeJournalTests.cpp)
add_executable(OEMCargoServiceInventoryConfigTests Tests/core/CargoServiceInventoryConfigTests.cpp)
target_link_libraries(OEMCargoTransferGateTests PRIVATE OEMCore)
target_link_libraries(OEMCargoServiceRegistryTests PRIVATE OEMCore)
target_link_libraries(OEMCargoServiceInventoryTests PRIVATE OEMCore)
target_link_libraries(OEMCargoExchangeJournalTests PRIVATE OEMCore)
target_link_libraries(OEMCargoServiceInventoryConfigTests PRIVATE OEMCore)
target_link_libraries(OEMCargoManifestJournalTests PRIVATE OEMCore)
target_link_libraries(OEMCargoManifestTests PRIVATE OEMCore)
target_link_libraries(OEMCoreTests PRIVATE OEMCore)
add_executable(OEMSpaceportLandingTests Tests/core/SpaceportLandingTests.cpp)
target_link_libraries(OEMSpaceportLandingTests PRIVATE OEMCore)
add_executable(OEMSimulationEventTests Tests/core/SimulationEventTests.cpp)
target_link_libraries(OEMSimulationEventTests PRIVATE OEMCore)
add_executable(OEMSimulationEventJournalTests Tests/core/SimulationEventJournalTests.cpp)
target_link_libraries(OEMSimulationEventJournalTests PRIVATE OEMCore)
add_executable(OEMDockingTransitionTests Tests/core/DockingTransitionTests.cpp)
target_link_libraries(OEMDockingTransitionTests PRIVATE OEMCore)
add_executable(OEMDeliveryContractTests Tests/core/DeliveryContractTests.cpp)
add_executable(OEMMissionBoardTests Tests/core/MissionBoardTests.cpp)
add_executable(OEMMissionObjectiveTests Tests/core/MissionObjectiveTests.cpp)
add_executable(OEMMissionObjectiveBookTests Tests/core/MissionObjectiveBookTests.cpp)
add_executable(OEMMissionObjectiveJournalTests Tests/core/MissionObjectiveJournalTests.cpp)
add_executable(OEMMissionObjectiveBoardTests Tests/core/MissionObjectiveBoardTests.cpp)
add_executable(OEMMissionIssuerRegistryTests Tests/core/MissionIssuerRegistryTests.cpp)
target_link_libraries(OEMMissionIssuerRegistryTests PRIVATE OEMCore)
add_executable(OEMMissionEconomicsTests Tests/core/MissionEconomicsTests.cpp)
target_link_libraries(OEMMissionEconomicsTests PRIVATE OEMCore)
target_link_libraries(OEMMissionBoardTests PRIVATE OEMCore)
target_link_libraries(OEMMissionObjectiveTests PRIVATE OEMCore)
target_link_libraries(OEMMissionObjectiveBookTests PRIVATE OEMCore)
target_link_libraries(OEMMissionObjectiveJournalTests PRIVATE OEMCore)
target_link_libraries(OEMMissionObjectiveBoardTests PRIVATE OEMCore)
target_link_libraries(OEMDeliveryContractTests PRIVATE OEMCore)
add_executable(OEMSpaceportRegistryTests Tests/core/SpaceportRegistryTests.cpp)
add_executable(OEMEconomicLocationRegistryTests Tests/core/EconomicLocationRegistryTests.cpp)
add_executable(OEMCampaignResetTests Tests/core/CampaignResetTests.cpp)
add_executable(OEMEconomicStationRegistryTests Tests/core/EconomicStationRegistryTests.cpp)
add_executable(OEMPropellantPlanTests Tests/core/PropellantPlanTests.cpp)
target_link_libraries(OEMPropellantPlanTests PRIVATE OEMCore)
add_executable(OEMPropellantQuoteTests Tests/core/PropellantQuoteTests.cpp)
target_link_libraries(OEMPropellantQuoteTests PRIVATE OEMCore)
add_executable(OEMPropellantTransactionTests Tests/core/PropellantTransactionTests.cpp)
target_link_libraries(OEMPropellantTransactionTests PRIVATE OEMCore)
add_executable(OEMPropellantTransactionJournalTests Tests/core/PropellantTransactionJournalTests.cpp)
add_executable(OEMEconomicAgentTests Tests/core/EconomicAgentTests.cpp)
add_executable(OEMEconomicAgentConfigTests Tests/core/EconomicAgentConfigTests.cpp)
target_link_libraries(OEMEconomicAgentConfigTests PRIVATE OEMCore)
add_executable(OEMPrimaryResourceTests Tests/core/PrimaryResourceTests.cpp)
add_executable(OEMProductionRecipeTests Tests/core/ProductionRecipeTests.cpp)
target_link_libraries(OEMProductionRecipeTests PRIVATE OEMCore)
add_executable(OEMProductionRecipeConfigTests Tests/core/ProductionRecipeConfigTests.cpp)
add_executable(OEMShipOwnershipTests Tests/core/ShipOwnershipTests.cpp)
add_executable(OEMShipPurchaseJournalTests Tests/core/ShipPurchaseJournalTests.cpp)
target_link_libraries(OEMShipPurchaseJournalTests PRIVATE OEMCore)
target_link_libraries(OEMShipOwnershipTests PRIVATE OEMCore)
target_link_libraries(OEMProductionRecipeConfigTests PRIVATE OEMCore)
target_link_libraries(OEMPrimaryResourceTests PRIVATE OEMCore)
target_link_libraries(OEMEconomicAgentTests PRIVATE OEMCore)
target_link_libraries(OEMPropellantTransactionJournalTests PRIVATE OEMCore)
target_link_libraries(OEMEconomicLocationRegistryTests PRIVATE OEMCore)
target_link_libraries(OEMCampaignResetTests PRIVATE OEMCore)
target_link_libraries(OEMEconomicStationRegistryTests PRIVATE OEMCore)
target_link_libraries(OEMSpaceportRegistryTests PRIVATE OEMCore)
target_link_libraries(OEMDeliveryContractTests PRIVATE OEMCore)
enable_testing()
add_test(NAME OEMCoreTests COMMAND OEMCoreTests)
add_test(NAME OEMLedgerConfigTests COMMAND OEMLedgerConfigTests)
add_test(NAME OEMCargoCatalogConfigTests COMMAND OEMCargoCatalogConfigTests)
add_test(NAME OEMMarketTests COMMAND OEMMarketTests)
add_test(NAME OEMMarketPricingTests COMMAND OEMMarketPricingTests)
add_test(NAME OEMMarketTradeTests COMMAND OEMMarketTradeTests)
add_test(NAME OEMMarketCycleTests COMMAND OEMMarketCycleTests)
add_test(NAME OEMMarketTradeJournalTests COMMAND OEMMarketTradeJournalTests)
add_test(NAME OEMEconomyJournalTests COMMAND OEMEconomyJournalTests)
add_test(NAME OEMLegacyEconomyInspectorTests COMMAND OEMLegacyEconomyInspectorTests)
add_test(NAME OEMCargoManifestTests COMMAND OEMCargoManifestTests)
add_test(NAME OEMCargoManifestJournalTests COMMAND OEMCargoManifestJournalTests)
add_test(NAME OEMCargoTransferGateTests COMMAND OEMCargoTransferGateTests)
add_test(NAME OEMCargoServiceRegistryTests COMMAND OEMCargoServiceRegistryTests)
add_test(NAME OEMCargoServiceInventoryTests COMMAND OEMCargoServiceInventoryTests)
add_test(NAME OEMCargoExchangeJournalTests COMMAND OEMCargoExchangeJournalTests)
add_test(NAME OEMCargoServiceInventoryConfigTests COMMAND OEMCargoServiceInventoryConfigTests)
add_test(NAME OEMSpaceportLandingTests COMMAND OEMSpaceportLandingTests)
add_test(NAME OEMSimulationEventTests COMMAND OEMSimulationEventTests)
add_test(NAME OEMSimulationEventJournalTests COMMAND OEMSimulationEventJournalTests)
add_test(NAME OEMDockingTransitionTests COMMAND OEMDockingTransitionTests)
add_test(NAME OEMDeliveryContractTests COMMAND OEMDeliveryContractTests)
add_test(NAME OEMMissionBoardTests COMMAND OEMMissionBoardTests)
add_test(NAME OEMMissionObjectiveTests COMMAND OEMMissionObjectiveTests)
add_test(NAME OEMMissionObjectiveBookTests COMMAND OEMMissionObjectiveBookTests)
add_test(NAME OEMMissionObjectiveJournalTests COMMAND OEMMissionObjectiveJournalTests)
add_test(NAME OEMMissionObjectiveBoardTests COMMAND OEMMissionObjectiveBoardTests)
add_test(NAME OEMMissionIssuerRegistryTests COMMAND OEMMissionIssuerRegistryTests)
add_test(NAME OEMMissionEconomicsTests COMMAND OEMMissionEconomicsTests)
add_test(NAME OEMSpaceportRegistryTests COMMAND OEMSpaceportRegistryTests)
add_test(NAME OEMEconomicLocationRegistryTests COMMAND OEMEconomicLocationRegistryTests)
add_test(NAME OEMCampaignResetTests COMMAND OEMCampaignResetTests)
add_test(NAME OEMEconomicStationRegistryTests COMMAND OEMEconomicStationRegistryTests)
add_test(NAME OEMPropellantPlanTests COMMAND OEMPropellantPlanTests)
add_test(NAME OEMPropellantQuoteTests COMMAND OEMPropellantQuoteTests)
add_test(NAME OEMPropellantTransactionTests COMMAND OEMPropellantTransactionTests)
add_test(NAME OEMPropellantTransactionJournalTests COMMAND OEMPropellantTransactionJournalTests)
add_test(NAME OEMEconomicAgentTests COMMAND OEMEconomicAgentTests)
add_test(NAME OEMEconomicAgentConfigTests COMMAND OEMEconomicAgentConfigTests)
add_test(NAME OEMPrimaryResourceTests COMMAND OEMPrimaryResourceTests)
add_test(NAME OEMProductionRecipeTests COMMAND OEMProductionRecipeTests)
add_test(NAME OEMProductionRecipeConfigTests COMMAND OEMProductionRecipeConfigTests)
add_test(NAME OEMShipOwnershipTests COMMAND OEMShipOwnershipTests)
add_test(NAME OEMShipPurchaseJournalTests COMMAND OEMShipPurchaseJournalTests)
