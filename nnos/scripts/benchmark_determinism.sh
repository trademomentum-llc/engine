#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
OUTPUT_DIR="${ROOT_DIR}/build/benchmarks"

RUNS="${RUNS:-100}"
ENV_NAME="native"
RUN_COMMAND=""
FIXTURE_PATH=""
INTERFERENCE_COMMAND=""
LABEL=""

usage() {
    cat <<'EOF'
Usage: benchmark_determinism.sh --command "<cmd>" --fixture <file> [options]

Required:
  --command <cmd>     Shell command to benchmark. The fixture file is provided on stdin.
  --fixture <file>    Deterministic input fixture used for every run.

Options:
  --env <name>        Label the execution environment: native, vm, docker, podman.
  --runs <count>      Number of repetitions, default 100.
  --label <name>      Optional report label. Defaults to env plus timestamp.
  --interference <cmd>
                      Optional background workload to run during the benchmark.

Outputs:
  CSV and JSON summary under engine/nnos/build/benchmarks.
EOF
}

require_command() {
    command -v "$1" >/dev/null 2>&1 || {
        printf 'error: required command not found: %s\n' "$1" >&2
        exit 1
    }
}

while [ "$#" -gt 0 ]; do
    case "$1" in
        --command)
            RUN_COMMAND="$2"
            shift 2
            ;;
        --fixture)
            FIXTURE_PATH="$2"
            shift 2
            ;;
        --env)
            ENV_NAME="$2"
            shift 2
            ;;
        --runs)
            RUNS="$2"
            shift 2
            ;;
        --label)
            LABEL="$2"
            shift 2
            ;;
        --interference)
            INTERFERENCE_COMMAND="$2"
            shift 2
            ;;
        -h|--help|help)
            usage
            exit 0
            ;;
        *)
            printf 'error: unknown argument: %s\n' "$1" >&2
            usage
            exit 1
            ;;
    esac
done

[ -n "${RUN_COMMAND}" ] || {
    usage
    exit 1
}

[ -f "${FIXTURE_PATH}" ] || {
    printf 'error: fixture not found: %s\n' "${FIXTURE_PATH}" >&2
    exit 1
}

require_command bash
require_command python3

mkdir -p "${OUTPUT_DIR}"

if [ -z "${LABEL}" ]; then
    LABEL="${ENV_NAME}-$(date '+%Y%m%d-%H%M%S')"
fi

CSV_PATH="${OUTPUT_DIR}/${LABEL}.csv"
JSON_PATH="${OUTPUT_DIR}/${LABEL}.json"
TMP_STDOUT="${OUTPUT_DIR}/${LABEL}.stdout.tmp"
INTERFERENCE_PID=""

cleanup() {
    if [ -n "${INTERFERENCE_PID}" ]; then
        kill "${INTERFERENCE_PID}" >/dev/null 2>&1 || true
        wait "${INTERFERENCE_PID}" 2>/dev/null || true
    fi
    rm -f "${TMP_STDOUT}"
}

trap cleanup EXIT

printf 'run,sha256,latency_ms,exit_code\n' > "${CSV_PATH}"

if [ -n "${INTERFERENCE_COMMAND}" ]; then
    bash -lc "${INTERFERENCE_COMMAND}" >/dev/null 2>&1 &
    INTERFERENCE_PID="$!"
fi

for run_id in $(seq 1 "${RUNS}"); do
    start_ns="$(python3 -c 'import time; print(time.perf_counter_ns())')"

    if bash -lc "${RUN_COMMAND}" < "${FIXTURE_PATH}" > "${TMP_STDOUT}"; then
        exit_code=0
    else
        exit_code=$?
    fi

    end_ns="$(python3 -c 'import time; print(time.perf_counter_ns())')"
    latency_ms="$(python3 -c "print((${end_ns} - ${start_ns}) / 1000000.0)")"
    digest="$(python3 -c "import hashlib, pathlib; print(hashlib.sha256(pathlib.Path('${TMP_STDOUT}').read_bytes()).hexdigest())")"

    printf '%s,%s,%s,%s\n' "${run_id}" "${digest}" "${latency_ms}" "${exit_code}" >> "${CSV_PATH}"

    if [ "${exit_code}" -ne 0 ]; then
        printf 'error: benchmark command failed on run %s\n' "${run_id}" >&2
        exit "${exit_code}"
    fi
done

python3 - <<PY
import csv
import json
from pathlib import Path
from statistics import mean, pstdev

csv_path = Path(${CSV_PATH@Q})
json_path = Path(${JSON_PATH@Q})
env_name = ${ENV_NAME@Q}
label = ${LABEL@Q}
fixture_path = ${FIXTURE_PATH@Q}
rows = list(csv.DictReader(csv_path.open()))
latencies = [float(row["latency_ms"]) for row in rows]
hashes = sorted({row["sha256"] for row in rows})

summary = {
    "label": label,
    "environment": env_name,
    "runs": len(rows),
    "fixture": fixture_path,
    "unique_hashes": len(hashes),
    "hashes": hashes,
    "latency_ms": {
        "mean": mean(latencies),
        "stddev": pstdev(latencies),
        "min": min(latencies),
        "max": max(latencies),
    },
}

json_path.write_text(json.dumps(summary, indent=2) + "\n")
print(json.dumps(summary, indent=2))
PY
