#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
DOCS_DIR="${ROOT_DIR}/docs"
BUILD_DIR="${ROOT_DIR}/build"
BOOTSTRAP_SCRIPT="${SCRIPT_DIR}/bootstrap_encoding.sh"

INSTALL_PREFIX="${INSTALL_PREFIX:-/usr/local}"
NNOS_CONFIG_DIR="${NNOS_CONFIG_DIR:-/etc/lsa}"
SYSTEM_LOG_ROOT="${NNOS_LOG_ROOT:-/var/log/lsa}"
PROJECT_LOG_ROOT="${ROOT_DIR}/build/logs"
SUPERVISOR_CONFIG_FILE="${NNOS_SUPERVISOR_CONF:-${BUILD_DIR}/supervisor/supervisord.conf}"
SERVICE_USER="${NNOS_SERVICE_USER:-lsa}"

REQUIRED_DOCS=(
    "NNOS-Requirements-Spec.md"
    "NNOS-Design-Spec.md"
    "NNOS-LSA-TECH-002.md"
)

ROLE=""
BOOT_BINARY=""
SERVICE_NAME=""
LOG_ROOT=""
LOG_FILE=""

usage() {
    cat <<'EOF'
Usage: propagate_nnos.sh [validate|normalize|build|install|start|all]

Actions:
  validate   Check platform, docs, role mapping, and environment mode.
  normalize  Normalize UTF-8 BOM and line endings under engine/nnos.
  build      Configure and build the NNOS tree if CMakeLists.txt exists.
  install    Install build artifacts and configure systemd, Docker, or Podman runtime.
  start      Start the configured runtime for the detected environment.
  all        Run validate, normalize, build, install, and start.

Environment:
  NODE_ROLE            Override role detection. Accepted aliases: dcn, hcn, epn,
                       nuc-dcn, m1-hcn, orin-epn.
  INSTALL_PREFIX       Install prefix, default /usr/local.
  NNOS_CONFIG_DIR      Runtime config directory, default /etc/lsa.
  NNOS_LOG_ROOT        Host log directory, default /var/log/lsa.
  NNOS_SUPERVISOR_CONF Docker supervisor config output path.
  NNOS_SERVICE_USER    Linux service user, default lsa.
  NNOS_BUILD_JOBS      Override build parallelism.
EOF
}

timestamp() {
    date '+%Y-%m-%d %H:%M:%S'
}

init_logging() {
    if [ "$(uname -s)" = "Linux" ] && mkdir -p "${SYSTEM_LOG_ROOT}" 2>/dev/null; then
        LOG_ROOT="${SYSTEM_LOG_ROOT}"
    else
        LOG_ROOT="${PROJECT_LOG_ROOT}"
        mkdir -p "${LOG_ROOT}"
    fi

    LOG_FILE="${LOG_ROOT}/propagate.log"
    touch "${LOG_FILE}"
}

log() {
    printf '%s [NNOS] %s\n' "$(timestamp)" "$1" | tee -a "${LOG_FILE}"
}

die() {
    log "ERROR: $1"
    exit 1
}

require_command() {
    command -v "$1" >/dev/null 2>&1 || die "required command not found: $1"
}

run_privileged() {
    if [ "$(id -u)" -eq 0 ]; then
        "$@"
        return
    fi

    require_command sudo
    sudo "$@"
}

cpu_count() {
    if command -v nproc >/dev/null 2>&1; then
        nproc
    elif command -v sysctl >/dev/null 2>&1; then
        sysctl -n hw.ncpu
    else
        printf '2\n'
    fi
}

build_jobs() {
    if [ -n "${NNOS_BUILD_JOBS:-}" ]; then
        printf '%s\n' "${NNOS_BUILD_JOBS}"
        return
    fi

    if [ -n "${MAKEFLAGS:-}" ] && printf '%s' "${MAKEFLAGS}" | grep -Eq -- '-j[0-9]+'; then
        printf '%s\n' "${MAKEFLAGS}" | sed -E 's/.*-j([0-9]+).*/\1/'
        return
    fi

    cpu_count
}

