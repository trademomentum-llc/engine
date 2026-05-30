CC       = cc
CFLAGS   = -Wall -Wextra -Wpedantic -O2 -std=c11 -I include
LDFLAGS  =

SRC      = src/main.c src/lst.c src/store.c src/runner.c src/seal.c
RECIPES  = recipes/license_attribution.c recipes/security_scan.c recipes/code_check.c \
           recipes/pqc_validation.c recipes/lsa_compliance.c recipes/neurodiv_safety.c \
           recipes/morphogenetic_healing.c recipes/agentxfoundry_threat_intel.c \
           recipes/tmg_compliance.c recipes/agent_lifecycle.c recipes/risk_assessment.c \
           recipes/traceability_audit.c recipes/compliance_guardrails.c \
           recipes/vectordb_provenance.c recipes/shared_state_schema.c \
           recipes/drift_detection.c recipes/jasterish_validation.c \
           recipes/bootstrap_registry.c recipes/daemon_constellation.c \
           recipes/deterministic_benchmark.c

TARGET   = engine

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRC) $(RECIPES)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(TARGET)
