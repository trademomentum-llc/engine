/*
 * lsa_compliance.c -- LSA Spec Compliance Validation Recipe
 *
 * Validates project codebases against LSA-SPEC-002 v2.0.0:
 * Jason's Living System Architecture / Tri-Plane Heterogeneous
 * Compute Fabric (TP-HCF).
 *
 * Deterministic checks:
 *   - Workload routing: E1->NUC, E2->M1/Orin, E3->Orin
 *   - Anti-pattern detection (DB on Orin, CUDA on M1, etc.)
 *   - Breathing room constants (phi_inverse, complementarity_zone)
 *   - Security NFRs (AES-256-GCM, secret storage, permissions)
 *   - Daemon inventory per node type
 *   - State ownership rules
 *   - Merge strategy compliance
 */

#include "lst.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <dirent.h>
#include <math.h>

/* --------------------------------------------------------------------------
 * LSA Constants from spec
 * -------------------------------------------------------------------------- */

#define PHI_INVERSE           0.618033988749895
#define COMPLEMENTARITY_ZONE  0.382
#define BREATHING_ROOM_THRESH 0.382

/* Execution classes */
typedef enum {
    EXEC_E1 = 1,  /* Deterministic Control -> NUC  */
    EXEC_E2 = 2,  /* Parallel Numerical    -> M1/Orin */
    EXEC_E3 = 3,  /* Real-Time Reactive    -> Orin */
} exec_class_t;

/* Node roles */
typedef enum {
    NODE_DCN = 0,  /* NUC - Deterministic Control Node */
    NODE_HCN = 1,  /* M1  - Hybrid Cognitive Node      */
    NODE_EPN = 2,  /* Orin - Edge Parallel Node         */
} node_role_t;

/* Daemon specs per node */
static const char *DCN_DAEMONS[] = {
    "lsa_boot", "lsa_task_manager", "lsa_context_gate",
    "lsa_drift_detector", "lsa_ethernet_sync", NULL
};

static const char *HCN_DAEMONS[] = {
    "lsa_boot", "lsa_convergence_bond", "lsa_profile_refiner",
    "lsa_comm_bridge", "lsa_ethernet_sync", NULL
};

static const char *EPN_DAEMONS[] = {
    "lsa_boot", "lsa_state_monitor", "lsa_ethernet_sync", NULL
};

/* --------------------------------------------------------------------------
 * Helpers
 * -------------------------------------------------------------------------- */