is_linux() {
    [ "$(uname -s)" = "Linux" ]
}

is_docker() {
    [ -f "/.dockerenv" ]
}

is_podman() {
    [ "${container:-}" = "podman" ] || [ -f "/run/.containerenv" ]
}

is_podman_rootless() {
    is_podman && [ "$(id -u)" -ne 0 ]
}

is_vm() {
    if ! is_linux; then
        return 1
    fi

    if command -v systemd-detect-virt >/dev/null 2>&1 && systemd-detect-virt -q; then
        return 0
    fi

    grep -qi hypervisor /proc/cpuinfo 2>/dev/null
}

normalize_role() {
    case "$1" in
        dcn|nuc|nuc-dcn)
            printf 'dcn\n'
            ;;
        hcn|m1|m1-hcn)
            printf 'hcn\n'
            ;;
        epn|orin|orin-epn)
            printf 'epn\n'
            ;;
        *)
            return 1
            ;;
    esac
}

detect_role() {
    local raw

    if [ -n "${NODE_ROLE:-}" ]; then
        raw="${NODE_ROLE}"
    else
        raw="$(hostname)"
    fi

    raw="$(printf '%s' "${raw}" | tr '[:upper:]' '[:lower:]')"

    if normalize_role "${raw}" >/dev/null 2>&1; then
        normalize_role "${raw}"
        return
    fi

    case "${raw}" in
        *nuc*|*dcn*)
            printf 'dcn\n'
            ;;
        *m1*|*hcn*)
            printf 'hcn\n'
            ;;
        *orin*|*epn*)
            printf 'epn\n'
            ;;
        *)
            printf 'dcn\n'
            ;;
    esac
}

boot_binary_for_role() {
    case "$1" in
        dcn)
            printf 'lsa_boot_dcn\n'
            ;;
        hcn)
            printf 'lsa_boot_hcn\n'
            ;;
        epn)
            printf 'lsa_boot_epn\n'
            ;;
        *)
            return 1
            ;;
    esac
}

service_name_for_role() {
    printf 'lsa-boot-%s.service\n' "$1"
}

find_boot_binary() {
    local candidate

    if [ -x "${INSTALL_PREFIX}/bin/${BOOT_BINARY}" ]; then
        printf '%s\n' "${INSTALL_PREFIX}/bin/${BOOT_BINARY}"
        return
    fi

    candidate="$(find "${BUILD_DIR}" -type f -perm -111 -name "${BOOT_BINARY}" 2>/dev/null | head -n 1)"
    if [ -n "${candidate}" ]; then
        printf '%s\n' "${candidate}"
        return
    fi

    die "boot binary ${BOOT_BINARY} was not found in ${INSTALL_PREFIX}/bin or ${BUILD_DIR}"
}

