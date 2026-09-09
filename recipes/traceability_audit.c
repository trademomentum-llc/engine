/*
 * traceability_audit.c -- AI Agent Traceability Audit Recipe
 *
 * Validates end-to-end traceability infrastructure:
 *   - Trace ID generation on every agent action
 *   - Decision chain logging with reasoning
 *   - Parent-child relationships for delegated tasks
 *   - Immutable audit log with tamper detection
 *   - Provenance metadata on all artifacts
 *   - Guardrail trigger logging
 *
 * Aligned with NIST AI RMF GOVERN/MAP functions and
 * ISO 42001 Annex B (AI traceability requirements).
 */

#include "lst.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* --------------------------------------------------------------------------
 * Traceability pattern sets
 * -------------------------------------------------------------------------- */

/* Trace ID generation */
static const char *TRACE_ID_PATTERNS[] = {
    "trace_id",       "TraceId",       "traceId",
    "trace_context",  "TraceContext",   "traceContext",
    "correlation_id", "CorrelationId", "correlationId",
    "request_id",     "RequestId",     "requestId",
    "span_id",        "SpanId",        "spanId",
    NULL
};

/* Decision logging */
static const char *DECISION_LOG_PATTERNS[] = {
    "decision_log",    "DecisionLog",    "decisionLog",
    "decision_chain",  "DecisionChain",  "decisionChain",
    "reasoning",       "rationale",      "justification",
    "decision_record", "DecisionRecord",
    "log_decision",    "logDecision",
    NULL
};

/* Delegation tracking */
static const char *DELEGATION_TRACK_PATTERNS[] = {
    "parent_trace",    "parentTrace",    "parent_id",
    "child_trace",     "childTrace",     "child_id",
    "delegation_chain", "DelegationChain",
    "spawn_with_trace", "propagate_trace",
    "parent_agent",    "child_agent",    "delegated_to",
    NULL
};

/* Immutable logging */
static const char *IMMUTABLE_LOG_PATTERNS[] = {
    "append_only",     "AppendOnly",     "appendOnly",
    "immutable_log",   "ImmutableLog",   "immutableLog",
    "write_ahead_log", "WAL",            "wal_log",
    "tamper_detect",   "TamperDetect",   "tamperDetect",
    "hash_chain",      "HashChain",      "hashChain",
    "merkle",          "checksum_chain",
    NULL
};

/* Provenance metadata */
static const char *PROVENANCE_PATTERNS[] = {
    "provenance",      "Provenance",
    "lineage",         "Lineage",
    "origin",          "data_source",    "DataSource",
    "created_by",      "modified_by",    "generated_by",
    "artifact_hash",   "content_hash",
    NULL
};

/* Guardrail logging */
static const char *GUARDRAIL_LOG_PATTERNS[] = {
    "guardrail_trigger", "GuardrailTrigger",
    "guardrail_log",     "GuardrailLog",
    "policy_violation",  "PolicyViolation",
    "constraint_breach", "ConstraintBreach",
    "blocked_action",    "BlockedAction",
    "rate_limit_hit",    "RateLimitHit",
    NULL
};

/* --------------------------------------------------------------------------
 * Helpers
 * -------------------------------------------------------------------------- */

static char *read_file_trace(const char *path, size_t *out_len) {
    FILE *f = lst_secure_fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    if (sz < 0 || sz > 50 * 1024 * 1024) { fclose(f); return NULL; }
    fseek(f, 0, SEEK_SET);
    char *buf = malloc((size_t)sz + 1);
    if (!buf) { fclose(f); return NULL; }
    size_t rd = fread(buf, 1, (size_t)sz, f);
    fclose(f);
    buf[rd] = '\0';
    if (out_len) *out_len = rd;
    return buf;
}

static int contains_any_trace(const char *content, const char **patterns) {
    for (int i = 0; patterns[i]; i++) {
        if (strstr(content, patterns[i])) return 1;
    }
    return 0;
}

static int count_trace_matches(const char *content, const char **patterns) {
    int count = 0;
    for (int i = 0; patterns[i]; i++) {
        const char *p = content;
        while ((p = strstr(p, patterns[i])) != NULL) {
            count++;
            p += strlen(patterns[i]);
        }
    }
    return count;
}

/* --------------------------------------------------------------------------
 * Traceability dimension checks
 * -------------------------------------------------------------------------- */

typedef struct {
    int has_trace_ids;
    int has_decision_logs;
    int has_delegation_tracking;
    int has_immutable_logs;
    int has_provenance;
    int has_guardrail_logs;
    int trace_id_count;
    int decision_log_count;
    int delegation_count;
    int immutable_count;
    int provenance_count;
    int guardrail_log_count;
} trace_scan_result_t;

