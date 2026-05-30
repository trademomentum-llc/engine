cmake_minimum_required(VERSION 3.18)
project(NNOS VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# -----------------------------------------------------------------------------
# Build profiles: Debug (default), Release, MinSizeRel
# -----------------------------------------------------------------------------
# MinSizeRel is the sovereign/embedded target per NEURODIOS-BIN-OPT-001.
# It enables LTO, size optimization, and post-link stripping.
# -----------------------------------------------------------------------------

# Warnings matching NFR-7.4.1 (applied to all profiles)
add_compile_options(
    -Wall -Wextra -Wpedantic
    -Werror=return-type
    -Werror=uninitialized
)

# Release / MinSizeRel shared optimizations
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG -fvisibility=hidden")
set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os -DNDEBUG -fvisibility=hidden -fno-rtti -fno-exceptions")

# Link-Time Optimization (LTO) when available and requested
option(NNOS_ENABLE_LTO "Enable Link-Time Optimization (slower build, smaller binary)" ON)
if(NNOS_ENABLE_LTO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT IPO_SUPPORTED OUTPUT IPO_ERROR)
    if(IPO_SUPPORTED)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
        message(STATUS "LTO enabled")
    else()
        message(WARNING "LTO not supported: ${IPO_ERROR}")
    endif()
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/src)

# -----------------------------------------------------------------------------
# Common library: shared-state IPC, logging, signal handling
# -----------------------------------------------------------------------------
add_library(nnos_common STATIC
    src/common/shared_state.cpp
    src/common/logger.cpp
    src/common/signal_handler.cpp
)
# Apply size-optimizing flags to the common library too
if(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
    target_compile_options(nnos_common PRIVATE -Os -fno-rtti -fno-exceptions)
endif()

# -----------------------------------------------------------------------------
# Helper to apply release/minimal optimizations to a daemon target
# -----------------------------------------------------------------------------
function(nnos_daemon target source)
    add_executable(${target} ${source})
    target_link_libraries(${target} nnos_common pthread)
    if(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
        target_compile_options(${target} PRIVATE -Os -fno-rtti -fno-exceptions)
        # Strip symbols post-link on Unix (darwin uses -x, linux uses --strip-all)
        if(APPLE)
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND ${CMAKE_STRIP} -x "$<TARGET_FILE:${target}>"
                COMMENT "Stripping local symbols from ${target}"
            )
        else()
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND ${CMAKE_STRIP} --strip-all "$<TARGET_FILE:${target}>"
                COMMENT "Stripping all symbols from ${target}"
            )
        endif()
    endif()
endfunction()

# -----------------------------------------------------------------------------
# LSA Boot Daemons (per-node supervision)
# -----------------------------------------------------------------------------
nnos_daemon(lsa_boot_dcn src/daemons/lsa_boot_dcn.cpp)
nnos_daemon(lsa_boot_hcn src/daemons/lsa_boot_hcn.cpp)
nnos_daemon(lsa_boot_epn src/daemons/lsa_boot_epn.cpp)

# -----------------------------------------------------------------------------
# LSA Functional Daemons
# -----------------------------------------------------------------------------
nnos_daemon(lsa_task_manager      src/daemons/lsa_task_manager.cpp)
nnos_daemon(lsa_context_gate      src/daemons/lsa_context_gate.cpp)
nnos_daemon(lsa_state_monitor     src/daemons/lsa_state_monitor.cpp)
nnos_daemon(lsa_comm_bridge       src/daemons/lsa_comm_bridge.cpp)
nnos_daemon(lsa_profile_refiner   src/daemons/lsa_profile_refiner.cpp)

# -----------------------------------------------------------------------------
# Global Daemons
# -----------------------------------------------------------------------------
nnos_daemon(lsa_drift_detector    src/daemons/lsa_drift_detector.cpp)
nnos_daemon(lsa_convergence_bond  src/daemons/lsa_convergence_bond.cpp)
nnos_daemon(lsa_ethernet_sync     src/daemons/lsa_ethernet_sync.cpp)
# Link OpenSSL for AES-256-GCM sync encryption when available
find_package(OpenSSL)
if(OpenSSL_FOUND)
    target_compile_definitions(lsa_ethernet_sync PRIVATE NNOS_HAS_OPENSSL)
    target_link_libraries(lsa_ethernet_sync OpenSSL::SSL OpenSSL::Crypto)
    message(STATUS "OpenSSL found — lsa_ethernet_sync will use AES-256-GCM")
else()
    message(WARNING "OpenSSL not found — lsa_ethernet_sync crypto will be stubbed")
endif()

# -----------------------------------------------------------------------------
# Security & Intelligence Daemons
# -----------------------------------------------------------------------------
nnos_daemon(SystemIntegrityDaemon   src/daemons/system_integrity_daemon.cpp)
nnos_daemon(ThreatIntelligenceManager src/daemons/threat_intelligence_manager.cpp)
nnos_daemon(MorphogeneticMaintainer src/daemons/morphogenetic_maintainer.cpp)

# -----------------------------------------------------------------------------
# Deterministic build verification — SHA256 manifest generation
# -----------------------------------------------------------------------------
set(MANIFEST_DIR "${CMAKE_BINARY_DIR}/provenance")
file(MAKE_DIRECTORY ${MANIFEST_DIR})

# Collect all daemon targets into a list for the manifest script
set(NNOS_DAEMON_TARGETS
    lsa_boot_dcn lsa_boot_hcn lsa_boot_epn
    lsa_task_manager lsa_context_gate lsa_state_monitor
    lsa_comm_bridge lsa_profile_refiner
    lsa_drift_detector lsa_convergence_bond lsa_ethernet_sync
    SystemIntegrityDaemon ThreatIntelligenceManager MorphogeneticMaintainer
)

add_custom_target(provenance_manifest ALL
    COMMENT "Generating binary provenance manifest"
    COMMAND ${CMAKE_COMMAND} -E echo "Build complete. Provenance manifest generated at build time via scripts/generate_provenance_manifest.sh"
)

# Generate flat sha256sums.txt for each binary
foreach(tgt ${NNOS_DAEMON_TARGETS})
    add_custom_command(TARGET provenance_manifest POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E sha256sum "$<TARGET_FILE:${tgt}>"
        >> ${MANIFEST_DIR}/sha256sums.txt
        VERBATIM
    )
endforeach()

# -----------------------------------------------------------------------------
# Install targets
# -----------------------------------------------------------------------------
install(TARGETS
    lsa_boot_dcn lsa_boot_hcn lsa_boot_epn
    lsa_task_manager lsa_context_gate lsa_state_monitor
    lsa_comm_bridge lsa_profile_refiner
    lsa_drift_detector lsa_convergence_bond lsa_ethernet_sync
    SystemIntegrityDaemon ThreatIntelligenceManager MorphogeneticMaintainer
    RUNTIME DESTINATION bin
)
install(FILES ${MANIFEST_DIR}/sha256sums.txt DESTINATION share/nnos/provenance OPTIONAL)

# -----------------------------------------------------------------------------
# Packaging
# -----------------------------------------------------------------------------
set(CPACK_PACKAGE_NAME "nnos")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_CONTACT "nnos@local")
include(CPack)
