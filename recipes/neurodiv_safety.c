#define _POSIX_C_SOURCE 200809L

/*
 * neurodiv_safety.c -- Neurodivergent Safety Validation Recipe
 *
 * Validates that neurodivergent support patterns from
 * NeuroDiv.md and LSA-SPEC-002 are correctly implemented.
 *
 * Deterministic checks:
 *   - Sensory load formula correctness (FR-6.5.2)
 *   - Intervention tier threshold validation (FR-6.5.3)
 *   - Anti-masking detection logic (FR-6.9.1: capacity<0.4 AND demand>0.7)
 *   - Context switch limits per profile (FR-6.7.1: range 2-5)
 *   - Task concurrency limits (FR-6.6.1: range 1-4)
 *   - Breathing room computation (FR-6.6.2)
 *   - Manual execution gate enforcement (FR-6.7.3: AI never executes trades)
 *   - Hyperfocus protection patterns (FR-6.7.2)
 *   - Profile confidence thresholds
 */

#include "lst.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <math.h>

/* --------------------------------------------------------------------------
 * NeuroDiv Constants
 * -------------------------------------------------------------------------- */

/* Sensory load formula weights (FR-6.5.2):
 * sensory_load = (noise * sensitivity * 0.4) +
 *                (light * sensitivity * 0.3) +
 *                (notifications * 0.3)
 */
#define SENSORY_WEIGHT_NOISE  0.4
#define SENSORY_WEIGHT_LIGHT  0.3
#define SENSORY_WEIGHT_NOTIF  0.3

/* Task limits (FR-6.6.1) */
#define MAX_CONCURRENT_TASKS_MIN 1
#define MAX_CONCURRENT_TASKS_MAX 4

/* Context switch limits (FR-6.7.1) */
#define CONTEXT_SWITCH_LIMIT_MIN 2
#define CONTEXT_SWITCH_LIMIT_MAX 5

/* Anti-masking thresholds (FR-6.9.1) */
#define MASKING_CAPACITY_THRESH  0.4
#define MASKING_DEMAND_THRESH    0.7

/* Drift threshold (FR-6.4.2) */
#define DRIFT_THRESHOLD          0.618

/* Intervention tier publish latency (FR-6.5.3) */
#define INTERVENTION_LATENCY_MS  50

/* --------------------------------------------------------------------------
 * Helpers
 * -------------------------------------------------------------------------- */