static void scan_traceability(lst_artifact_t *art, trace_scan_result_t *result) {
    memset(result, 0, sizeof(*result));

    for (uint32_t i = 0; i < art->file_count; i++) {
        size_t len = 0;
        char *content = read_file_trace(art->files[i].path, &len);
        if (!content) continue;

        if (contains_any_trace(content, TRACE_ID_PATTERNS)) {
            result->has_trace_ids = 1;
            result->trace_id_count += count_trace_matches(content, TRACE_ID_PATTERNS);
        }

        if (contains_any_trace(content, DECISION_LOG_PATTERNS)) {
            result->has_decision_logs = 1;
            result->decision_log_count += count_trace_matches(content, DECISION_LOG_PATTERNS);
        }

        if (contains_any_trace(content, DELEGATION_TRACK_PATTERNS)) {
            result->has_delegation_tracking = 1;
            result->delegation_count += count_trace_matches(content, DELEGATION_TRACK_PATTERNS);
        }

        if (contains_any_trace(content, IMMUTABLE_LOG_PATTERNS)) {
            result->has_immutable_logs = 1;
            result->immutable_count += count_trace_matches(content, IMMUTABLE_LOG_PATTERNS);
        }

        if (contains_any_trace(content, PROVENANCE_PATTERNS)) {
            result->has_provenance = 1;
            result->provenance_count += count_trace_matches(content, PROVENANCE_PATTERNS);
        }

        if (contains_any_trace(content, GUARDRAIL_LOG_PATTERNS)) {
            result->has_guardrail_logs = 1;
            result->guardrail_log_count += count_trace_matches(content, GUARDRAIL_LOG_PATTERNS);
        }

        free(content);
    }
}

/* --------------------------------------------------------------------------
 * Trace record validation
 * -------------------------------------------------------------------------- */

static void validate_trace_records(
    lst_artifact_t *art, int *error_count, int *warning_count
) {
    int orphaned_traces = 0;
    int missing_reasoning = 0;
    int missing_outcome = 0;

    for (uint32_t i = 0; i < art->trace_count; i++) {
        lst_trace_record_t *tr = &art->traces[i];

        /* Check trace ID is populated */
        if (tr->trace_id[0] == '\0') {
            printf("  [ERROR] Trace record %u has empty trace_id\n", i);
            (*error_count)++;
            continue;
        }

        /* Check parent chain for delegation events */
        if (tr->event_type == TRACE_DELEGATION && tr->parent_trace_id[0] == '\0') {
            orphaned_traces++;
        }

        /* Decision events must have reasoning */
        if (tr->event_type == TRACE_DECISION && tr->reasoning[0] == '\0') {
            missing_reasoning++;
        }

        /* All events should have outcomes */
        if (tr->outcome[0] == '\0') {
            missing_outcome++;
        }

        /* Guardrail events must be logged */
        if (tr->guardrail_triggered && tr->event_type != TRACE_GUARDRAIL) {
            printf("  [WARNING] Trace %s: guardrail triggered but event_type=%d\n",
                   tr->trace_id, tr->event_type);
            (*warning_count)++;
        }
    }

    if (orphaned_traces > 0) {
        printf("  [ERROR] %d delegation traces without parent_trace_id\n",
               orphaned_traces);
        (*error_count)++;
    }

    if (missing_reasoning > 0) {
        printf("  [WARNING] %d decision traces without reasoning recorded\n",
               missing_reasoning);
        (*warning_count)++;
    }

    if (missing_outcome > 0) {
        printf("  [WARNING] %d traces without outcome recorded\n",
               missing_outcome);
        (*warning_count)++;
    }
}

/* --------------------------------------------------------------------------
 * Traceability completeness scoring
 * -------------------------------------------------------------------------- */

static float compute_traceability_score(trace_scan_result_t *result) {
    float score = 0.0f;
    float max_score = 6.0f;

    if (result->has_trace_ids)            score += 1.0f;
    if (result->has_decision_logs)        score += 1.0f;
    if (result->has_delegation_tracking)  score += 1.0f;
    if (result->has_immutable_logs)       score += 1.0f;
    if (result->has_provenance)           score += 1.0f;
    if (result->has_guardrail_logs)       score += 1.0f;

    return (score / max_score) * 100.0f;
}

/* --------------------------------------------------------------------------
 * Recipe entry point
 * -------------------------------------------------------------------------- */

