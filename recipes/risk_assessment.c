/*
 * risk_assessment.c -- AI Agent Risk Assessment Recipe
 *
 * Multi-dimensional risk scoring aligned with NIST AI RMF:
 *   - Capability Risk: tool/API access scope
 *   - Data Access Risk: sensitivity of accessible data
 *   - Autonomy Risk: unsupervised operation level
 *   - Impact Risk: blast radius of agent actions
 *   - Temporal Risk: duration/persistence of effects
 *
 * Produces aggregate risk tier (MINIMAL..CRITICAL) and maps
 * to HITL requirements per EU AI Act Article 14.
 */

#include "lst.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* --------------------------------------------------------------------------
 * Risk dimension weights (sum to 1.0)
 * -------------------------------------------------------------------------- */

static const float RISK_WEIGHTS[RISK_DIM_COUNT] = {
    0.20f,  /* RISK_CAPABILITY   */
    0.25f,  /* RISK_DATA_ACCESS  */
    0.25f,  /* RISK_AUTONOMY     */
    0.20f,  /* RISK_IMPACT       */
    0.10f,  /* RISK_TEMPORAL     */
};

/* Risk tier thresholds (aggregate score 0-100) */
#define TIER_LOW_THRESH      20.0f
#define TIER_MODERATE_THRESH 40.0f
#define TIER_HIGH_THRESH     70.0f
#define TIER_CRITICAL_THRESH 90.0f

/* --------------------------------------------------------------------------
 * Capability risk indicators
 * -------------------------------------------------------------------------- */

/* Patterns that increase capability risk */
static const char *HIGH_CAPABILITY_PATTERNS[] = {
    "exec(",        "subprocess",   "os.system",    "shell_exec",
    "eval(",        "Function(",    "child_process", "spawn(",
    "popen(",       "system(",      "runtime.exec",
    NULL
};

static const char *NETWORK_PATTERNS[] = {
    "http.request", "fetch(",       "urllib",       "requests.get",
    "requests.post", "axios",       "curl_exec",   "http.Get",
    "net.Dial",     "socket(",      "websocket",
    NULL
};

static const char *FILE_WRITE_PATTERNS[] = {
    "fwrite(",      "writeFile",    "open(.*w",     "fs.write",
    "io.Writer",    "BufferedWriter", "StreamWriter",
    NULL
};

/* --------------------------------------------------------------------------
 * Data access risk indicators
 * -------------------------------------------------------------------------- */

static const char *PII_PATTERNS[] = {
    "email",        "ssn",          "social_security", "date_of_birth",
    "phone_number", "address",      "credit_card",  "passport",
    "medical_record", "health_record", "patient_id", "diagnosis",
    NULL
};

static const char *SECRET_PATTERNS[] = {
    "api_key",      "API_KEY",      "secret_key",   "SECRET_KEY",
    "password",     "PASSWORD",     "private_key",  "PRIVATE_KEY",
    "access_token", "ACCESS_TOKEN", "auth_token",
    NULL
};

/* --------------------------------------------------------------------------
 * Autonomy risk indicators
 * -------------------------------------------------------------------------- */

static const char *AUTO_DECISION_PATTERNS[] = {
    "auto_execute",  "auto_approve", "autonomous",   "self_directed",
    "auto_deploy",   "auto_commit",  "auto_merge",   "auto_trade",
    "auto_transfer", "unattended",   "headless",
    NULL
};

static const char *DELEGATION_PATTERNS[] = {
    "delegate(",     "spawn_agent",  "sub_agent",    "SubAgent",
    "create_agent",  "fork_agent",   "child_agent",
    NULL
};

/* --------------------------------------------------------------------------
 * Impact risk indicators
 * -------------------------------------------------------------------------- */

static const char *DESTRUCTIVE_PATTERNS[] = {
    "DELETE FROM",  "DROP TABLE",   "rm -rf",       "rmdir",
    "unlink(",      "remove(",      "shutil.rmtree", "force_push",
    "reset --hard", "destroy(",     "truncate(",
    NULL
};