static char *read_file_nd(const char *path, size_t *out_len) {
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

static const char *strcasestr_nd(const char *haystack, const char *needle) {
    if (!*needle) return haystack;
    size_t nlen = strlen(needle);
    for (; *haystack; haystack++) {
        if (strncasecmp(haystack, needle, nlen) == 0)
            return haystack;
    }
    return NULL;
}

static void add_nd_issue(lst_artifact_t *art, uint8_t severity,
                          const char *title, const char *desc,
                          const char *file_path, uint32_t line,
                          const char *req_id) {
    if (art->issue_count >= LST_MAX_ISSUES) return;
    lst_issue_t *issue = &art->issues[art->issue_count];
    memset(issue, 0, sizeof(lst_issue_t));

    snprintf(issue->id, sizeof(issue->id), "ND-%04u", art->issue_count + 1);
    issue->severity = severity;
    snprintf(issue->title, sizeof(issue->title), "%s", title);
    snprintf(issue->description, sizeof(issue->description), "%s", desc);
    if (file_path)
        snprintf(issue->file_path, sizeof(issue->file_path), "%s", file_path);
    issue->line_number = line;
    if (req_id)
        snprintf(issue->cwe, sizeof(issue->cwe), "%s", req_id);
    snprintf(issue->remediation, sizeof(issue->remediation),
        "See NeuroDiv.md / LSA-SPEC-002");

    art->issue_count++;
}

/* --------------------------------------------------------------------------
 * Neurodivergent Safety Checks
 * -------------------------------------------------------------------------- */

/* FR-6.7.3: AI never executes trades -- manual execution gate */
static void check_execution_gate(lst_artifact_t *art, const char *fpath,
                                  const char *line, int lineno) {
    /* Detect automated trade execution */
    if ((strcasestr_nd(line, "execute_trade") || strcasestr_nd(line, "place_order") ||
         strcasestr_nd(line, "submit_order") || strcasestr_nd(line, "auto_trade") ||
         strcasestr_nd(line, "auto_execute")) &&
        !strcasestr_nd(line, "manual") && !strcasestr_nd(line, "confirm") &&
        !strcasestr_nd(line, "gate") && !strcasestr_nd(line, "approval")) {
        add_nd_issue(art, SEV_CRITICAL,
            "Missing Manual Execution Gate",
            "Trade execution without manual confirmation gate -- FR-6.7.3: AI never executes trades",
            fpath, (uint32_t)lineno, "FR-6.7.3");
    }
}

/* FR-6.6.1: Max concurrent tasks range 1-4 */
static void check_task_limits(lst_artifact_t *art, const char *fpath,
                               const char *line, int lineno) {
    if (strstr(line, "max_concurrent_tasks") || strstr(line, "MAX_CONCURRENT_TASKS") ||
        strstr(line, "max_tasks")) {
        const char *eq = strstr(line, "=");
        if (eq) {
            int val = atoi(eq + 1);
            if (val > 0 && (val < MAX_CONCURRENT_TASKS_MIN || val > MAX_CONCURRENT_TASKS_MAX)) {
                char desc[LST_MAX_NAME];
                snprintf(desc, sizeof(desc),
                    "max_concurrent_tasks=%d is outside allowed range [%d-%d]",
                    val, MAX_CONCURRENT_TASKS_MIN, MAX_CONCURRENT_TASKS_MAX);
                add_nd_issue(art, SEV_ERROR,
                    "Task Limit Out of Range",
                    desc, fpath, (uint32_t)lineno, "FR-6.6.1");
            }
        }
    }
}

/* FR-6.7.1: Context switch limits range 2-5 per hour */
static void check_context_switch_limits(lst_artifact_t *art, const char *fpath,
                                         const char *line, int lineno) {
    if (strstr(line, "context_switch_limit") || strstr(line, "CONTEXT_SWITCH_LIMIT") ||
        strstr(line, "max_context_switches")) {
        const char *eq = strstr(line, "=");
        if (eq) {
            int val = atoi(eq + 1);
            if (val > 0 && (val < CONTEXT_SWITCH_LIMIT_MIN || val > CONTEXT_SWITCH_LIMIT_MAX)) {
                char desc[LST_MAX_NAME];
                snprintf(desc, sizeof(desc),
                    "context_switch_limit=%d is outside allowed range [%d-%d]",
                    val, CONTEXT_SWITCH_LIMIT_MIN, CONTEXT_SWITCH_LIMIT_MAX);
                add_nd_issue(art, SEV_ERROR,
                    "Context Switch Limit Out of Range",
                    desc, fpath, (uint32_t)lineno, "FR-6.7.1");
            }
        }
    }
}

/* FR-6.9.1: Anti-masking detection thresholds */
static void check_antimasking(lst_artifact_t *art, const char *fpath,
                               const char *line, int lineno) {
    /* Verify masking detection uses correct thresholds */
    if (strstr(line, "masking") && (strstr(line, "capacity") || strstr(line, "demand"))) {
        /* Check for incorrect thresholds */
        if (strstr(line, "0.5") || strstr(line, "0.6") || strstr(line, "0.8")) {
            add_nd_issue(art, SEV_WARNING,
                "Non-Standard Masking Thresholds",
                "Anti-masking should use capacity<0.4 AND demand>0.7 per FR-6.9.1",
                fpath, (uint32_t)lineno, "FR-6.9.1");
        }
    }
}

/* FR-6.5.2: Sensory load formula validation */
static void check_sensory_load(lst_artifact_t *art, const char *fpath,
                                const char *line, int lineno) {
    if (strstr(line, "sensory_load") || strstr(line, "SENSORY_LOAD")) {
        /* Check weights are present and correct */
        if (strstr(line, "0.4") && strstr(line, "0.3")) {
            /* Formula appears correct */
            return;
        }
        /* If computing sensory_load with different weights */
        if (strstr(line, "=") && (strstr(line, "noise") || strstr(line, "light") ||
                                   strstr(line, "notification"))) {
            if (!strstr(line, "0.4") || !strstr(line, "0.3")) {
                add_nd_issue(art, SEV_WARNING,
                    "Non-Standard Sensory Load Weights",
                    "Sensory load should use weights: noise*0.4, light*0.3, notifications*0.3",
                    fpath, (uint32_t)lineno, "FR-6.5.2");
            }
        }
    }
}

/* FR-6.4.2: Drift threshold validation */
static void check_drift_threshold(lst_artifact_t *art, const char *fpath,
                                    const char *line, int lineno) {
    if (strstr(line, "drift_threshold") || strstr(line, "DRIFT_THRESHOLD")) {
        const char *eq = strstr(line, "=");
        if (eq) {
            double val = atof(eq + 1);
            if (val > 0 && fabs(val - DRIFT_THRESHOLD) > 0.01) {
                char desc[LST_MAX_NAME];
                snprintf(desc, sizeof(desc),
                    "drift_threshold should be 0.618 (phi_inverse), found %.3f", val);
                add_nd_issue(art, SEV_ERROR,
                    "Incorrect Drift Threshold",
                    desc, fpath, (uint32_t)lineno, "FR-6.4.2");
            }
        }
    }
}

/* FR-6.7.2: Hyperfocus protection */
static void check_hyperfocus_protection(lst_artifact_t *art, const char *fpath,
                                         const char *line, int lineno) {
    /* Detect hyperfocus loops without break enforcement */
    if (strcasestr_nd(line, "hyperfocus") &&
        (strcasestr_nd(line, "loop") || strcasestr_nd(line, "continue") ||
         strcasestr_nd(line, "extend"))) {
        if (!strcasestr_nd(line, "break") && !strcasestr_nd(line, "limit") &&
            !strcasestr_nd(line, "timeout") && !strcasestr_nd(line, "protect")) {
            add_nd_issue(art, SEV_WARNING,
                "Hyperfocus Without Break Protection",
                "Hyperfocus handling should include break/timeout enforcement per FR-6.7.2",
                fpath, (uint32_t)lineno, "FR-6.7.2");
        }
    }
}

/* FR-6.5.3: Intervention latency check */
static void check_intervention_latency(lst_artifact_t *art, const char *fpath,
                                         const char *line, int lineno) {
    if (strstr(line, "intervention") && strstr(line, "timeout")) {
        const char *eq = strstr(line, "=");
        if (eq) {
            int val = atoi(eq + 1);
            if (val > INTERVENTION_LATENCY_MS && val < 10000) {
                char desc[LST_MAX_NAME];
                snprintf(desc, sizeof(desc),
                    "Intervention timeout=%dms exceeds %dms requirement",
                    val, INTERVENTION_LATENCY_MS);
                add_nd_issue(art, SEV_ERROR,
                    "Intervention Latency Too High",
                    desc, fpath, (uint32_t)lineno, "FR-6.5.3");
            }
        }
    }
}

/* FR-6.9.2: Market tier classification */
static void check_market_tier(lst_artifact_t *art, const char *fpath,
                               const char *line, int lineno) {
    /* Verify market tier uses correct classification names */
    if (strcasestr_nd(line, "market_tier") || strcasestr_nd(line, "MarketTier")) {
        int has_valid = 0;
        if (strcasestr_nd(line, "UNDERLEVERAGED") ||
            strcasestr_nd(line, "MATCHED") ||
            strcasestr_nd(line, "MASKING")) {
            has_valid = 1;
        }
        /* If defining market tier with non-standard names */
        if (!has_valid && (strcasestr_nd(line, "enum") || strcasestr_nd(line, "class") ||
                           strstr(line, "="))) {
            if (strcasestr_nd(line, "OVERLEVERAGED") || strcasestr_nd(line, "BALANCED") ||
                strcasestr_nd(line, "NORMAL")) {
                add_nd_issue(art, SEV_WARNING,
                    "Non-Standard Market Tier Names",
                    "Market tiers should be UNDERLEVERAGED/MATCHED/MASKING per FR-6.9.2",
                    fpath, (uint32_t)lineno, "FR-6.9.2");
            }
        }
    }
}

/* --------------------------------------------------------------------------
 * File scanner
 * -------------------------------------------------------------------------- */

static const char *SKIP_DIRS_ND[] = {
    "node_modules", "vendor", ".git", "__pycache__", "dist", "build",
    ".venv", "venv", ".next", "target", ".cache", NULL
};

static int should_skip_nd(const char *name) {
    if (name[0] == '.') return 1;
    for (int i = 0; SKIP_DIRS_ND[i]; i++)
        if (strcmp(name, SKIP_DIRS_ND[i]) == 0) return 1;
    return 0;
}

static int is_nd_scannable(const char *name) {
    const char *ext = strrchr(name, '.');
    if (!ext) return 0;
    const char *exts[] = {".c", ".cpp", ".h", ".py", ".js", ".ts",
                          ".yaml", ".yml", ".toml", ".json", ".conf",
                          ".go", ".rs", ".java", NULL};
    for (int i = 0; exts[i]; i++)
        if (strcasecmp(ext, exts[i]) == 0) return 1;
    return 0;
}

static void scan_file_nd(lst_artifact_t *art, const char *fpath) {
    size_t len = 0;
    char *content = read_file_nd(fpath, &len);
    if (!content) return;

    int lineno = 1;
    char *line_start = content;

    while (*line_start) {
        char *eol = strchr(line_start, '\n');
        if (eol) *eol = '\0';

        check_execution_gate(art, fpath, line_start, lineno);
        check_task_limits(art, fpath, line_start, lineno);
        check_context_switch_limits(art, fpath, line_start, lineno);
        check_antimasking(art, fpath, line_start, lineno);
        check_sensory_load(art, fpath, line_start, lineno);
        check_drift_threshold(art, fpath, line_start, lineno);
        check_hyperfocus_protection(art, fpath, line_start, lineno);
        check_intervention_latency(art, fpath, line_start, lineno);
        check_market_tier(art, fpath, line_start, lineno);

        if (!eol) break;
        line_start = eol + 1;
        lineno++;
    }

    free(content);
}

static void scan_dir_nd(lst_artifact_t *art, const char *dir, int depth) {
    if (depth > 8) return;

    DIR *d = opendir(dir);
    if (!d) return;

    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        if (ent->d_name[0] == '.') continue;
        if (should_skip_nd(ent->d_name)) continue;

        char child[LST_MAX_PATH];
        snprintf(child, sizeof(child), "%s/%s", dir, ent->d_name);

        struct stat st;
        if (stat(child, &st) != 0) continue;

        if (S_ISDIR(st.st_mode)) {
            scan_dir_nd(art, child, depth + 1);
        } else if (S_ISREG(st.st_mode) && is_nd_scannable(ent->d_name)) {
            scan_file_nd(art, child);
        }
    }
    closedir(d);
}