static int recipe_traceability_audit(lst_artifact_t *art, const char *output_dir) {
    (void)output_dir;

    printf("\n");
    for (int i = 0; i < 70; i++) putchar('=');
    printf("\nTRACEABILITY AUDIT REPORT\n");
    printf("Project: %s\n", art->project_name);
    printf("Aligned: NIST AI RMF GOVERN, ISO 42001 Annex B\n");
    for (int i = 0; i < 70; i++) putchar('=');
    printf("\n\n");

    int critical = 0, error = 0, warning = 0, info = 0;

    /* Scan codebase for traceability patterns */
    trace_scan_result_t scan;
    scan_traceability(art, &scan);

    /* Dimension 1: Trace IDs */
    printf("[1/6] Trace ID Generation\n");
    if (scan.has_trace_ids) {
        printf("  [INFO] Trace ID patterns found (%d references)\n",
               scan.trace_id_count);
        info++;
    } else {
        printf("  [CRITICAL] No trace ID generation found\n");
        printf("    Every agent action must generate a unique trace_id\n");
        critical++;

        if (art->issue_count < LST_MAX_ISSUES) {
            lst_issue_t *iss = &art->issues[art->issue_count++];
            snprintf(iss->id, sizeof(iss->id), "TRC-001");
            iss->severity = SEV_CRITICAL;
            snprintf(iss->title, sizeof(iss->title),
                     "No trace ID generation");
            snprintf(iss->description, sizeof(iss->description),
                     "Agent actions must produce traceable correlation IDs");
            snprintf(iss->cwe, sizeof(iss->cwe), "ISO42001");
            snprintf(iss->remediation, sizeof(iss->remediation),
                     "Implement trace_id generation with TraceContext propagation");
        }
    }

    /* Dimension 2: Decision logging */
    printf("\n[2/6] Decision Chain Logging\n");
    if (scan.has_decision_logs) {
        printf("  [INFO] Decision logging found (%d references)\n",
               scan.decision_log_count);
        info++;
    } else {
        printf("  [ERROR] No decision chain logging found\n");
        printf("    Autonomous decisions must record reasoning and rationale\n");
        error++;
    }

    /* Dimension 3: Delegation tracking */
    printf("\n[3/6] Delegation Tracking\n");
    if (scan.has_delegation_tracking) {
        printf("  [INFO] Delegation tracking found (%d references)\n",
               scan.delegation_count);
        info++;
    } else {
        printf("  [WARNING] No delegation tracking found\n");
        printf("    Sub-agent spawning should propagate parent trace context\n");
        warning++;
    }

    /* Dimension 4: Immutable logs */
    printf("\n[4/6] Immutable Audit Logs\n");
    if (scan.has_immutable_logs) {
        printf("  [INFO] Immutable logging found (%d references)\n",
               scan.immutable_count);
        info++;
    } else {
        printf("  [ERROR] No immutable/append-only log infrastructure found\n");
        printf("    Audit logs must be tamper-resistant (hash chains, WAL)\n");
        error++;
    }

    /* Dimension 5: Provenance */
    printf("\n[5/6] Provenance Metadata\n");
    if (scan.has_provenance) {
        printf("  [INFO] Provenance tracking found (%d references)\n",
               scan.provenance_count);
        info++;
    } else {
        printf("  [WARNING] No provenance metadata found\n");
        printf("    Artifacts should track origin, creator, and lineage\n");
        warning++;
    }

    /* Dimension 6: Guardrail logging */
    printf("\n[6/6] Guardrail Trigger Logging\n");
    if (scan.has_guardrail_logs) {
        printf("  [INFO] Guardrail logging found (%d references)\n",
               scan.guardrail_log_count);
        info++;
    } else {
        printf("  [WARNING] No guardrail trigger logging found\n");
        printf("    Policy violations and constraint breaches should be logged\n");
        warning++;
    }

    /* Validate trace records if present */
    if (art->trace_count > 0) {
        printf("\nTrace Record Validation (%u records)\n", art->trace_count);
        validate_trace_records(art, &error, &warning);
    }

    /* Traceability score */
    float trace_score = compute_traceability_score(&scan);

    /* Summary */
    printf("\n");
    for (int i = 0; i < 70; i++) putchar('-');
    printf("\nTRACEABILITY SCORE: %.0f%% (6 dimensions)\n", (double)trace_score);
    printf("SUMMARY: Critical=%d Error=%d Warning=%d Info=%d\n",
           critical, error, warning, info);

    int pass = (critical == 0);
    printf("RESULT: %s\n", pass ? "PASS" : "FAIL");
    for (int i = 0; i < 70; i++) putchar('=');
    printf("\n");

    return pass ? 0 : 1;
}

void recipe_traceability_audit_register(void) {
    lst_recipe_t r;
    memset(&r, 0, sizeof(r));
    snprintf(r.name, sizeof(r.name), "traceability_audit");
    snprintf(r.description, sizeof(r.description),
             "End-to-end traceability validation (ISO 42001)");
    r.execute = recipe_traceability_audit;
    r.version = 1;
    lst_recipe_register(&r);
}
