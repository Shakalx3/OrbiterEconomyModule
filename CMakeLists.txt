cmake_minimum_required(VERSION 3.10)
project(OrbiterEconomyModule LANGUAGES CXX)

add_library(OEMCore
    src/core/Ledger.cpp
    src/core/SpaceportLanding.cpp
    src/core/SimulationEvent.cpp
    src/core/SimulationEventJournal.cpp
    src/core/FlightTransition.cpp
)
target_include_directories(OEMCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/core)

add_executable(OEMCoreTests
    Tests/core/LedgerTests.cpp
)
target_link_libraries(OEMCoreTests PRIVATE OEMCore)

add_executable(OEMSpaceportLandingTests
    Tests/core/SpaceportLandingTests.cpp
)
target_link_libraries(OEMSpaceportLandingTests PRIVATE OEMCore)

add_executable(OEMSimulationEventTests
    Tests/core/SimulationEventTests.cpp
)
target_link_libraries(OEMSimulationEventTests PRIVATE OEMCore)

add_executable(OEMSimulationEventJournalTests
    Tests/core/SimulationEventJournalTests.cpp
)
target_link_libraries(OEMSimulationEventJournalTests PRIVATE OEMCore)

enable_testing()
add_test(NAME OEMCoreTests COMMAND OEMCoreTests)
add_test(NAME OEMSpaceportLandingTests COMMAND OEMSpaceportLandingTests)
add_test(NAME OEMSimulationEventTests COMMAND OEMSimulationEventTests)
add_test(NAME OEMSimulationEventJournalTests COMMAND OEMSimulationEventJournalTests)
