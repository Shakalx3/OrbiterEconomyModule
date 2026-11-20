cmake_minimum_required(VERSION 3.10)
project(OrbiterEconomyModule LANGUAGES CXX)
add_library(OEMCore
    src/core/Ledger.cpp
    src/core/CargoManifest.cpp
    src/core/CargoTransferGate.cpp
    src/core/CargoManifestJournal.cpp
    src/core/SpaceportLanding.cpp
    src/core/SimulationEvent.cpp
    src/core/SimulationEventJournal.cpp
    src/core/FlightTransition.cpp
    src/core/DockingTransition.cpp
)
target_include_directories(OEMCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/core)
add_executable(OEMCoreTests Tests/core/LedgerTests.cpp)
add_executable(OEMCargoManifestTests Tests/core/CargoManifestTests.cpp)
add_executable(OEMCargoManifestJournalTests Tests/core/CargoManifestJournalTests.cpp)
add_executable(OEMCargoTransferGateTests Tests/core/CargoTransferGateTests.cpp)
target_link_libraries(OEMCargoTransferGateTests PRIVATE OEMCore)
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
enable_testing()
add_test(NAME OEMCoreTests COMMAND OEMCoreTests)
add_test(NAME OEMCargoManifestTests COMMAND OEMCargoManifestTests)
add_test(NAME OEMCargoManifestJournalTests COMMAND OEMCargoManifestJournalTests)
add_test(NAME OEMCargoTransferGateTests COMMAND OEMCargoTransferGateTests)
add_test(NAME OEMSpaceportLandingTests COMMAND OEMSpaceportLandingTests)
add_test(NAME OEMSimulationEventTests COMMAND OEMSimulationEventTests)
add_test(NAME OEMSimulationEventJournalTests COMMAND OEMSimulationEventJournalTests)
add_test(NAME OEMDockingTransitionTests COMMAND OEMDockingTransitionTests)
