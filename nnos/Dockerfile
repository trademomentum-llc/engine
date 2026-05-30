# NNOS Daemon Constellation — Multi-stage build
# Phase 6, Item 1 of Binary Optimization Plan

FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y \
    build-essential cmake git libssl-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build
COPY . .
RUN cmake -B build -S . -DCMAKE_BUILD_TYPE=MinSizeRel -DNNOS_ENABLE_LTO=ON \
    && cmake --build build -j$(nproc)

# Runtime stage — minimal footprint
FROM ubuntu:22.04 AS runtime

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/* \
    && mkdir -p /var/log/lsa

ENV NNOS_LOG_DIR=/var/log/lsa

COPY --from=builder /build/build/lsa_boot_dcn /usr/bin/
COPY --from=builder /build/build/lsa_boot_hcn /usr/bin/
COPY --from=builder /build/build/lsa_boot_epn /usr/bin/
COPY --from=builder /build/build/lsa_task_manager /usr/bin/
COPY --from=builder /build/build/lsa_context_gate /usr/bin/
COPY --from=builder /build/build/lsa_state_monitor /usr/bin/
COPY --from=builder /build/build/lsa_comm_bridge /usr/bin/
COPY --from=builder /build/build/lsa_profile_refiner /usr/bin/
COPY --from=builder /build/build/lsa_drift_detector /usr/bin/
COPY --from=builder /build/build/lsa_convergence_bond /usr/bin/
COPY --from=builder /build/build/lsa_ethernet_sync /usr/bin/
COPY --from=builder /build/build/SystemIntegrityDaemon /usr/bin/
COPY --from=builder /build/build/ThreatIntelligenceManager /usr/bin/
COPY --from=builder /build/build/MorphogeneticMaintainer /usr/bin/

# Default: run DCN boot daemon (master supervision)
# Override per node via deployment config
CMD ["/usr/bin/lsa_boot_dcn"]
