/*
 * agent_lifecycle.c -- AI Agent Lifecycle Management Recipe
 *
 * Validates that projects implement proper agent lifecycle patterns:
 *   - State machine: PROPOSED -> APPROVED -> INITIALIZED -> ACTIVE
 *                    -> SUSPENDED -> TERMINATED -> ARCHIVED
 *   - Required lifecycle hooks at each transition
 *   - Mandatory approval gates for high-risk transitions
 *   - Lifecycle event logging requirements
 *   - Kill switch presence for active agents
 *   - Proper cleanup on termination
 *   - Archive compliance before deletion
 *
 * Aligned with NIST AI RMF MAP/MEASURE/MANAGE functions and
 * EU AI Act Article 14 (human oversight requirements).
 */

#include "lst.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* --------------------------------------------------------------------------
 * Constants
 * -------------------------------------------------------------------------- */

/* Valid state transitions (from -> to).  Any other transition is illegal.
 * Used by check_agent_records to validate transition legality. */
static const uint8_t VALID_TRANSITIONS[][2] = {
    { AGENT_PROPOSED,    AGENT_APPROVED     },
    { AGENT_APPROVED,    AGENT_INITIALIZED  },
    { AGENT_INITIALIZED, AGENT_ACTIVE       },
    { AGENT_ACTIVE,      AGENT_SUSPENDED    },
    { AGENT_SUSPENDED,   AGENT_ACTIVE       },  /* resume */
    { AGENT_ACTIVE,      AGENT_TERMINATED   },
    { AGENT_SUSPENDED,   AGENT_TERMINATED   },
    { AGENT_TERMINATED,  AGENT_ARCHIVED     },
    /* Emergency transitions */
    { AGENT_INITIALIZED, AGENT_TERMINATED   },
};
#define TRANSITION_COUNT (sizeof(VALID_TRANSITIONS) / sizeof(VALID_TRANSITIONS[0]))

/* Patterns indicating lifecycle management in source */
static const char *LIFECYCLE_PATTERNS[] = {
    "agent_stage",    "AgentStage",    "agent_lifecycle",
    "AgentLifecycle", "lifecycle_state", "LifecycleState",
    "PROPOSED",       "APPROVED",       "INITIALIZED",
    "ACTIVE",         "SUSPENDED",      "TERMINATED",
    "ARCHIVED",       "lifecycle_hook", "on_transition",
    "state_machine",  "StateMachine",
    NULL
};

static const char *KILL_SWITCH_PATTERNS[] = {
    "kill_switch",    "KillSwitch",     "emergency_stop",
    "EmergencyStop",  "force_terminate", "abort_agent",
    "KILL_SWITCH",    "emergency_halt",
    NULL
};

static const char *APPROVAL_GATE_PATTERNS[] = {
    "approval_gate",  "ApprovalGate",   "human_approval",
    "HumanApproval",  "require_approval", "approval_required",
    "hitl_gate",      "HITLGate",       "manual_approval",
    NULL
};

static const char *AUDIT_TRAIL_PATTERNS[] = {
    "audit_trail",    "AuditTrail",     "audit_log",
    "AuditLog",       "trace_id",       "TraceId",
    "event_log",      "EventLog",       "lifecycle_event",
    "LifecycleEvent", "transition_log",
    NULL
};

/* --------------------------------------------------------------------------
 * Helpers
 * -------------------------------------------------------------------------- */