static const char *EXTERNAL_PATTERNS[] = {
    "send_email",   "send_message", "post_slack",   "publish(",
    "deploy(",      "push(",        "release(",     "notify(",
    "broadcast(",   "webhook(",
    NULL
};

/* --------------------------------------------------------------------------
 * Helpers
 * -------------------------------------------------------------------------- */

static char *read_file_risk(const char *path, size_t *out_len) {
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

static int count_pattern_matches(const char *content, const char **patterns) {
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

static float normalize_score(int raw_count, int low_thresh, int high_thresh) {
    if (raw_count <= low_thresh) return 0.0f;
    if (raw_count >= high_thresh) return 1.0f;
    return (float)(raw_count - low_thresh) / (float)(high_thresh - low_thresh);
}

static const char *tier_name(risk_tier_t tier) {
    switch (tier) {
        case RISK_TIER_MINIMAL:  return "MINIMAL";
        case RISK_TIER_LOW:      return "LOW";
        case RISK_TIER_MODERATE: return "MODERATE";
        case RISK_TIER_HIGH:     return "HIGH";
        case RISK_TIER_CRITICAL: return "CRITICAL";
        default:                 return "UNKNOWN";
    }
}

static const char *hitl_name(hitl_requirement_t hitl) {
    switch (hitl) {
        case HITL_NONE:     return "NONE (autonomous)";
        case HITL_ADVISORY: return "ADVISORY (human notified)";
        case HITL_APPROVAL: return "APPROVAL (human approves)";
        case HITL_MANDATORY:return "MANDATORY (human executes)";
        default:            return "UNKNOWN";
    }
}

static risk_tier_t score_to_tier(float score) {
    if (score < TIER_LOW_THRESH)      return RISK_TIER_MINIMAL;
    if (score < TIER_MODERATE_THRESH) return RISK_TIER_LOW;
    if (score < TIER_HIGH_THRESH)     return RISK_TIER_MODERATE;
    if (score < TIER_CRITICAL_THRESH) return RISK_TIER_HIGH;
    return RISK_TIER_CRITICAL;
}

static hitl_requirement_t tier_to_hitl(risk_tier_t tier) {
    switch (tier) {
        case RISK_TIER_MINIMAL:  return HITL_NONE;
        case RISK_TIER_LOW:      return HITL_ADVISORY;
        case RISK_TIER_MODERATE: return HITL_ADVISORY;
        case RISK_TIER_HIGH:     return HITL_APPROVAL;
        case RISK_TIER_CRITICAL: return HITL_MANDATORY;
        default:                 return HITL_MANDATORY;
    }
}

/* --------------------------------------------------------------------------
 * Dimension scoring
 * -------------------------------------------------------------------------- */

typedef struct {
    float scores[RISK_DIM_COUNT];
    float aggregate;
    risk_tier_t tier;
    hitl_requirement_t hitl;
    int capability_matches;
    int data_matches;
    int autonomy_matches;
    int impact_matches;
} risk_profile_result_t;

static void compute_risk_profile(lst_artifact_t *art, risk_profile_result_t *rp) {
    int cap_exec = 0, cap_net = 0, cap_file = 0;
    int data_pii = 0, data_secret = 0;
    int auto_decision = 0, auto_delegate = 0;
    int impact_destructive = 0, impact_external = 0;

    for (uint32_t i = 0; i < art->file_count; i++) {
        size_t len = 0;
        char *content = read_file_risk(art->files[i].path, &len);
        if (!content) continue;

        cap_exec += count_pattern_matches(content, HIGH_CAPABILITY_PATTERNS);
        cap_net  += count_pattern_matches(content, NETWORK_PATTERNS);
        cap_file += count_pattern_matches(content, FILE_WRITE_PATTERNS);

        data_pii    += count_pattern_matches(content, PII_PATTERNS);
        data_secret += count_pattern_matches(content, SECRET_PATTERNS);

        auto_decision += count_pattern_matches(content, AUTO_DECISION_PATTERNS);
        auto_delegate += count_pattern_matches(content, DELEGATION_PATTERNS);

        impact_destructive += count_pattern_matches(content, DESTRUCTIVE_PATTERNS);
        impact_external    += count_pattern_matches(content, EXTERNAL_PATTERNS);

        free(content);
    }

    /* Capability: exec + network + file_write */
    rp->scores[RISK_CAPABILITY] = normalize_score(cap_exec + cap_net + cap_file, 2, 30);
    rp->capability_matches = cap_exec + cap_net + cap_file;

    /* Data access: PII + secrets */
    rp->scores[RISK_DATA_ACCESS] = normalize_score(data_pii + data_secret, 1, 20);
    rp->data_matches = data_pii + data_secret;

    /* Autonomy: auto-decisions + delegation */
    rp->scores[RISK_AUTONOMY] = normalize_score(auto_decision + auto_delegate, 1, 15);
    rp->autonomy_matches = auto_decision + auto_delegate;

    /* Impact: destructive + external */
    rp->scores[RISK_IMPACT] = normalize_score(impact_destructive + impact_external, 1, 15);
    rp->impact_matches = impact_destructive + impact_external;

    /* Temporal: based on file count (proxy for system size/persistence) */
    rp->scores[RISK_TEMPORAL] = normalize_score((int)art->file_count, 10, 500);

    /* Aggregate weighted score */
    rp->aggregate = 0.0f;
    for (int d = 0; d < RISK_DIM_COUNT; d++) {
        rp->aggregate += rp->scores[d] * RISK_WEIGHTS[d] * 100.0f;
    }

    rp->tier = score_to_tier(rp->aggregate);
    rp->hitl = tier_to_hitl(rp->tier);
}

/* --------------------------------------------------------------------------
 * Mitigation checks
 * -------------------------------------------------------------------------- */

static const char *MITIGATION_PATTERNS[] = {
    "rate_limit",    "RateLimit",    "throttle(",    "Throttle(",
    "scope_bound",   "ScopeBound",   "permission_check", "PermissionCheck",
    "input_validation", "sanitize(",  "validate(",   "authorize(",
    "encrypt(",      "sign(",        "verify_signature",
    "circuit_breaker", "CircuitBreaker", "timeout(",
    NULL
};

static void check_mitigations(
    lst_artifact_t *art, risk_profile_result_t *rp, int *warning_count
) {
    int mitigation_count = 0;

    for (uint32_t i = 0; i < art->file_count; i++) {
        size_t len = 0;
        char *content = read_file_risk(art->files[i].path, &len);
        if (!content) continue;
        mitigation_count += count_pattern_matches(content, MITIGATION_PATTERNS);
        free(content);
    }

    printf("\n[3/3] Risk Mitigations\n");
    printf("  Mitigation patterns found: %d\n", mitigation_count);

    /* High/critical risk should have proportional mitigations */
    if (rp->tier >= RISK_TIER_HIGH && mitigation_count < 5) {
        printf("  [WARNING] High-risk project has insufficient mitigations (%d found)\n",
               mitigation_count);
        printf("    Recommend: rate limiting, scope bounds, input validation,\n");
        printf("    encryption, circuit breakers\n");
        (*warning_count)++;
    } else if (rp->tier >= RISK_TIER_MODERATE && mitigation_count < 3) {
        printf("  [WARNING] Moderate-risk project has few mitigations (%d found)\n",
               mitigation_count);
        (*warning_count)++;
    } else {
        printf("  [INFO] Mitigation coverage adequate for risk tier\n");
    }
}

/* --------------------------------------------------------------------------
 * Recipe entry point
 * -------------------------------------------------------------------------- */

static int recipe_risk_assessment(lst_artifact_t *art, const char *output_dir) {
    (void)output_dir;

    printf("\n");
    for (int i = 0; i < 70; i++) putchar('=');
    printf("\nAI AGENT RISK ASSESSMENT REPORT\n");
    printf("Project: %s\n", art->project_name);
    printf("Aligned: NIST AI RMF, EU AI Act Art.14\n");
    for (int i = 0; i < 70; i++) putchar('=');
    printf("\n\n");

    int critical = 0, error = 0, warning = 0;

    /* Compute risk profile */
    risk_profile_result_t rp;
    memset(&rp, 0, sizeof(rp));
    compute_risk_profile(art, &rp);

    /* Print dimension scores */
    printf("[1/3] Risk Dimensions\n");
    const char *dim_names[] = {
        "Capability", "Data Access", "Autonomy", "Impact", "Temporal"
    };
    for (int d = 0; d < RISK_DIM_COUNT; d++) {
        printf("  %-14s  %.1f%%  (weight: %.0f%%)\n",
               dim_names[d],
               (double)(rp.scores[d] * 100.0f),
               (double)(RISK_WEIGHTS[d] * 100.0f));
    }

    printf("\n  Pattern matches:\n");
    printf("    Capability (exec/net/file):  %d\n", rp.capability_matches);
    printf("    Data (PII/secrets):          %d\n", rp.data_matches);
    printf("    Autonomy (auto/delegate):    %d\n", rp.autonomy_matches);
    printf("    Impact (destructive/extern): %d\n", rp.impact_matches);

    /* Aggregate results */
    printf("\n[2/3] Aggregate Assessment\n");
    printf("  Aggregate Risk Score: %.1f / 100.0\n", (double)rp.aggregate);
    printf("  Risk Tier:            %s\n", tier_name(rp.tier));
    printf("  HITL Requirement:     %s\n", hitl_name(rp.hitl));

    /* Update artifact risk score */
    art->risk_score = rp.aggregate;

    /* Flag critical risk */
    if (rp.tier == RISK_TIER_CRITICAL) {
        printf("  [CRITICAL] Agent risk tier is CRITICAL\n");
        printf("    Mandatory human oversight required for all operations\n");
        critical++;

        if (art->issue_count < LST_MAX_ISSUES) {
            lst_issue_t *iss = &art->issues[art->issue_count++];
            snprintf(iss->id, sizeof(iss->id), "RSK-001");
            iss->severity = SEV_CRITICAL;
            snprintf(iss->title, sizeof(iss->title),
                     "Critical risk tier requires mandatory human oversight");
            snprintf(iss->description, sizeof(iss->description),
                     "Aggregate risk=%.1f exceeds critical threshold",
                     (double)rp.aggregate);
            snprintf(iss->cwe, sizeof(iss->cwe), "NIST-MGT");
            snprintf(iss->remediation, sizeof(iss->remediation),
                     "Implement HITL_MANDATORY controls before deployment");
        }
    } else if (rp.tier == RISK_TIER_HIGH) {
        printf("  [ERROR] Agent risk tier is HIGH\n");
        printf("    Human approval required for critical actions\n");
        error++;
    }

    /* Check mitigations */
    check_mitigations(art, &rp, &warning);

    /* Summary */
    printf("\n");
    for (int i = 0; i < 70; i++) putchar('-');
    printf("\nSUMMARY: Critical=%d Error=%d Warning=%d\n",
           critical, error, warning);
    printf("RISK TIER: %s  |  HITL: %s\n",
           tier_name(rp.tier), hitl_name(rp.hitl));
    printf("RESULT: %s\n", (critical == 0) ? "PASS" : "FAIL");
    for (int i = 0; i < 70; i++) putchar('=');
    printf("\n");

    return (critical == 0) ? 0 : 1;
}

void recipe_risk_assessment_register(void) {
    lst_recipe_t r;
    memset(&r, 0, sizeof(r));
    snprintf(r.name, sizeof(r.name), "risk_assessment");
    snprintf(r.description, sizeof(r.description),
             "Multi-dimensional AI risk scoring (NIST AI RMF)");
    r.execute = recipe_risk_assessment;
    r.version = 1;
    lst_recipe_register(&r);
}
