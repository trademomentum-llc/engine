/*
 * compliance_guardrails.c -- AI Agent Compliance Guardrails Recipe
 *
 * Validates compliance guardrail infrastructure:
 *   - Human-in-the-loop gates for critical operations
 *   - Rate limiting on resource-intensive operations
 *   - Data handling rules (PII detection, encryption)
 *   - Scope constraint enforcement (agent boundaries)
 *   - Kill switch / emergency termination
 *   - Budget and resource caps
 *   - Regulatory alignment (GDPR, HIPAA, SOC2, NIST AI RMF, EU AI Act)
 *   - Guardrail bypass detection (anti-patterns)
 */

#include "lst.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* --------------------------------------------------------------------------
 * Guardrail pattern categories
 * -------------------------------------------------------------------------- */

/* HITL gates */
static const char *HITL_PATTERNS[] = {
    "human_in_the_loop", "HumanInTheLoop",  "hitl_gate",
    "HITLGate",          "require_human",    "RequireHuman",
    "await_approval",    "AwaitApproval",    "human_review",
    "HumanReview",       "confirmation_required", "ConfirmationRequired",
    "manual_approval",   "ManualApproval",
    NULL
};

/* Rate limiting */
static const char *RATE_LIMIT_PATTERNS[] = {
    "rate_limit",     "RateLimit",     "rateLimit",
    "throttle",       "Throttle",      "token_bucket",
    "TokenBucket",    "sliding_window", "SlidingWindow",
    "requests_per",   "max_requests",  "cooldown",
    "backoff",        "retry_after",
    NULL
};

/* Data handling */
static const char *DATA_HANDLING_PATTERNS[] = {
    "encrypt(",       "decrypt(",      "hash(",
    "pii_filter",     "PIIFilter",     "redact(",
    "anonymize(",     "pseudonymize(", "mask_data",
    "data_classification", "DataClassification",
    "sensitive_data", "SensitiveData", "phi_protected",
    NULL
};

/* Scope constraints */
static const char *SCOPE_PATTERNS[] = {
    "scope_bound",    "ScopeBound",    "scopeBound",
    "allowed_actions", "AllowedActions", "action_whitelist",
    "permission_boundary", "PermissionBoundary",
    "capability_set",  "CapabilitySet", "allowed_tools",
    "AllowedTools",   "tool_filter",   "ToolFilter",
    "max_depth",      "max_iterations", "resource_limit",
    NULL
};

/* Kill switch / emergency controls */
static const char *EMERGENCY_PATTERNS[] = {
    "kill_switch",    "KillSwitch",    "emergency_stop",
    "EmergencyStop",  "circuit_breaker", "CircuitBreaker",
    "dead_man_switch", "DeadManSwitch", "watchdog",
    "Watchdog",       "heartbeat_timeout", "force_terminate",
    NULL
};

/* Budget/resource caps */
static const char *BUDGET_PATTERNS[] = {
    "budget_cap",     "BudgetCap",     "cost_limit",
    "CostLimit",      "max_tokens",    "MaxTokens",
    "max_cost",       "MaxCost",       "spending_limit",
    "resource_quota",  "ResourceQuota", "usage_limit",
    NULL
};

/* Anti-patterns: guardrail bypasses */
static const char *BYPASS_ANTIPATTERNS[] = {
    "skip_validation",  "SKIP_VALIDATION",  "bypass_guardrail",
    "BYPASS_GUARDRAIL", "disable_safety",   "DISABLE_SAFETY",
    "no_limit",         "NO_LIMIT",         "skip_approval",
    "SKIP_APPROVAL",    "force_execute",    "FORCE_EXECUTE",
    "override_policy",  "OVERRIDE_POLICY",  "unsafe_mode",
    "UNSAFE_MODE",      "skip_check",       "SKIP_CHECK",
    NULL
};

/* Regulatory-specific patterns */
static const char *GDPR_PATTERNS[] = {
    "right_to_erasure", "data_portability", "consent_management",
    "data_retention",   "privacy_impact",   "dpia",
    "data_subject_request", "processing_basis",
    NULL
};