static char *read_file_alc(const char *path, size_t *out_len) {
    FILE *f = fopen(path, "rb");
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

static int contains_any(const char *content, const char **patterns) {
    for (int i = 0; patterns[i]; i++) {
        if (strstr(content, patterns[i])) return 1;
    }
    return 0;
}

static int count_matches(const char *content, const char **patterns) {
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
 * Checks
 * -------------------------------------------------------------------------- */

typedef struct {
    int critical;
    int error;
    int warning;
    int info;
} finding_counts_t;

static void check_lifecycle_state_machine(
    lst_artifact_t *art, finding_counts_t *counts
) {
    int has_lifecycle = 0;
    int has_all_stages = 0;
    int stage_found[AGENT_STAGE_COUNT];
    memset(stage_found, 0, sizeof(stage_found));

    const char *stage_names[] = {
        "PROPOSED", "APPROVED", "INITIALIZED", "ACTIVE",
        "SUSPENDED", "TERMINATED", "ARCHIVED"
    };

    for (uint32_t i = 0; i < art->file_count; i++) {
        size_t len = 0;
        char *content = read_file_alc(art->files[i].path, &len);
        if (!content) continue;

        if (contains_any(content, LIFECYCLE_PATTERNS)) {
            has_lifecycle = 1;
            for (int s = 0; s < AGENT_STAGE_COUNT; s++) {
                if (strstr(content, stage_names[s]))
                    stage_found[s] = 1;
            }
        }
        free(content);
    }

    if (!has_lifecycle) {
        printf("  [CRITICAL] No agent lifecycle state machine found\n");
        printf("    Agents MUST implement lifecycle stages: PROPOSED -> ARCHIVED\n");
        counts->critical++;

        if (art->issue_count < LST_MAX_ISSUES) {
            lst_issue_t *iss = &art->issues[art->issue_count++];
            snprintf(iss->id, sizeof(iss->id), "ALC-001");
            iss->severity = SEV_CRITICAL;
            snprintf(iss->title, sizeof(iss->title),
                     "Missing agent lifecycle state machine");
            snprintf(iss->description, sizeof(iss->description),
                     "No lifecycle management patterns detected in codebase");
            snprintf(iss->cwe, sizeof(iss->cwe), "NIST-MAP");
            snprintf(iss->remediation, sizeof(iss->remediation),
                     "Implement agent_stage_t state machine with all 7 stages");
        }
        return;
    }

    printf("  [INFO] Agent lifecycle state machine detected\n");
    counts->info++;

    /* Check for all required stages */
    has_all_stages = 1;
    for (int s = 0; s < AGENT_STAGE_COUNT; s++) {
        if (!stage_found[s]) {
            has_all_stages = 0;
            printf("  [ERROR] Missing lifecycle stage: %s\n", stage_names[s]);
            counts->error++;
        }
    }

    if (has_all_stages) {
        printf("  [INFO] All 7 lifecycle stages implemented\n");
        counts->info++;
    }
}

static void check_kill_switch(
    lst_artifact_t *art, finding_counts_t *counts
) {
    int has_kill_switch = 0;

    for (uint32_t i = 0; i < art->file_count; i++) {
        size_t len = 0;
        char *content = read_file_alc(art->files[i].path, &len);
        if (!content) continue;

        if (contains_any(content, KILL_SWITCH_PATTERNS)) {
            has_kill_switch = 1;
            printf("  [INFO] Kill switch found: %s\n", art->files[i].path);
            counts->info++;
        }
        free(content);
    }

    if (!has_kill_switch) {
        printf("  [CRITICAL] No kill switch / emergency stop mechanism found\n");
        printf("    EU AI Act Art.14: High-risk AI must have human override\n");
        counts->critical++;

        if (art->issue_count < LST_MAX_ISSUES) {
            lst_issue_t *iss = &art->issues[art->issue_count++];
            snprintf(iss->id, sizeof(iss->id), "ALC-002");
            iss->severity = SEV_CRITICAL;
            snprintf(iss->title, sizeof(iss->title),
                     "No kill switch for AI agents");
            snprintf(iss->description, sizeof(iss->description),
                     "Active agents must have emergency termination capability");
            snprintf(iss->cwe, sizeof(iss->cwe), "EU-AI-14");
            snprintf(iss->remediation, sizeof(iss->remediation),
                     "Implement kill_switch() with immediate effect");
        }
    }
}

static void check_approval_gates(
    lst_artifact_t *art, finding_counts_t *counts
) {
    int has_approval = 0;

    for (uint32_t i = 0; i < art->file_count; i++) {
        size_t len = 0;
        char *content = read_file_alc(art->files[i].path, &len);
        if (!content) continue;

        if (contains_any(content, APPROVAL_GATE_PATTERNS)) {
            has_approval = 1;
            int gate_count = count_matches(content, APPROVAL_GATE_PATTERNS);
            printf("  [INFO] Approval gates found (%d references): %s\n",
                   gate_count, art->files[i].path);
            counts->info++;
        }
        free(content);
    }

    if (!has_approval) {
        printf("  [ERROR] No approval gates for lifecycle transitions\n");
        printf("    High-risk transitions (PROPOSED->APPROVED, *->ACTIVE)\n");
        printf("    require human approval gates\n");
        counts->error++;

        if (art->issue_count < LST_MAX_ISSUES) {
            lst_issue_t *iss = &art->issues[art->issue_count++];
            snprintf(iss->id, sizeof(iss->id), "ALC-003");
            iss->severity = SEV_ERROR;
            snprintf(iss->title, sizeof(iss->title),
                     "Missing approval gates for agent transitions");
            snprintf(iss->description, sizeof(iss->description),
                     "Agent activation requires human approval mechanism");
            snprintf(iss->cwe, sizeof(iss->cwe), "NIST-GOV");
            snprintf(iss->remediation, sizeof(iss->remediation),
                     "Add approval_gate checks before APPROVED and ACTIVE transitions");
        }
    }
}

static void check_audit_trail(
    lst_artifact_t *art, finding_counts_t *counts
) {
    int has_audit = 0;

    for (uint32_t i = 0; i < art->file_count; i++) {
        size_t len = 0;
        char *content = read_file_alc(art->files[i].path, &len);
        if (!content) continue;

        if (contains_any(content, AUDIT_TRAIL_PATTERNS)) {
            has_audit = 1;
        }
        free(content);
    }

    if (!has_audit) {
        printf("  [ERROR] No lifecycle audit trail implementation found\n");
        printf("    All state transitions must be logged immutably\n");
        counts->error++;

        if (art->issue_count < LST_MAX_ISSUES) {
            lst_issue_t *iss = &art->issues[art->issue_count++];
            snprintf(iss->id, sizeof(iss->id), "ALC-004");
            iss->severity = SEV_ERROR;
            snprintf(iss->title, sizeof(iss->title),
                     "No lifecycle audit trail");
            snprintf(iss->description, sizeof(iss->description),
                     "Lifecycle transitions must have immutable audit logging");
            snprintf(iss->cwe, sizeof(iss->cwe), "NIST-GOV");
            snprintf(iss->remediation, sizeof(iss->remediation),
                     "Implement audit_trail with trace_id per transition event");
        }
    } else {
        printf("  [INFO] Lifecycle audit trail detected\n");
        counts->info++;
    }
}

static int is_valid_transition(uint8_t from, uint8_t to) {
    for (size_t i = 0; i < TRANSITION_COUNT; i++) {
        if (VALID_TRANSITIONS[i][0] == from && VALID_TRANSITIONS[i][1] == to)
            return 1;
    }
    return 0;
}

static void check_agent_records(
    lst_artifact_t *art, finding_counts_t *counts
) {
    for (uint32_t i = 0; i < art->agent_count; i++) {
        lst_agent_record_t *agent = &art->agents[i];

        /* Validate current stage is reachable via valid transitions */
        if (agent->current_stage > AGENT_PROPOSED) {
            int reachable = is_valid_transition(
                agent->current_stage - 1, agent->current_stage
            );
            if (!reachable && agent->current_stage != AGENT_TERMINATED) {
                printf("  [WARNING] Agent '%s': stage %d may not be reachable "
                       "via standard transitions\n",
                       agent->agent_name, agent->current_stage);
                counts->warning++;
            }
        }

        /* Validate risk tier matches HITL requirement */
        if (agent->risk_tier >= RISK_TIER_HIGH && agent->hitl_requirement < HITL_APPROVAL) {
            printf("  [CRITICAL] Agent '%s': risk_tier=%d but hitl=%d\n",
                   agent->agent_name, agent->risk_tier, agent->hitl_requirement);
            printf("    High/Critical risk agents MUST require HITL_APPROVAL or HITL_MANDATORY\n");
            counts->critical++;
        }

        /* Validate active agents have kill switch */
        if (agent->current_stage == AGENT_ACTIVE && !agent->has_kill_switch) {
            printf("  [CRITICAL] Active agent '%s' has no kill switch\n",
                   agent->agent_name);
            counts->critical++;
        }

        /* Validate active agents have audit trail */
        if (agent->current_stage == AGENT_ACTIVE && !agent->has_audit_trail) {
            printf("  [ERROR] Active agent '%s' has no audit trail\n",
                   agent->agent_name);
            counts->error++;
        }

        /* Validate owner assignment */
        if (agent->current_stage >= AGENT_APPROVED && agent->owner[0] == '\0') {
            printf("  [ERROR] Agent '%s' (stage %d) has no assigned owner\n",
                   agent->agent_name, agent->current_stage);
            counts->error++;
        }

        /* Validate scope bounds for active agents */
        if (agent->current_stage == AGENT_ACTIVE && !agent->has_scope_bounds) {
            printf("  [WARNING] Active agent '%s' has no scope bounds defined\n",
                   agent->agent_name);
            counts->warning++;
        }

        /* Validate aggregate risk is computed */
        if (agent->aggregate_risk < 0.0f || agent->aggregate_risk > 100.0f) {
            printf("  [ERROR] Agent '%s': invalid aggregate_risk=%.2f\n",
                   agent->agent_name, (double)agent->aggregate_risk);
            counts->error++;
        }
    }
}

/* --------------------------------------------------------------------------
 * Recipe entry point
 * -------------------------------------------------------------------------- */

static int recipe_agent_lifecycle(lst_artifact_t *art, const char *output_dir) {
    (void)output_dir;

    printf("\n");
    for (int i = 0; i < 70; i++) putchar('=');
    printf("\nAGENT LIFECYCLE MANAGEMENT REPORT\n");
    printf("Project: %s\n", art->project_name);
    for (int i = 0; i < 70; i++) putchar('=');
    printf("\n\n");

    finding_counts_t counts = {0, 0, 0, 0};

    printf("[1/5] Lifecycle State Machine\n");
    check_lifecycle_state_machine(art, &counts);

    printf("\n[2/5] Kill Switch / Emergency Stop\n");
    check_kill_switch(art, &counts);

    printf("\n[3/5] Approval Gates\n");
    check_approval_gates(art, &counts);

    printf("\n[4/5] Audit Trail\n");
    check_audit_trail(art, &counts);

    printf("\n[5/5] Agent Records Validation\n");
    if (art->agent_count == 0) {
        printf("  [INFO] No agent records in artifact (scan only)\n");
        counts.info++;
    } else {
        printf("  Validating %u agent records...\n", art->agent_count);
        check_agent_records(art, &counts);
    }

    /* Summary */
    printf("\n");
    for (int i = 0; i < 70; i++) putchar('-');
    printf("\nSUMMARY: Critical=%d Error=%d Warning=%d Info=%d\n",
           counts.critical, counts.error, counts.warning, counts.info);

    int pass = (counts.critical == 0);
    printf("RESULT: %s\n", pass ? "PASS" : "FAIL");
    for (int i = 0; i < 70; i++) putchar('=');
    printf("\n");

    return pass ? 0 : 1;
}

void recipe_agent_lifecycle_register(void) {
    lst_recipe_t r;
    memset(&r, 0, sizeof(r));
    snprintf(r.name, sizeof(r.name), "agent_lifecycle");
    snprintf(r.description, sizeof(r.description),
             "AI agent lifecycle management validation (NIST AI RMF)");
    r.execute = recipe_agent_lifecycle;
    r.version = 1;
    lst_recipe_register(&r);
}
