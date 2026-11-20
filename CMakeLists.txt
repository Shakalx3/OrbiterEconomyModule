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
)
target_include_directories(OEMCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/core)
add_executable(OEMCoreTests Tests/core/LedgerTests.cpp)
add_executable(OEMLedgerConfigTests Tests/core/LedgerConfigTests.cpp)
target_link_libraries(OEMLedgerConfigTests PRIVATE OEMCore)
add_executable(OEMCargoCatalogConfigTests Tests/core/CargoCatalogConfigTests.cpp)
target_link_libraries(OEMCargoCatalogConfigTests PRIVATE OEMCore)
add_executable(OEMMarketTests Tests/core/MarketTests.cpp)
target_link_libraries(OEMMarketTests PRIVATE OEMCore)
add_executable(OEMMarketTradeTests Tests/core/MarketTradeTests.cpp)
target_link_libraries(OEMMarketTradeTests PRIVATE OEMCore)
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
target_link_libraries(OEMDeliveryContractTests PRIVATE OEMCore)
enable_testing()
add_test(NAME OEMCoreTests COMMAND OEMCoreTests)
add_test(NAME OEMLedgerConfigTests COMMAND OEMLedgerConfigTests)
add_test(NAME OEMCargoCatalogConfigTests COMMAND OEMCargoCatalogConfigTests)
add_test(NAME OEMMarketTests COMMAND OEMMarketTests)
add_test(NAME OEMMarketTradeTests COMMAND OEMMarketTradeTests)
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