validate_env() {
    local arch
    local os_name
    local doc

    log "Validating environment..."

    os_name="$(uname -s)"
    arch="$(uname -m)"

    case "${os_name}" in
        Linux)
            case "${arch}" in
                x86_64|aarch64)
                    ;;
                *)
                    die "unsupported Linux architecture: ${arch}"
                    ;;
            esac
            ;;
        Darwin)
            case "${arch}" in
                x86_64|arm64)
                    ;;
                *)
                    die "unsupported Darwin architecture: ${arch}"
                    ;;
            esac
            ;;
        *)
            die "unsupported operating system: ${os_name}"
            ;;
    esac

    for doc in "${REQUIRED_DOCS[@]}"; do
        [ -f "${DOCS_DIR}/${doc}" ] || die "missing required doc: ${DOCS_DIR}/${doc}"
    done

    ROLE="$(detect_role)"
    BOOT_BINARY="$(boot_binary_for_role "${ROLE}")"
    SERVICE_NAME="$(service_name_for_role "${ROLE}")"

    if is_podman_rootless; then
        export MAKEFLAGS="-j2"
        log "Podman rootless detected. Using foreground container mode with MAKEFLAGS=${MAKEFLAGS}."
    elif is_docker; then
        export MAKEFLAGS="-j2"
        log "Docker detected. Using supervisord container mode with MAKEFLAGS=${MAKEFLAGS}."
    elif is_vm; then
        export MAKEFLAGS="-j2"
        log "Virtual machine detected. Capping build parallelism with MAKEFLAGS=${MAKEFLAGS}."
    else
        log "Native host detected."
    fi

    log "Resolved role=${ROLE} boot_binary=${BOOT_BINARY} service=${SERVICE_NAME}"

    if [ ! -f "${ROOT_DIR}/CMakeLists.txt" ]; then
        log "CMakeLists.txt is not present in ${ROOT_DIR}. The specs describe a future CMake build, so build/install will fail until that tree exists."
    fi
}

normalize_encodings() {
    log "Normalizing encodings..."
    [ -f "${BOOTSTRAP_SCRIPT}" ] || die "bootstrap script not found: ${BOOTSTRAP_SCRIPT}"
    bash "${BOOTSTRAP_SCRIPT}" "${ROOT_DIR}" | tee -a "${LOG_FILE}"
}

build_daemons() {
    local jobs

    log "Building NNOS artifacts..."
    require_command cmake

    [ -f "${ROOT_DIR}/CMakeLists.txt" ] || die "cannot build: ${ROOT_DIR}/CMakeLists.txt is missing"

    jobs="$(build_jobs)"
    mkdir -p "${BUILD_DIR}"

    cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="${INSTALL_PREFIX}" \
        -DCMAKE_CXX_FLAGS="-O3 -fno-exceptions -fno-rtti"

    cmake --build "${BUILD_DIR}" --parallel "${jobs}"
}

install_artifacts() {
    log "Installing NNOS artifacts..."
    [ -f "${BUILD_DIR}/CMakeCache.txt" ] || die "build directory is missing CMakeCache.txt"

    if [ "$(id -u)" -eq 0 ] || [ -w "${INSTALL_PREFIX}" ] || [ -w "$(dirname "${INSTALL_PREFIX}")" ]; then
        cmake --install "${BUILD_DIR}"
    else
        run_privileged cmake --install "${BUILD_DIR}"
    fi
}

ensure_service_user() {
    if ! is_linux || is_docker || is_podman; then
        return
    fi

    if id -u "${SERVICE_USER}" >/dev/null 2>&1; then
        return
    fi

    log "Creating system service user ${SERVICE_USER}..."
    run_privileged groupadd --system "${SERVICE_USER}" 2>/dev/null || true
    run_privileged useradd --system --gid "${SERVICE_USER}" \
        --home /var/lib/"${SERVICE_USER}" --create-home \
        --shell /usr/sbin/nologin "${SERVICE_USER}" 2>/dev/null || true
}

ensure_runtime_dirs() {
    if is_linux && ! is_docker && ! is_podman; then
        run_privileged mkdir -p "${NNOS_CONFIG_DIR}" "${SYSTEM_LOG_ROOT}"
        if id -u "${SERVICE_USER}" >/dev/null 2>&1; then
            run_privileged chown -R "${SERVICE_USER}:${SERVICE_USER}" "${NNOS_CONFIG_DIR}" "${SYSTEM_LOG_ROOT}"
        fi
    else
        mkdir -p "${BUILD_DIR}/runtime" "${LOG_ROOT}"
    fi
}