static const char *HIPAA_PATTERNS[] = {
    "phi_protected",    "hipaa_compliant",  "baa_required",
    "minimum_necessary", "access_control_list", "audit_control",
    "encryption_at_rest", "encryption_in_transit",
    NULL
};

static const char *NIST_AI_PATTERNS[] = {
    "ai_risk_management", "risk_assessment", "bias_detection",
    "fairness_metric",    "explainability",  "transparency",
    "accountability",     "model_card",      "data_card",
    NULL
};

/* --------------------------------------------------------------------------
 * Helpers
 * -------------------------------------------------------------------------- */

static char *read_file_cg(const char *path, size_t *out_len) {
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

static int count_cg_matches(const char *content, const char **patterns) {
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

static int has_any_cg(const char *content, const char **patterns) {
    for (int i = 0; patterns[i]; i++) {
        if (strstr(content, patterns[i])) return 1;
    }
    return 0;
}

/* --------------------------------------------------------------------------
 * Guardrail assessment
 * -------------------------------------------------------------------------- */

typedef struct {
    int hitl_count;
    int rate_limit_count;
    int data_handling_count;
    int scope_count;
    int emergency_count;
    int budget_count;
    int bypass_count;
    int gdpr_count;
    int hipaa_count;
    int nist_ai_count;
    int has_hitl;
    int has_rate_limit;
    int has_data_handling;
    int has_scope;
    int has_emergency;
    int has_budget;
    int has_bypass;
} guardrail_scan_t;

static void scan_guardrails(lst_artifact_t *art, guardrail_scan_t *gs) {
    memset(gs, 0, sizeof(*gs));

    for (uint32_t i = 0; i < art->file_count; i++) {
        size_t len = 0;
        char *content = read_file_cg(art->files[i].path, &len);
        if (!content) continue;

        if (has_any_cg(content, HITL_PATTERNS)) {
            gs->has_hitl = 1;
            gs->hitl_count += count_cg_matches(content, HITL_PATTERNS);
        }
        if (has_any_cg(content, RATE_LIMIT_PATTERNS)) {
            gs->has_rate_limit = 1;
            gs->rate_limit_count += count_cg_matches(content, RATE_LIMIT_PATTERNS);
        }
        if (has_any_cg(content, DATA_HANDLING_PATTERNS)) {
            gs->has_data_handling = 1;
            gs->data_handling_count += count_cg_matches(content, DATA_HANDLING_PATTERNS);
        }
        if (has_any_cg(content, SCOPE_PATTERNS)) {
            gs->has_scope = 1;
            gs->scope_count += count_cg_matches(content, SCOPE_PATTERNS);
        }
        if (has_any_cg(content, EMERGENCY_PATTERNS)) {
            gs->has_emergency = 1;
            gs->emergency_count += count_cg_matches(content, EMERGENCY_PATTERNS);
        }
        if (has_any_cg(content, BUDGET_PATTERNS)) {
            gs->has_budget = 1;
            gs->budget_count += count_cg_matches(content, BUDGET_PATTERNS);
        }
        if (has_any_cg(content, BYPASS_ANTIPATTERNS)) {
            gs->has_bypass = 1;
            gs->bypass_count += count_cg_matches(content, BYPASS_ANTIPATTERNS);
        }

        gs->gdpr_count    += count_cg_matches(content, GDPR_PATTERNS);
        gs->hipaa_count   += count_cg_matches(content, HIPAA_PATTERNS);
        gs->nist_ai_count += count_cg_matches(content, NIST_AI_PATTERNS);

        free(content);
    }
}

/* --------------------------------------------------------------------------
 * Guardrail rule validation
 * -------------------------------------------------------------------------- */

static void validate_guardrail_rules(
    lst_artifact_t *art, int *error_count, int *warning_count
) {
    for (uint32_t i = 0; i < art->guardrail_count; i++) {
        lst_guardrail_t *gr = &art->guardrails[i];

        if (!gr->enabled) {
            printf("  [WARNING] Guardrail '%s' is DISABLED\n", gr->rule_id);
            (*warning_count)++;
        }

        if (gr->bypass_count > 0) {
            printf("  [WARNING] Guardrail '%s' has %u authorized bypasses\n",
                   gr->rule_id, gr->bypass_count);
            (*warning_count)++;
        }

        /* HITL gates should never be disabled for high-severity rules */
        if (gr->guardrail_type == GUARD_HITL_GATE &&
            gr->severity_on_breach >= SEV_CRITICAL && !gr->enabled) {
            printf("  [ERROR] Critical HITL gate '%s' is disabled\n", gr->rule_id);
            (*error_count)++;
        }

        /* Kill switches must always be enabled */
        if (gr->guardrail_type == GUARD_KILL_SWITCH && !gr->enabled) {
            printf("  [CRITICAL] Kill switch '%s' is disabled\n", gr->rule_id);
            (*error_count)++;
        }
    }
}

/* --------------------------------------------------------------------------
 * Compliance coverage scoring
 * -------------------------------------------------------------------------- */

static float compute_guardrail_coverage(guardrail_scan_t *gs) {
    float score = 0.0f;
    float max_score = 6.0f;

    if (gs->has_hitl)          score += 1.0f;
    if (gs->has_rate_limit)    score += 1.0f;
    if (gs->has_data_handling) score += 1.0f;
    if (gs->has_scope)         score += 1.0f;
    if (gs->has_emergency)     score += 1.0f;
    if (gs->has_budget)        score += 1.0f;

    return (score / max_score) * 100.0f;
}

/* --------------------------------------------------------------------------
 * Recipe entry point
 * -------------------------------------------------------------------------- */

static int recipe_compliance_guardrails(lst_artifact_t *art, const char *output_dir) {
    (void)output_dir;

    printf("\n");
    for (int i = 0; i < 70; i++) putchar('=');
    printf("\nCOMPLIANCE GUARDRAILS REPORT\n");
    printf("Project: %s\n", art->project_name);
    printf("Aligned: NIST AI RMF, EU AI Act, GDPR, HIPAA, SOC2\n");
    for (int i = 0; i < 70; i++) putchar('=');
    printf("\n\n");

    int critical = 0, error = 0, warning = 0, info = 0;

    guardrail_scan_t gs;
    scan_guardrails(art, &gs);

    /* Check 1: HITL gates */
    printf("[1/8] Human-in-the-Loop Gates\n");
    if (gs.has_hitl) {
        printf("  [INFO] HITL gate patterns found (%d references)\n", gs.hitl_count);
        info++;
    } else {
        printf("  [CRITICAL] No HITL gates found\n");
        printf("    EU AI Act Art.14 requires human oversight for high-risk AI\n");
        critical++;

        if (art->issue_count < LST_MAX_ISSUES) {
            lst_issue_t *iss = &art->issues[art->issue_count++];
            snprintf(iss->id, sizeof(iss->id), "CGR-001");
            iss->severity = SEV_CRITICAL;
            snprintf(iss->title, sizeof(iss->title),
                     "No human-in-the-loop gates");
            snprintf(iss->description, sizeof(iss->description),
                     "High-risk AI systems require human oversight mechanisms");
            snprintf(iss->cwe, sizeof(iss->cwe), "EU-AI-14");
            snprintf(iss->remediation, sizeof(iss->remediation),
                     "Implement HITL approval gates for critical agent actions");
        }
    }

    /* Check 2: Rate limiting */
    printf("\n[2/8] Rate Limiting\n");
    if (gs.has_rate_limit) {
        printf("  [INFO] Rate limiting found (%d references)\n", gs.rate_limit_count);
        info++;
    } else {
        printf("  [ERROR] No rate limiting infrastructure found\n");
        printf("    Agent actions should be rate-limited to prevent runaway behavior\n");
        error++;
    }

    /* Check 3: Data handling */
    printf("\n[3/8] Data Handling Controls\n");
    if (gs.has_data_handling) {
        printf("  [INFO] Data handling controls found (%d references)\n",
               gs.data_handling_count);
        info++;
    } else {
        printf("  [ERROR] No data handling controls found\n");
        printf("    PII/PHI must be encrypted, redacted, or anonymized\n");
        error++;
    }

    /* Check 4: Scope constraints */
    printf("\n[4/8] Scope Constraints\n");
    if (gs.has_scope) {
        printf("  [INFO] Scope constraint patterns found (%d references)\n",
               gs.scope_count);
        info++;
    } else {
        printf("  [ERROR] No scope constraints found\n");
        printf("    Agents must operate within defined capability boundaries\n");
        error++;
    }

    /* Check 5: Emergency controls */
    printf("\n[5/8] Emergency Controls\n");
    if (gs.has_emergency) {
        printf("  [INFO] Emergency controls found (%d references)\n",
               gs.emergency_count);
        info++;
    } else {
        printf("  [CRITICAL] No emergency controls (kill switch, circuit breaker)\n");
        printf("    Agents must have emergency termination capability\n");
        critical++;
    }

    /* Check 6: Budget/resource caps */
    printf("\n[6/8] Budget / Resource Caps\n");
    if (gs.has_budget) {
        printf("  [INFO] Budget/resource caps found (%d references)\n",
               gs.budget_count);
        info++;
    } else {
        printf("  [WARNING] No budget or resource caps found\n");
        printf("    Consider adding cost limits, token caps, or usage quotas\n");
        warning++;
    }

    /* Check 7: Bypass anti-patterns */
    printf("\n[7/8] Guardrail Bypass Detection\n");
    if (gs.has_bypass) {
        printf("  [CRITICAL] Guardrail bypass patterns detected (%d occurrences)\n",
               gs.bypass_count);
        printf("    Found: skip_validation, disable_safety, force_execute, etc.\n");
        printf("    These undermine compliance and must be removed or gated\n");
        critical++;

        if (art->issue_count < LST_MAX_ISSUES) {
            lst_issue_t *iss = &art->issues[art->issue_count++];
            snprintf(iss->id, sizeof(iss->id), "CGR-007");
            iss->severity = SEV_CRITICAL;
            snprintf(iss->title, sizeof(iss->title),
                     "Guardrail bypass patterns detected");
            snprintf(iss->description, sizeof(iss->description),
                     "%d bypass anti-patterns found in codebase",
                     gs.bypass_count);
            snprintf(iss->cwe, sizeof(iss->cwe), "NIST-GOV");
            snprintf(iss->remediation, sizeof(iss->remediation),
                     "Remove or gate all guardrail bypass mechanisms");
        }
    } else {
        printf("  [INFO] No guardrail bypass anti-patterns detected\n");
        info++;
    }

    /* Check 8: Regulatory coverage */
    printf("\n[8/8] Regulatory Coverage\n");
    printf("  GDPR patterns:     %d\n", gs.gdpr_count);
    printf("  HIPAA patterns:    %d\n", gs.hipaa_count);
    printf("  NIST AI patterns:  %d\n", gs.nist_ai_count);

    if (gs.gdpr_count == 0 && gs.hipaa_count == 0 && gs.nist_ai_count == 0) {
        printf("  [WARNING] No regulatory-specific compliance patterns found\n");
        warning++;
    } else {
        printf("  [INFO] Regulatory compliance patterns detected\n");
        info++;
    }

    /* Validate guardrail rules in artifact */
    if (art->guardrail_count > 0) {
        printf("\nGuardrail Rule Validation (%u rules)\n", art->guardrail_count);
        validate_guardrail_rules(art, &error, &warning);
    }

    /* Guardrail coverage score */
    float coverage = compute_guardrail_coverage(&gs);

    /* Summary */
    printf("\n");
    for (int i = 0; i < 70; i++) putchar('-');
    printf("\nGUARDRAIL COVERAGE: %.0f%% (6 categories)\n", (double)coverage);
    printf("SUMMARY: Critical=%d Error=%d Warning=%d Info=%d\n",
           critical, error, warning, info);

    int pass = (critical == 0);
    printf("RESULT: %s\n", pass ? "PASS" : "FAIL");
    for (int i = 0; i < 70; i++) putchar('=');
    printf("\n");

    return pass ? 0 : 1;
}

void recipe_compliance_guardrails_register(void) {
    lst_recipe_t r;
    memset(&r, 0, sizeof(r));
    snprintf(r.name, sizeof(r.name), "compliance_guardrails");
    snprintf(r.description, sizeof(r.description),
             "Compliance guardrail validation (NIST/EU AI Act/GDPR/HIPAA)");
    r.execute = recipe_compliance_guardrails;
    r.version = 1;
    lst_recipe_register(&r);
}
