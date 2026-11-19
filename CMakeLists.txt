cmake_minimum_required(VERSION 3.10)
project(OrbiterEconomyModule LANGUAGES CXX)

add_library(OEMCore
    src/core/Ledger.cpp
)
target_include_directories(OEMCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/core)

add_executable(OEMCoreTests
    Tests/core/LedgerTests.cpp
)
target_link_libraries(OEMCoreTests PRIVATE OEMCore)

enable_testing()
add_test(NAME OEMCoreTests COMMAND OEMCoreTests)