write_systemd_unit() {
    local exec_path
    local tmp_unit
    local unit_path

    is_linux || die "systemd installation is only supported on Linux"
    require_command systemctl

    exec_path="$(find_boot_binary)"
    unit_path="/etc/systemd/system/${SERVICE_NAME}"
    tmp_unit="$(mktemp "${TMPDIR:-/tmp}/nnos-systemd.XXXXXX")"

    cat > "${tmp_unit}" <<EOF
[Unit]
Description=NNOS boot daemon (${ROLE})
After=network.target

[Service]
Type=simple
ExecStart=${exec_path}
Restart=always
RestartSec=2
User=${SERVICE_USER}
Group=${SERVICE_USER}
Environment=NODE_ROLE=${ROLE}
Environment=NNOS_CONFIG_DIR=${NNOS_CONFIG_DIR}
Environment=NNOS_LOG_ROOT=${SYSTEM_LOG_ROOT}

[Install]
WantedBy=multi-user.target
EOF

    run_privileged install -m 0644 "${tmp_unit}" "${unit_path}"
    rm -f "${tmp_unit}"
    log "Wrote ${unit_path}"
}

write_supervisor_config() {
    local exec_path
    local supervisor_dir

    require_command supervisord
    exec_path="$(find_boot_binary)"
    supervisor_dir="$(dirname "${SUPERVISOR_CONFIG_FILE}")"
    mkdir -p "${supervisor_dir}"

    cat > "${SUPERVISOR_CONFIG_FILE}" <<EOF
[supervisord]
nodaemon=true
logfile=${LOG_ROOT}/supervisord.log
pidfile=/tmp/nnos-supervisord.pid

[program:nnos-boot]
command=${exec_path}
autorestart=true
stdout_logfile=${LOG_ROOT}/${BOOT_BINARY}.stdout.log
stderr_logfile=${LOG_ROOT}/${BOOT_BINARY}.stderr.log
environment=NODE_ROLE="${ROLE}",NNOS_CONFIG_DIR="${NNOS_CONFIG_DIR}",NNOS_LOG_ROOT="${LOG_ROOT}"
EOF

    log "Wrote ${SUPERVISOR_CONFIG_FILE}"
}

install_configure() {
    log "Configuring runtime integration..."
    install_artifacts
    ensure_runtime_dirs

    if is_podman_rootless; then
        log "Podman rootless detected. Skipping systemd and supervisord. Use foreground execution or a user-level quadlet unit."
        return
    fi

    if is_docker; then
        write_supervisor_config
        return
    fi

    ensure_service_user
    write_systemd_unit
}

start_system() {
    local exec_path

    exec_path="$(find_boot_binary)"

    if is_podman_rootless; then
        log "Starting ${BOOT_BINARY} in Podman rootless foreground mode..."
        exec "${exec_path}"
    fi

    if is_docker; then
        require_command supervisord
        [ -f "${SUPERVISOR_CONFIG_FILE}" ] || die "supervisor config not found: ${SUPERVISOR_CONFIG_FILE}"
        log "Starting supervisord with ${SUPERVISOR_CONFIG_FILE}..."
        exec "$(command -v supervisord)" -c "${SUPERVISOR_CONFIG_FILE}"
    fi

    is_linux || die "automatic service start is only supported on Linux or containers"
    require_command systemctl
    run_privileged systemctl daemon-reload
    run_privileged systemctl enable "${SERVICE_NAME}"
    run_privileged systemctl start "${SERVICE_NAME}"
    log "Started ${SERVICE_NAME}"
}

main() {
    local action="${1:-all}"

    init_logging

    case "${action}" in
        validate)
            validate_env
            ;;
        normalize)
            validate_env
            normalize_encodings
            ;;
        build)
            validate_env
            build_daemons
            ;;
        install)
            validate_env
            install_configure
            ;;
        start)
            validate_env
            start_system
            ;;
        all)
            validate_env
            normalize_encodings
            build_daemons
            install_configure
            start_system
            ;;
        -h|--help|help)
            usage
            ;;
        *)
            usage
            exit 1
            ;;
    esac
}

main "$@"
