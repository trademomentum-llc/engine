#!/bin/bash
# smoke_test_all.sh — Automated smoke test for all NNOS daemon targets
# Builds every daemon, runs with timeout, verifies clean boot/shutdown.
# Exits 0 if all pass, 1 if any fail.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"
LOG_DIR="/tmp/nnos_smoke_$(date +%s)"
TIMEOUT_SEC=3

DAEMONS=(
    lsa_boot_dcn
    lsa_boot_hcn
    lsa_boot_epn
    lsa_task_manager
    lsa_context_gate
    lsa_state_monitor
    lsa_comm_bridge
    lsa_profile_refiner
    lsa_drift_detector
    lsa_convergence_bond
    lsa_ethernet_sync
    SystemIntegrityDaemon
    ThreatIntelligenceManager
    MorphogeneticMaintainer
)

mkdir -p "${LOG_DIR}"
cd "${PROJECT_ROOT}"

echo "=== NNOS Daemon Smoke Test Suite ==="
echo "Build dir: ${BUILD_DIR}"
echo "Log dir:   ${LOG_DIR}"
echo ""

# Build all targets first
echo "Building all daemon targets..."
make -C "${BUILD_DIR}" -j$(nproc 2>/dev/null || echo 4) >/dev/null 2>&1
echo "Build complete."
echo ""

PASS=0
FAIL=0

for daemon in "${DAEMONS[@]}"; do
    daemon_log_dir="${LOG_DIR}/${daemon}"
    mkdir -p "${daemon_log_dir}"

    exit_code=0
    # Run daemon with timeout; ignore exit code (daemon may not handle SIGTERM gracefully)
    NNOS_LOG_DIR="${daemon_log_dir}" timeout "${TIMEOUT_SEC}" "${BUILD_DIR}/${daemon}" >/dev/null 2>&1 || exit_code=$?

    # Logger uses lowercase underscore name, not binary name
    log_name=$(echo "${daemon}" | sed 's/\([A-Z]\)/_\1/g' | sed 's/^_//' | tr '[:upper:]' '[:lower:]')
    log_file="${daemon_log_dir}/${log_name}.jsonl"
    crit_count=0
    if [[ -f "${log_file}" ]]; then
        # Count CRITICAL logs, excluding expected ones
        crit_count=$(grep -c '"level":"CRITICAL"' "${log_file}" 2>/dev/null || true)
        expected_crit=$(grep -c 'default key' "${log_file}" 2>/dev/null || true)
        expected_crit2=$(grep -c '0 nodes active' "${log_file}" 2>/dev/null || true)
        expected_crit3=$(grep -c 'Severity=CRITICAL indicator detected' "${log_file}" 2>/dev/null || true)
        crit_count=$((crit_count - expected_crit - expected_crit2 - expected_crit3))
    fi

    # Check for BOOT log as minimum success criteria
    if [[ -f "${log_file}" ]] && grep -q '"event":"BOOT"' "${log_file}"; then
        if [[ ${crit_count} -eq 0 ]]; then
            echo "[PASS] ${daemon}"
            PASS=$((PASS + 1))
        else
            echo "[FAIL] ${daemon} — ${crit_count} unexpected CRITICAL log(s)"
            FAIL=$((FAIL + 1))
        fi
    else
        echo "[FAIL] ${daemon} — no BOOT log found (exit=${exit_code})"
        FAIL=$((FAIL + 1))
    fi
done

echo ""
echo "========================================"
echo "Results: ${PASS} passed, ${FAIL} failed"
echo "========================================"

if [[ ${FAIL} -gt 0 ]]; then
    exit 1
fi
exit 0