static char *read_file_lsa(const char *path, size_t *out_len) {
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

static const char *strcasestr_lsa(const char *haystack, const char *needle) {
    if (!*needle) return haystack;
    size_t nlen = strlen(needle);
    for (; *haystack; haystack++) {
        if (strncasecmp(haystack, needle, nlen) == 0)
            return haystack;
    }
    return NULL;
}

static void add_lsa_issue(lst_artifact_t *art, uint8_t severity,
                           const char *title, const char *desc,
                           const char *file_path, uint32_t line,
                           const char *req_id) {
    if (art->issue_count >= LST_MAX_ISSUES) return;
    lst_issue_t *issue = &art->issues[art->issue_count];
    memset(issue, 0, sizeof(lst_issue_t));

    snprintf(issue->id, sizeof(issue->id), "LSA-%04u", art->issue_count + 1);
    issue->severity = severity;
    snprintf(issue->title, sizeof(issue->title), "%s", title);
    snprintf(issue->description, sizeof(issue->description), "%s", desc);
    if (file_path)
        snprintf(issue->file_path, sizeof(issue->file_path), "%s", file_path);
    issue->line_number = line;
    if (req_id)
        snprintf(issue->cwe, sizeof(issue->cwe), "%s", req_id);
    snprintf(issue->remediation, sizeof(issue->remediation),
        "See LSA-SPEC-002 v2.0.0");

    art->issue_count++;
}

/* --------------------------------------------------------------------------
 * Anti-Pattern Detection (Section 2.5)
 * -------------------------------------------------------------------------- */

/* Detect DB hosting on edge nodes (Orin) */
static void check_antipattern_db_on_edge(lst_artifact_t *art,
                                          const char *fpath, const char *line,
                                          int lineno) {
    /* Look for PostgreSQL/DB config combined with Orin/edge/EPN references */
    if ((strcasestr_lsa(line, "postgresql") || strcasestr_lsa(line, "postgres") ||
         strcasestr_lsa(line, "database_url")) &&
        (strcasestr_lsa(line, "orin") || strcasestr_lsa(line, "epn") ||
         strcasestr_lsa(line, "jetson"))) {
        add_lsa_issue(art, SEV_CRITICAL,
            "Anti-Pattern: DB on Edge Node",
            "Database hosting detected on Orin/EPN -- violates Section 2.5",
            fpath, (uint32_t)lineno, "NFR-7.3.2");
    }
}

/* Detect CUDA workloads targeted at M1 */
static void check_antipattern_cuda_on_m1(lst_artifact_t *art,
                                          const char *fpath, const char *line,
                                          int lineno) {
    if ((strcasestr_lsa(line, "cuda") || strcasestr_lsa(line, "tensorrt")) &&
        (strcasestr_lsa(line, "m1") || strcasestr_lsa(line, "hcn") ||
         strcasestr_lsa(line, "metal"))) {
        add_lsa_issue(art, SEV_ERROR,
            "Anti-Pattern: CUDA on M1",
            "CUDA workload targeted at M1/HCN -- use Orin/EPN instead",
            fpath, (uint32_t)lineno, "SEC-2.5");
    }
}

/* Detect ML/inference on NUC */
static void check_antipattern_ml_on_nuc(lst_artifact_t *art,
                                         const char *fpath, const char *line,
                                         int lineno) {
    if ((strcasestr_lsa(line, "torch") || strcasestr_lsa(line, "tensorflow") ||
         strcasestr_lsa(line, "inference") || strcasestr_lsa(line, "model.predict")) &&
        (strcasestr_lsa(line, "nuc") || strcasestr_lsa(line, "dcn"))) {
        add_lsa_issue(art, SEV_ERROR,
            "Anti-Pattern: ML on NUC",
            "ML inference targeted at NUC/DCN -- NUC is deterministic control only",
            fpath, (uint32_t)lineno, "SEC-2.5");
    }
}

/* --------------------------------------------------------------------------
 * Security NFR Checks (Section 7.3)
 * -------------------------------------------------------------------------- */

/* Detect secrets stored on non-NUC nodes */
static void check_secret_placement(lst_artifact_t *art,
                                    const char *fpath, const char *line,
                                    int lineno) {
    if ((strcasestr_lsa(line, "api_key") || strcasestr_lsa(line, "api_secret") ||
         strcasestr_lsa(line, "private_key") || strcasestr_lsa(line, "exchange_key")) &&
        (strcasestr_lsa(line, "orin") || strcasestr_lsa(line, "epn") ||
         strcasestr_lsa(line, "jetson"))) {
        add_lsa_issue(art, SEV_CRITICAL,
            "Secrets on Edge Node",
            "Private keys/API secrets must not be stored on Orin -- NFR-7.3.4",
            fpath, (uint32_t)lineno, "NFR-7.3.4");
    }
}

/* Verify encryption references use AES-256-GCM for inter-node traffic */
static void check_encryption_standard(lst_artifact_t *art,
                                       const char *fpath, const char *line,
                                       int lineno) {
    /* If doing sync/multicast encryption, must be AES-256-GCM */
    if ((strcasestr_lsa(line, "ethernet_sync") || strcasestr_lsa(line, "multicast") ||
         strcasestr_lsa(line, "sync_encrypt")) &&
        (strcasestr_lsa(line, "aes-128") || strcasestr_lsa(line, "aes_128") ||
         strcasestr_lsa(line, "des") || strcasestr_lsa(line, "rc4") ||
         strcasestr_lsa(line, "chacha"))) {
        add_lsa_issue(art, SEV_ERROR,
            "Non-Compliant Encryption",
            "Inter-node traffic must use AES-256-GCM per NFR-7.3.1",
            fpath, (uint32_t)lineno, "NFR-7.3.1");
    }
}

/* --------------------------------------------------------------------------
 * Breathing Room Validation (Section 5)
 * -------------------------------------------------------------------------- */

static void check_breathing_room(lst_artifact_t *art,
                                  const char *fpath, const char *line,
                                  int lineno) {
    /* Check for phi_inverse / complementarity constants */
    if (strstr(line, "phi_inverse") || strstr(line, "PHI_INVERSE")) {
        /* Verify the value is correct */
        const char *eq = strstr(line, "=");
        if (eq) {
            double val = atof(eq + 1);
            if (val > 0 && fabs(val - PHI_INVERSE) > 0.001) {
                char desc[LST_MAX_NAME];
                snprintf(desc, sizeof(desc),
                    "phi_inverse should be 0.618034, found %.6f", val);
                add_lsa_issue(art, SEV_ERROR,
                    "Incorrect Phi Inverse Constant",
                    desc, fpath, (uint32_t)lineno, "SEC-5");
            }
        }
    }

    if (strstr(line, "breathing_room") || strstr(line, "BREATHING_ROOM")) {
        const char *eq = strstr(line, "=");
        if (eq) {
            double val = atof(eq + 1);
            if (val > 0 && fabs(val - BREATHING_ROOM_THRESH) > 0.01) {
                char desc[LST_MAX_NAME];
                snprintf(desc, sizeof(desc),
                    "breathing_room threshold should be 0.382, found %.3f", val);
                add_lsa_issue(art, SEV_WARNING,
                    "Non-Standard Breathing Room Threshold",
                    desc, fpath, (uint32_t)lineno, "SEC-5");
            }
        }
    }
}

/* --------------------------------------------------------------------------
 * Workload Routing Validation (Section 2.3)
 * -------------------------------------------------------------------------- */

static void check_workload_routing(lst_artifact_t *art,
                                    const char *fpath, const char *line,
                                    int lineno) {
    /* Detect E3 workloads not routed to Orin */
    if ((strcasestr_lsa(line, "sensor") || strcasestr_lsa(line, "vision_pipeline") ||
         strcasestr_lsa(line, "edge_inference") || strcasestr_lsa(line, "4hz") ||
         strcasestr_lsa(line, "real_time")) &&
        (strcasestr_lsa(line, "route") || strcasestr_lsa(line, "deploy") ||
         strcasestr_lsa(line, "target")) &&
        (strcasestr_lsa(line, "nuc") || strcasestr_lsa(line, "m1"))) {
        add_lsa_issue(art, SEV_WARNING,
            "E3 Workload Misrouted",
            "Real-time reactive workloads (E3) should route to Orin/EPN",
            fpath, (uint32_t)lineno, "SEC-2.3");
    }
}

/* --------------------------------------------------------------------------
 * Daemon Configuration Check
 * -------------------------------------------------------------------------- */

/* Check if a daemon name appears in a node's inventory */
static int daemon_belongs(const char *daemon, const char **inventory) {
    for (int i = 0; inventory[i]; i++)
        if (strcmp(daemon, inventory[i]) == 0) return 1;
    return 0;
}

static void check_daemon_config(lst_artifact_t *art,
                                 const char *fpath, const char *content) {
    if (!strstr(fpath, "daemon") && !strstr(fpath, "service") &&
        !strstr(fpath, "systemd") && !strstr(fpath, "lsa_boot") &&
        !strstr(fpath, "config"))
        return;

    /* For each DCN daemon, flag if deployed on wrong node */
    for (int i = 0; DCN_DAEMONS[i]; i++) {
        if (!daemon_belongs(DCN_DAEMONS[i], HCN_DAEMONS) &&
            !daemon_belongs(DCN_DAEMONS[i], EPN_DAEMONS) &&
            strcasestr_lsa(content, DCN_DAEMONS[i])) {
            if (strcasestr_lsa(content, "orin") || strcasestr_lsa(content, "epn") ||
                strcasestr_lsa(content, "jetson")) {
                char desc[LST_MAX_NAME];
                snprintf(desc, sizeof(desc),
                    "%s is DCN-only -- must run on NUC, not Orin/EPN", DCN_DAEMONS[i]);
                add_lsa_issue(art, SEV_ERROR,
                    "Daemon Misplacement", desc, fpath, 0, "SEC-8.3");
            }
        }
    }

    /* For each HCN daemon, flag if deployed on wrong node */
    for (int i = 0; HCN_DAEMONS[i]; i++) {
        if (!daemon_belongs(HCN_DAEMONS[i], DCN_DAEMONS) &&
            !daemon_belongs(HCN_DAEMONS[i], EPN_DAEMONS) &&
            strcasestr_lsa(content, HCN_DAEMONS[i])) {
            if (strcasestr_lsa(content, "orin") || strcasestr_lsa(content, "nuc") ||
                strcasestr_lsa(content, "dcn")) {
                char desc[LST_MAX_NAME];
                snprintf(desc, sizeof(desc),
                    "%s is HCN-only -- must run on M1", HCN_DAEMONS[i]);
                add_lsa_issue(art, SEV_ERROR,
                    "Daemon Misplacement", desc, fpath, 0, "SEC-8.2");
            }
        }
    }

    /* For each EPN daemon, flag if deployed on wrong node */
    for (int i = 0; EPN_DAEMONS[i]; i++) {
        if (!daemon_belongs(EPN_DAEMONS[i], DCN_DAEMONS) &&
            !daemon_belongs(EPN_DAEMONS[i], HCN_DAEMONS) &&
            strcasestr_lsa(content, EPN_DAEMONS[i])) {
            if (strcasestr_lsa(content, "nuc") || strcasestr_lsa(content, "dcn") ||
                strcasestr_lsa(content, "m1")) {
                char desc[LST_MAX_NAME];
                snprintf(desc, sizeof(desc),
                    "%s is EPN-only -- must run on Orin", EPN_DAEMONS[i]);
                add_lsa_issue(art, SEV_ERROR,
                    "Daemon Misplacement", desc, fpath, 0, "SEC-8.1");
            }
        }
    }
}

/* --------------------------------------------------------------------------
 * Hybrid Sync Validation (FR-6.10 + Tailscale fallback)
 *
 * Policy: Use LAN multicast (239.73.78.69:20046) when peers are local.
 *         Fall back to unicast over Tailscale (100.x.x.x) only when
 *         the device is not on the local network.
 *         Orin should stay local (E3 latency: <50ms intervention tier).
 * -------------------------------------------------------------------------- */

static void check_hybrid_sync(lst_artifact_t *art, const char *fpath,
                               const char *line, int lineno) {
    /* Flag hardcoded unicast-only sync (should be hybrid) */
    if ((strcasestr_lsa(line, "ethernet_sync") || strcasestr_lsa(line, "sync_peer") ||
         strcasestr_lsa(line, "sync_target")) &&
        (strstr(line, "100.") || strcasestr_lsa(line, "tailscale"))) {
        if (!strcasestr_lsa(line, "fallback") && !strcasestr_lsa(line, "remote") &&
            !strcasestr_lsa(line, "hybrid") && !strcasestr_lsa(line, "lan_detect")) {
            add_lsa_issue(art, SEV_WARNING,
                "Sync Should Be Hybrid",
                "Tailscale sync should only activate when peer is off-LAN; use multicast on local network",
                fpath, (uint32_t)lineno, "FR-6.10");
        }
    }

    /* Flag multicast-only sync (no Tailscale fallback) */
    if (strcasestr_lsa(line, "multicast") && strcasestr_lsa(line, "only")) {
        if (strcasestr_lsa(line, "sync") || strcasestr_lsa(line, "ethernet")) {
            add_lsa_issue(art, SEV_INFO,
                "No Remote Sync Fallback",
                "Multicast-only sync has no Tailscale fallback for remote peers",
                fpath, (uint32_t)lineno, "FR-6.10");
        }
    }

    /* Flag Orin being configured for remote/Tailscale sync (latency risk) */
    if ((strcasestr_lsa(line, "orin") || strcasestr_lsa(line, "epn") ||
         strcasestr_lsa(line, "state_monitor")) &&
        (strcasestr_lsa(line, "tailscale") || strcasestr_lsa(line, "remote") ||
         strstr(line, "100."))) {
        if (strcasestr_lsa(line, "sync") || strcasestr_lsa(line, "connect")) {
            add_lsa_issue(art, SEV_WARNING,
                "Orin Should Stay Local",
                "Orin/EPN handles E3 real-time workloads -- remote sync risks >50ms intervention latency",
                fpath, (uint32_t)lineno, "NFR-7.1.4");
        }
    }

    /* Verify sync key permissions if referenced */
    if (strstr(line, "sync.key") || strstr(line, "sync_key")) {
        if (strstr(line, "0644") || strstr(line, "0755") || strstr(line, "0666") ||
            strstr(line, "world") || strstr(line, "readable")) {
            add_lsa_issue(art, SEV_CRITICAL,
                "Sync Key Permissions Too Open",
                "sync.key must be 0600 per NFR-7.3.1",
                fpath, (uint32_t)lineno, "NFR-7.3.1");
        }
    }
}

/* --------------------------------------------------------------------------
 * State Ownership Validation (Section 4.1)
 * -------------------------------------------------------------------------- */

static void check_state_ownership(lst_artifact_t *art,
                                   const char *fpath, const char *line,
                                   int lineno) {
    /* NUC-owned fields being written on non-NUC */
    if ((strstr(line, "origin_vault") || strstr(line, "drift_score")) &&
        (strstr(line, "write") || strstr(line, "update") || strstr(line, "set")) &&
        (strcasestr_lsa(line, "orin") || strcasestr_lsa(line, "m1"))) {
        add_lsa_issue(art, SEV_ERROR,
            "State Ownership Violation",
            "NUC-owned fields (origin_vault, drift_score) must only be written by NUC",
            fpath, (uint32_t)lineno, "SEC-4.1");
    }

    /* Orin-owned fields being written on non-Orin */
    if ((strstr(line, "sensory_load") || strstr(line, "intervention_tier")) &&
        (strstr(line, "write") || strstr(line, "update") || strstr(line, "set")) &&
        (strcasestr_lsa(line, "nuc") || strcasestr_lsa(line, "m1"))) {
        add_lsa_issue(art, SEV_ERROR,
            "State Ownership Violation",
            "Orin-owned fields (sensory_load, intervention_tier) must only be written by Orin",
            fpath, (uint32_t)lineno, "SEC-4.1");
    }
}

/* --------------------------------------------------------------------------
 * File scanner
 * -------------------------------------------------------------------------- */

static const char *SKIP_DIRS_LSA[] = {
    "node_modules", "vendor", ".git", "__pycache__", "dist", "build",
    ".venv", "venv", ".next", "target", ".cache", NULL
};

static int should_skip_lsa(const char *name) {
    if (name[0] == '.') return 1;
    for (int i = 0; SKIP_DIRS_LSA[i]; i++)
        if (strcmp(name, SKIP_DIRS_LSA[i]) == 0) return 1;
    return 0;
}

static int is_lsa_scannable(const char *name) {
    const char *ext = strrchr(name, '.');
    if (!ext) return 0;
    const char *exts[] = {".c", ".cpp", ".h", ".hpp", ".py", ".js", ".ts",
                          ".yaml", ".yml", ".toml", ".ini", ".conf", ".cfg",
                          ".service", ".json", ".sh", ".go", ".rs", NULL};
    for (int i = 0; exts[i]; i++)
        if (strcasecmp(ext, exts[i]) == 0) return 1;
    return 0;
}

static void scan_file_lsa(lst_artifact_t *art, const char *fpath) {
    size_t len = 0;
    char *content = read_file_lsa(fpath, &len);
    if (!content) return;

    /* Full-file checks */
    check_daemon_config(art, fpath, content);

    /* Line-by-line checks */
    int lineno = 1;
    char *line_start = content;

    while (*line_start) {
        char *eol = strchr(line_start, '\n');
        if (eol) *eol = '\0';

        check_antipattern_db_on_edge(art, fpath, line_start, lineno);
        check_antipattern_cuda_on_m1(art, fpath, line_start, lineno);
        check_antipattern_ml_on_nuc(art, fpath, line_start, lineno);
        check_secret_placement(art, fpath, line_start, lineno);
        check_encryption_standard(art, fpath, line_start, lineno);
        check_breathing_room(art, fpath, line_start, lineno);
        check_workload_routing(art, fpath, line_start, lineno);
        check_state_ownership(art, fpath, line_start, lineno);
        check_hybrid_sync(art, fpath, line_start, lineno);

        if (!eol) break;
        line_start = eol + 1;
        lineno++;
    }

    free(content);
}

static void scan_dir_lsa(lst_artifact_t *art, const char *dir, int depth) {
    if (depth > 8) return;

    DIR *d = opendir(dir);
    if (!d) return;

    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        if (ent->d_name[0] == '.') continue;
        if (should_skip_lsa(ent->d_name)) continue;

        char child[LST_MAX_PATH];
        snprintf(child, sizeof(child), "%s/%s", dir, ent->d_name);

        struct stat st;
        if (stat(child, &st) != 0) continue;

        if (S_ISDIR(st.st_mode)) {
            scan_dir_lsa(art, child, depth + 1);
        } else if (S_ISREG(st.st_mode) && is_lsa_scannable(ent->d_name)) {
            scan_file_lsa(art, child);
        }
    }
    closedir(d);
}