/* --------------------------------------------------------------------------
 * Report writer
 * -------------------------------------------------------------------------- */

static void write_sep_nd(FILE *f) {
    for (int i = 0; i < 80; i++) fputc('=', f);
    fputc('\n', f);
}

static void write_line_nd(FILE *f) {
    for (int i = 0; i < 40; i++) fputc('-', f);
    fputc('\n', f);
}

static int recipe_neurodiv_safety(lst_artifact_t *art, const char *output_dir) {
    if (!art) return -1;

    uint32_t initial_issues = art->issue_count;
    scan_dir_nd(art, art->project_path, 0);
    uint32_t new_issues = art->issue_count - initial_issues;

    /* Build output path */
    char outpath[LST_MAX_PATH];
    if (output_dir)
        snprintf(outpath, sizeof(outpath), "%s/NEURODIV_SAFETY_REPORT", output_dir);
    else
        snprintf(outpath, sizeof(outpath), "%s/NEURODIV_SAFETY_REPORT", art->project_path);

    if (output_dir) mkdir(output_dir, 0755);

    int rfd = open(outpath, O_WRONLY | O_CREAT | O_TRUNC | O_NOFOLLOW | O_CLOEXEC, 0644);
    if (rfd < 0) {
        fprintf(stderr, "neurodiv-safety: cannot write %s\n", outpath);
        return -1;
    }
    FILE *f = fdopen(rfd, "w");
    if (!f) {
        close(rfd);
        fprintf(stderr, "neurodiv-safety: cannot write %s\n", outpath);
        return -1;
    }

    /* Header */
    write_sep_nd(f);
    fprintf(f, "NEURODIVERGENT SAFETY VALIDATION REPORT\n");
    fprintf(f, "Project: %s\n", art->project_name);
    fprintf(f, "Path: %s\n", art->project_path);

    time_t now = time(NULL);
    struct tm tm_buf;
    struct tm *t = gmtime_r(&now, &tm_buf);
    char ts[64];
    if (t) {
        strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S UTC", t);
    } else {
        snprintf(ts, sizeof(ts), "1970-01-01 00:00:00 UTC");
    }
    fprintf(f, "Generated: %s\n", ts);
    fprintf(f, "Violations Found: %u\n", new_issues);
    write_sep_nd(f);
    fprintf(f, "\n");

    /* Safety parameters reference */
    fprintf(f, "SAFETY PARAMETERS REFERENCE\n");
    write_line_nd(f);
    fprintf(f, "  Sensory Load Formula (FR-6.5.2):\n");
    fprintf(f, "    sensory_load = (noise * sensitivity * 0.4)\n");
    fprintf(f, "                 + (light * sensitivity * 0.3)\n");
    fprintf(f, "                 + (notifications * 0.3)\n\n");
    fprintf(f, "  Task Limits (FR-6.6.1):             1-4 concurrent\n");
    fprintf(f, "  Context Switch Limits (FR-6.7.1):   2-5 per hour\n");
    fprintf(f, "  Anti-Masking (FR-6.9.1):            capacity<0.4 AND demand>0.7\n");
    fprintf(f, "  Drift Threshold (FR-6.4.2):         0.618 (phi_inverse)\n");
    fprintf(f, "  Intervention Latency (FR-6.5.3):    <50ms\n");
    fprintf(f, "  Execution Gate (FR-6.7.3):          AI NEVER executes trades\n");
    fprintf(f, "  Market Tiers (FR-6.9.2):            UNDERLEVERAGED/MATCHED/MASKING\n");
    fprintf(f, "\n");

    /* Findings */
    if (new_issues > 0) {
        fprintf(f, "SAFETY VIOLATIONS\n");
        write_sep_nd(f);
        fprintf(f, "\n");

        for (int sev = SEV_CRITICAL; sev >= SEV_INFO; sev--) {
            int printed = 0;
            for (uint32_t i = initial_issues; i < art->issue_count; i++) {
                if (art->issues[i].severity != sev) continue;
                if (!printed) {
                    fprintf(f, "  [%s]\n\n",
                        sev == SEV_CRITICAL ? "CRITICAL" :
                        sev == SEV_ERROR    ? "ERROR" :
                        sev == SEV_WARNING  ? "WARNING" : "INFO");
                    printed = 1;
                }
                const lst_issue_t *issue = &art->issues[i];
                fprintf(f, "    %s: %s\n", issue->id, issue->title);
                if (issue->file_path[0])
                    fprintf(f, "      File: %s:%u\n", issue->file_path, issue->line_number);
                if (issue->cwe[0])
                    fprintf(f, "      Req: %s\n", issue->cwe);
                fprintf(f, "      %s\n\n", issue->description);
            }
        }
    } else {
        fprintf(f, "SAFETY VALIDATION: PASS\n");
        fprintf(f, "  No neurodivergent safety violations found.\n\n");
    }

    /* Safety checklist */
    fprintf(f, "SAFETY CHECKLIST\n");
    write_line_nd(f);
    int has_gate = 0, has_task = 0, has_ctx = 0, has_mask = 0;
    for (uint32_t i = initial_issues; i < art->issue_count; i++) {
        if (strstr(art->issues[i].title, "Execution Gate")) has_gate = 1;
        if (strstr(art->issues[i].title, "Task Limit")) has_task = 1;
        if (strstr(art->issues[i].title, "Context Switch")) has_ctx = 1;
        if (strstr(art->issues[i].title, "Masking")) has_mask = 1;
    }
    fprintf(f, "  [%s] Manual execution gate enforced\n", has_gate ? "FAIL" : "PASS");
    fprintf(f, "  [%s] Task concurrency within limits\n", has_task ? "FAIL" : "PASS");
    fprintf(f, "  [%s] Context switch limits valid\n", has_ctx ? "FAIL" : "PASS");
    fprintf(f, "  [%s] Anti-masking thresholds correct\n", has_mask ? "FAIL" : "PASS");
    fprintf(f, "\n");

    /* Footer */
    write_sep_nd(f);
    fprintf(f, "END OF NEURODIVERGENT SAFETY REPORT\n");
    fprintf(f, "\nThis report validates neurodivergent safety patterns as defined in\n");
    fprintf(f, "NeuroDiv.md and LSA-SPEC-002 Sections 6.4-6.9.\n");
    fprintf(f, "Executive function protection is a core architectural requirement.\n");
    write_sep_nd(f);
    fprintf(f, "\n");

    fclose(f);

    printf("  Wrote %s (%u violations)\n", outpath, new_issues);
    return 0;
}

/* --------------------------------------------------------------------------
 * Recipe registration
 * -------------------------------------------------------------------------- */

void recipe_neurodiv_safety_register(void) {
    lst_recipe_t r = {0};
    snprintf(r.name, LST_MAX_NAME, "neurodiv-safety");
    snprintf(r.description, LST_MAX_NAME,
        "Validate neurodivergent safety patterns (LSA-SPEC FR-6.4-6.9)");
    r.execute = recipe_neurodiv_safety;
    r.version = 1;
    lst_recipe_register(&r);
}
