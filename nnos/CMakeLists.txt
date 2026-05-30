cmake_minimum_required(VERSION 3.18)
project(NNOS VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Warnings matching NFR-7.4.1
add_compile_options(
    -Wall -Wextra -Wpedantic
    -Werror=return-type
    -Werror=uninitialized
)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/src)

# ------------------------------------------------------------------------------
# Common library: shared-state IPC, logging, signal handling
# ------------------------------------------------------------------------------
add_library(nnos_common STATIC
    src/common/shared_state.cpp
    src/common/logger.cpp
    src/common/signal_handler.cpp
)

# ------------------------------------------------------------------------------
# LSA Boot Daemons (per-node supervision)
# ------------------------------------------------------------------------------
add_executable(lsa_boot_dcn src/daemons/lsa_boot_dcn.cpp)
target_link_libraries(lsa_boot_dcn nnos_common pthread)

add_executable(lsa_boot_hcn src/daemons/lsa_boot_hcn.cpp)
target_link_libraries(lsa_boot_hcn nnos_common pthread)

add_executable(lsa_boot_epn src/daemons/lsa_boot_epn.cpp)
target_link_libraries(lsa_boot_epn nnos_common pthread)

# ------------------------------------------------------------------------------
# LSA Functional Daemons (from existing C++ headers)
# ------------------------------------------------------------------------------
add_executable(lsa_task_manager src/daemons/lsa_task_manager.cpp)
target_link_libraries(lsa_task_manager nnos_common pthread)

add_executable(lsa_context_gate src/daemons/lsa_context_gate.cpp)
target_link_libraries(lsa_context_gate nnos_common pthread)

add_executable(lsa_state_monitor src/daemons/lsa_state_monitor.cpp)
target_link_libraries(lsa_state_monitor nnos_common pthread)

add_executable(lsa_comm_bridge src/daemons/lsa_comm_bridge.cpp)
target_link_libraries(lsa_comm_bridge nnos_common pthread)

add_executable(lsa_profile_refiner src/daemons/lsa_profile_refiner.cpp)
target_link_libraries(lsa_profile_refiner nnos_common pthread)

# ------------------------------------------------------------------------------
# LSA Functional Daemons (new — no prior C++ code)
# ------------------------------------------------------------------------------
add_executable(lsa_drift_detector src/daemons/lsa_drift_detector.cpp)
target_link_libraries(lsa_drift_detector nnos_common pthread)

add_executable(lsa_convergence_bond src/daemons/lsa_convergence_bond.cpp)
target_link_libraries(lsa_convergence_bond nnos_common pthread)

add_executable(lsa_ethernet_sync src/daemons/lsa_ethernet_sync.cpp)
target_link_libraries(lsa_ethernet_sync nnos_common pthread)

# ------------------------------------------------------------------------------
# Security & Intelligence Daemons (baseline constellation)
# ------------------------------------------------------------------------------
add_executable(SystemIntegrityDaemon src/daemons/system_integrity_daemon.cpp)
target_link_libraries(SystemIntegrityDaemon nnos_common pthread)

add_executable(ThreatIntelligenceManager src/daemons/threat_intelligence_manager.cpp)
target_link_libraries(ThreatIntelligenceManager nnos_common pthread)

add_executable(MorphogeneticMaintainer src/daemons/morphogenetic_maintainer.cpp)
target_link_libraries(MorphogeneticMaintainer nnos_common pthread)

# ------------------------------------------------------------------------------
# Install targets
# ------------------------------------------------------------------------------
install(TARGETS
    lsa_boot_dcn lsa_boot_hcn lsa_boot_epn
    lsa_task_manager lsa_context_gate lsa_state_monitor
    lsa_comm_bridge lsa_profile_refiner
    lsa_drift_detector lsa_convergence_bond lsa_ethernet_sync
    SystemIntegrityDaemon ThreatIntelligenceManager MorphogeneticMaintainer
    RUNTIME DESTINATION bin
)

# ------------------------------------------------------------------------------
# Packaging
# ------------------------------------------------------------------------------
set(CPACK_PACKAGE_NAME "nnos")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_CONTACT "nnos@local")
include(CPack)