/* --------------------------------------------------------------------------
 * Report writer
 * -------------------------------------------------------------------------- */

static void write_sep_lsa(FILE *f) {
    for (int i = 0; i < 80; i++) fputc('=', f);
    fputc('\n', f);
}

static void write_line_lsa(FILE *f) {
    for (int i = 0; i < 40; i++) fputc('-', f);
    fputc('\n', f);
}

static int recipe_lsa_compliance(lst_artifact_t *art, const char *output_dir) {
    if (!art) return -1;

    uint32_t initial_issues = art->issue_count;
    scan_dir_lsa(art, art->project_path, 0);
    uint32_t new_issues = art->issue_count - initial_issues;

    /* Build output path */
    char outpath[LST_MAX_PATH];
    if (output_dir)
        snprintf(outpath, sizeof(outpath), "%s/LSA_COMPLIANCE_REPORT", output_dir);
    else
        snprintf(outpath, sizeof(outpath), "%s/LSA_COMPLIANCE_REPORT", art->project_path);

    if (output_dir) mkdir(output_dir, 0755);

    FILE *f = lst_secure_fopen(outpath, "w");
    if (!f) {
        fprintf(stderr, "lsa-compliance: cannot write %s\n", outpath);
        return -1;
    }

    /* Header */
    write_sep_lsa(f);
    fprintf(f, "LSA SPEC COMPLIANCE REPORT\n");
    fprintf(f, "Reference: LSA-SPEC-002 v2.0.0 (TP-HCF)\n");
    fprintf(f, "Project: %s\n", art->project_name);
    fprintf(f, "Path: %s\n", art->project_path);

    time_t now = time(NULL);
    struct tm tm_buf;
    struct tm *t = gmtime_r(&now, &tm_buf);
    char ts[64];
    strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S UTC", t);
    fprintf(f, "Generated: %s\n", ts);
    fprintf(f, "Violations Found: %u\n", new_issues);
    write_sep_lsa(f);
    fprintf(f, "\n");

    /* Architecture reference */
    fprintf(f, "ARCHITECTURE REFERENCE\n");
    write_line_lsa(f);
    fprintf(f, "  Execution Classes:\n");
    fprintf(f, "    E1 Deterministic Control -> NUC (DCN)\n");
    fprintf(f, "    E2 Parallel Numerical    -> M1 (HCN) / Orin (EPN)\n");
    fprintf(f, "    E3 Real-Time Reactive    -> Orin (EPN)\n");
    fprintf(f, "\n");
    fprintf(f, "  Core Constants:\n");
    fprintf(f, "    phi_inverse            = 0.618034\n");
    fprintf(f, "    complementarity_zone   = 0.382\n");
    fprintf(f, "    breathing_room_thresh  = 0.382\n");
    fprintf(f, "\n");

    /* Findings */
    if (new_issues > 0) {
        fprintf(f, "VIOLATIONS\n");
        write_sep_lsa(f);
        fprintf(f, "\n");

        /* Group by severity */
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
                    fprintf(f, "      Ref: %s\n", issue->cwe);
                fprintf(f, "      %s\n\n", issue->description);
            }
        }
    } else {
        fprintf(f, "COMPLIANCE: PASS\n");
        fprintf(f, "  No LSA spec violations found.\n\n");
    }

    /* Anti-pattern checklist */
    fprintf(f, "ANTI-PATTERN CHECKLIST (Section 2.5)\n");
    write_line_lsa(f);
    int ap_db = 0, ap_cuda = 0, ap_ml = 0, ap_secrets = 0;
    for (uint32_t i = initial_issues; i < art->issue_count; i++) {
        if (strstr(art->issues[i].title, "DB on Edge")) ap_db = 1;
        if (strstr(art->issues[i].title, "CUDA on M1")) ap_cuda = 1;
        if (strstr(art->issues[i].title, "ML on NUC")) ap_ml = 1;
        if (strstr(art->issues[i].title, "Secrets on Edge")) ap_secrets = 1;
    }
    fprintf(f, "  [%s] No DB hosting on Orin\n", ap_db ? "FAIL" : "PASS");
    fprintf(f, "  [%s] No CUDA jobs on M1\n", ap_cuda ? "FAIL" : "PASS");
    fprintf(f, "  [%s] No ML inference on NUC\n", ap_ml ? "FAIL" : "PASS");
    fprintf(f, "  [%s] No secrets on edge nodes\n", ap_secrets ? "FAIL" : "PASS");
    fprintf(f, "\n");

    /* Hybrid sync policy */
    fprintf(f, "SYNC POLICY (FR-6.10 + Tailscale Hybrid)\n");
    write_line_lsa(f);
    fprintf(f, "  LAN:       UDP multicast 239.73.78.69:20046 (preferred)\n");
    fprintf(f, "  Remote:    Unicast UDP over Tailscale (100.x.x.x) fallback\n");
    fprintf(f, "  Policy:    Tailscale only when peer is not on local network\n");
    fprintf(f, "  Orin/EPN:  Must stay local (E3 latency: <50ms intervention)\n");
    fprintf(f, "  M1/HCN:   May roam (E2 workloads are batch-tolerant)\n");
    fprintf(f, "  NUC/DCN:  Stationary (source of truth, TCP:20047 for mobile)\n");
    fprintf(f, "  Encryption: AES-256-GCM, key at /etc/lsa/sync.key (0600)\n");
    fprintf(f, "\n");
    int has_sync = 0, has_orin_remote = 0;
    for (uint32_t i = initial_issues; i < art->issue_count; i++) {
        if (strstr(art->issues[i].title, "Hybrid")) has_sync = 1;
        if (strstr(art->issues[i].title, "Orin Should Stay")) has_orin_remote = 1;
    }
    fprintf(f, "  [%s] Hybrid sync (multicast + Tailscale fallback)\n",
        has_sync ? "REVIEW" : "PASS");
    fprintf(f, "  [%s] Orin stays on local network\n",
        has_orin_remote ? "FAIL" : "PASS");
    fprintf(f, "\n");

    /* Footer */
    write_sep_lsa(f);
    fprintf(f, "END OF LSA COMPLIANCE REPORT\n");
    fprintf(f, "\nValidated against: Jason's Living System Architecture\n");
    fprintf(f, "Tri-Plane Heterogeneous Compute Fabric (TP-HCF)\n");
    fprintf(f, "Document: LSA-SPEC-002 v2.0.0 (2026-02-22)\n");
    write_sep_lsa(f);
    fprintf(f, "\n");

    fclose(f);

    printf("  Wrote %s (%u violations)\n", outpath, new_issues);
    return 0;
}

/* --------------------------------------------------------------------------
 * Recipe registration
 * -------------------------------------------------------------------------- */

void recipe_lsa_compliance_register(void) {
    lst_recipe_t r = {0};
    snprintf(r.name, LST_MAX_NAME, "lsa-compliance");
    snprintf(r.description, LST_MAX_NAME,
        "Validate against LSA-SPEC-002 TP-HCF architecture");
    r.execute = recipe_lsa_compliance;
    r.version = 1;
    lst_recipe_register(&r);
}
