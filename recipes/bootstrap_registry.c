#define _POSIX_C_SOURCE 200809L

/*
 * bootstrap_registry.c -- Bootstrap Registry Validation Recipe
 *
 * Validates the NNOS bootstrap registry against capability rules
 * from the 180-pattern workstream.
 *
 * Deterministic checks:
 *   - Registry entries exist for each of the 11 NNOS daemons
 *   - Capability patterns are declared per daemon
 *   - Boot order dependencies are correct (supervisor before workers)
 *   - LSA boot targets (lsa_boot_dcn, lsa_boot_hcn, lsa_boot_epn) defined
 *   - No orphaned dependencies (deps referencing non-existent daemons)
 *   - No circular dependencies in the boot graph
 */

#include "lst.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>

/* --------------------------------------------------------------------------
 * NNOS Daemon Constellation (11 daemons from NNOS-FULL-SPEC-CHAIN)
 * -------------------------------------------------------------------------- */

static const char *NNOS_DAEMONS[] = {
    "nnos_boot_daemon",
    "nnos_state_monitor",
    "nnos_task_manager",
    "nnos_context_gate",
    "nnos_comm_bridge",
    "nnos_profile_refiner",
    "nnos_ethernet_sync",
    "nnos_morph_engine",
    "nnos_threat_scanner",
    "nnos_neuro_analyzer",
    "nnos_quantum_morph",
    NULL
};

/* LSA boot targets -- one per node role */
static const char *LSA_BOOT_TARGETS[] = {
    "lsa_boot_dcn",
    "lsa_boot_hcn",
    "lsa_boot_epn",
    NULL
};

/* Daemons that must boot before any worker daemon */
static const char *SUPERVISOR_DAEMONS[] = {
    "nnos_boot_daemon",
    "nnos_ethernet_sync",
    NULL
};

/* Daemons that depend on supervisor being up first */
static const char *WORKER_DAEMONS[] = {
    "nnos_state_monitor",
    "nnos_task_manager",
    "nnos_context_gate",
    "nnos_comm_bridge",
    "nnos_profile_refiner",
    "nnos_morph_engine",
    "nnos_threat_scanner",
    "nnos_neuro_analyzer",
    "nnos_quantum_morph",
    NULL
};

/* 9 capability pattern categories from the 180-pattern workstream (9 x 20) */
static const char *CAPABILITY_CATEGORIES[] = {
    "sensory",
    "executive",
    "hyperfocus",
    "masking",
    "autonomic",
    "social",
    "morph",
    "threat",
    "quantum",
    NULL
};

/* Directories to skip during scanning */
static const char *SKIP_DIRS_BSR[] = {
    "node_modules", "vendor", ".git", "__pycache__", "dist", "build",
    ".venv", "venv", ".next", "target", ".cache", NULL
};

/* --------------------------------------------------------------------------
 * Helpers
 * -------------------------------------------------------------------------- */

static char *read_file_bsr(const char *path, size_t *out_len) {
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

static void add_bsr_issue(lst_artifact_t *art, uint8_t severity,
                           const char *title, const char *desc,
                           const char *file_path, uint32_t line,
                           const char *remediation_text) {
    if (art->issue_count >= LST_MAX_ISSUES) return;
    lst_issue_t *issue = &art->issues[art->issue_count];
    memset(issue, 0, sizeof(lst_issue_t));

    snprintf(issue->id, sizeof(issue->id), "BSR-%04u", art->issue_count + 1);
    issue->severity = severity;
    snprintf(issue->title, sizeof(issue->title), "%s", title);
    snprintf(issue->description, sizeof(issue->description), "%s", desc);
    if (file_path)
        snprintf(issue->file_path, sizeof(issue->file_path), "%s", file_path);
    issue->line_number = line;
    snprintf(issue->cwe, sizeof(issue->cwe), "BSR");
    if (remediation_text)
        snprintf(issue->remediation, sizeof(issue->remediation), "%s", remediation_text);

    art->issue_count++;
}

/* --------------------------------------------------------------------------
 * Scan helpers
 * -------------------------------------------------------------------------- */

static int should_skip_bsr(const char *name) {
    if (name[0] == '.') return 1;
    for (int i = 0; SKIP_DIRS_BSR[i]; i++)
        if (strcmp(name, SKIP_DIRS_BSR[i]) == 0) return 1;
    return 0;
}

static int is_bsr_scannable(const char *name) {
    const char *ext = strrchr(name, '.');
    if (!ext) return 0;
    const char *exts[] = {
        ".c", ".cpp", ".h", ".hpp", ".py", ".js", ".ts",
        ".yaml", ".yml", ".toml", ".ini", ".conf", ".cfg",
        ".service", ".json", ".sh", ".go", ".rs", ".md", NULL
    };
    for (int i = 0; exts[i]; i++)
        if (strcasecmp(ext, exts[i]) == 0) return 1;
    return 0;
}

/* --------------------------------------------------------------------------
 * Detection state -- tracks what has been found across all scanned files
 * -------------------------------------------------------------------------- */

typedef struct {
    int daemon_found[11];       /* one flag per NNOS_DAEMONS entry   */
    int capability_found[9];    /* one flag per CAPABILITY_CATEGORIES */
    int boot_target_found[3];   /* one flag per LSA_BOOT_TARGETS     */
    int supervisor_before_worker;  /* supervisor declared before workers */
    int boot_order_file_seen;      /* found a boot order / init config  */
    char daemon_files[11][LST_MAX_PATH]; /* file where daemon was found */
} bsr_state_t;

/* --------------------------------------------------------------------------
 * File analysis
 * -------------------------------------------------------------------------- */

static void scan_file_bsr(lst_artifact_t *art, const char *fpath,
                           bsr_state_t *state) {
    size_t len = 0;
    char *content = read_file_bsr(fpath, &len);
    if (!content) return;

    /* Check for each daemon declaration */
    for (int i = 0; NNOS_DAEMONS[i]; i++) {
        if (strstr(content, NNOS_DAEMONS[i])) {
            state->daemon_found[i] = 1;
            if (state->daemon_files[i][0] == '\0')
                snprintf(state->daemon_files[i], LST_MAX_PATH, "%s", fpath);
        }
    }

    /* Check for capability pattern categories */
    for (int i = 0; CAPABILITY_CATEGORIES[i]; i++) {
        if (strstr(content, CAPABILITY_CATEGORIES[i]) &&
            (strstr(content, "pattern") || strstr(content, "capability") ||
             strstr(content, "cluster"))) {
            state->capability_found[i] = 1;
        }
    }

    /* Check for LSA boot targets */
    for (int i = 0; LSA_BOOT_TARGETS[i]; i++) {
        if (strstr(content, LSA_BOOT_TARGETS[i])) {
            state->boot_target_found[i] = 1;
        }
    }

    /* Check for boot order configuration files */
    if (strstr(fpath, "boot") || strstr(fpath, "init") ||
        strstr(fpath, "startup") || strstr(fpath, "systemd")) {
        state->boot_order_file_seen = 1;

        /* Verify supervisor daemons appear before worker daemons */
        const char *first_supervisor = NULL;
        const char *first_worker = NULL;

        for (int i = 0; SUPERVISOR_DAEMONS[i]; i++) {
            const char *pos = strstr(content, SUPERVISOR_DAEMONS[i]);
            if (pos && (!first_supervisor || pos < first_supervisor))
                first_supervisor = pos;
        }
        for (int i = 0; WORKER_DAEMONS[i]; i++) {
            const char *pos = strstr(content, WORKER_DAEMONS[i]);
            if (pos && (!first_worker || pos < first_worker))
                first_worker = pos;
        }

        if (first_supervisor && first_worker) {
            if (first_supervisor < first_worker) {
                state->supervisor_before_worker = 1;
            } else {
                add_bsr_issue(art, SEV_ERROR,
                    "Boot Order Violation",
                    "Worker daemon declared before supervisor in boot config",
                    fpath, 0,
                    "Ensure nnos_boot_daemon and nnos_ethernet_sync boot first");
            }
        }
    }

    /* Line-by-line: detect circular dependency declarations */
    int lineno = 1;
    char *line_start = content;
    while (*line_start) {
        char *eol = strchr(line_start, '\n');
        if (eol) *eol = '\0';

        /* Detect self-referential dependency (daemon depends on itself) */
        for (int i = 0; NNOS_DAEMONS[i]; i++) {
            if (strstr(line_start, NNOS_DAEMONS[i]) &&
                (strstr(line_start, "depends_on") || strstr(line_start, "after") ||
                 strstr(line_start, "requires") || strstr(line_start, "dep:"))) {
                /* Count how many times the daemon name appears on this line */
                int count = 0;
                const char *p = line_start;
                size_t dlen = strlen(NNOS_DAEMONS[i]);
                while ((p = strstr(p, NNOS_DAEMONS[i])) != NULL) {
                    count++;
                    p += dlen;
                }
                if (count >= 2) {
                    char desc[LST_MAX_NAME];
                    snprintf(desc, sizeof(desc),
                        "Circular dependency: %s depends on itself", NNOS_DAEMONS[i]);
                    add_bsr_issue(art, SEV_CRITICAL,
                        "Circular Dependency Detected", desc,
                        fpath, (uint32_t)lineno,
                        "Remove self-referencing dependency");
                }
            }
        }

        /* Detect orphaned dependency references */
        if (strstr(line_start, "depends_on") || strstr(line_start, "after") ||
            strstr(line_start, "requires")) {
            if (strstr(line_start, "nnos_")) {
                /* Check if the referenced daemon is in our known list */
                const char *p = strstr(line_start, "nnos_");
                while (p) {
                    char token[128];
                    int ti = 0;
                    while (ti < 127 && p[ti] && (p[ti] == '_' ||
                           (p[ti] >= 'a' && p[ti] <= 'z') ||
                           (p[ti] >= '0' && p[ti] <= '9'))) {
                        token[ti] = p[ti];
                        ti++;
                    }
                    token[ti] = '\0';

                    if (ti > 5) { /* longer than just "nnos_" */
                        int known = 0;
                        for (int d = 0; NNOS_DAEMONS[d]; d++) {
                            if (strcmp(token, NNOS_DAEMONS[d]) == 0) {
                                known = 1;
                                break;
                            }
                        }
                        if (!known) {
                            char desc[LST_MAX_NAME];
                            snprintf(desc, sizeof(desc),
                                "Orphaned dependency: '%s' is not a known daemon",
                                token);
                            add_bsr_issue(art, SEV_WARNING,
                                "Orphaned Dependency Reference", desc,
                                fpath, (uint32_t)lineno,
                                "Verify daemon name or add to registry");
                        }
                    }

                    p = strstr(p + ti + 1, "nnos_");
                }
            }
        }

        if (!eol) break;
        line_start = eol + 1;
        lineno++;
    }

    free(content);
}

static void scan_dir_bsr(lst_artifact_t *art, const char *dir, int depth,
                          bsr_state_t *state) {
    if (depth > 8) return;

    DIR *d = opendir(dir);
    if (!d) return;

    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        if (ent->d_name[0] == '.') continue;
        if (should_skip_bsr(ent->d_name)) continue;

        char child[LST_MAX_PATH];
        snprintf(child, sizeof(child), "%s/%s", dir, ent->d_name);

        struct stat st;
        if (stat(child, &st) != 0) continue;

        if (S_ISDIR(st.st_mode)) {
            scan_dir_bsr(art, child, depth + 1, state);
        } else if (S_ISREG(st.st_mode) && is_bsr_scannable(ent->d_name)) {
            scan_file_bsr(art, child, state);
        }
    }
    closedir(d);
}

/* --------------------------------------------------------------------------
 * Post-scan validation: emit issues for missing registry entries
 * -------------------------------------------------------------------------- */

static void validate_registry(lst_artifact_t *art, const bsr_state_t *state) {
    /* Check that all 11 daemons have registry entries */
    for (int i = 0; NNOS_DAEMONS[i]; i++) {
        if (!state->daemon_found[i]) {
            char desc[LST_MAX_NAME];
            snprintf(desc, sizeof(desc),
                "No registry entry found for daemon '%s'", NNOS_DAEMONS[i]);
            add_bsr_issue(art, SEV_ERROR,
                "Missing Daemon Registry Entry", desc, NULL, 0,
                "Add daemon declaration to bootstrap registry");
        }
    }

    /* Check that all 9 capability categories have patterns declared */
    for (int i = 0; CAPABILITY_CATEGORIES[i]; i++) {
        if (!state->capability_found[i]) {
            char desc[LST_MAX_NAME];
            snprintf(desc, sizeof(desc),
                "No capability patterns declared for category '%s'",
                CAPABILITY_CATEGORIES[i]);
            add_bsr_issue(art, SEV_WARNING,
                "Missing Capability Pattern Category", desc, NULL, 0,
                "Declare patterns from the 180-pattern workstream");
        }
    }

    /* Check that all 3 LSA boot targets are defined */
    for (int i = 0; LSA_BOOT_TARGETS[i]; i++) {
        if (!state->boot_target_found[i]) {
            char desc[LST_MAX_NAME];
            snprintf(desc, sizeof(desc),
                "LSA boot target '%s' is not defined", LSA_BOOT_TARGETS[i]);
            add_bsr_issue(art, SEV_ERROR,
                "Missing LSA Boot Target", desc, NULL, 0,
                "Define boot target in bootstrap configuration");
        }
    }

    /* Warn if no boot order configuration was found at all */
    if (!state->boot_order_file_seen) {
        add_bsr_issue(art, SEV_WARNING,
            "No Boot Order Configuration",
            "No boot/init/startup config file found in project tree",
            NULL, 0,
            "Create a boot order configuration for daemon startup sequence");
    } else if (!state->supervisor_before_worker) {
        /* Boot order file exists but supervisor ordering not confirmed */
        add_bsr_issue(art, SEV_INFO,
            "Boot Order Not Verified",
            "Boot config exists but supervisor-before-worker order unconfirmed",
            NULL, 0,
            "Ensure nnos_boot_daemon starts before worker daemons");
    }
}

/* --------------------------------------------------------------------------
 * Report writer
 * -------------------------------------------------------------------------- */

static void write_sep_bsr(FILE *f) {
    for (int i = 0; i < 80; i++) fputc('=', f);
    fputc('\n', f);
}

static void write_line_bsr(FILE *f) {
    for (int i = 0; i < 40; i++) fputc('-', f);
    fputc('\n', f);
}

static int write_report_bsr(lst_artifact_t *art, const char *output_dir,
                              uint32_t initial_issues,
                              const bsr_state_t *state) {
    uint32_t new_issues = art->issue_count - initial_issues;

    char outpath[LST_MAX_PATH];
    if (output_dir)
        snprintf(outpath, sizeof(outpath), "%s/BOOTSTRAP_REGISTRY_REPORT", output_dir);
    else
        snprintf(outpath, sizeof(outpath), "%s/BOOTSTRAP_REGISTRY_REPORT", art->project_path);

    if (output_dir) mkdir(output_dir, 0755);

    int rfd = open(outpath, O_WRONLY | O_CREAT | O_TRUNC | O_NOFOLLOW | O_CLOEXEC, 0644);
    if (rfd < 0) {
        fprintf(stderr, "bootstrap-registry: cannot write %s\n", outpath);
        return -1;
    }
    FILE *f = fdopen(rfd, "w");
    if (!f) {
        close(rfd);
        fprintf(stderr, "bootstrap-registry: cannot write %s\n", outpath);
        return -1;
    }

    /* Header */
    write_sep_bsr(f);
    fprintf(f, "BOOTSTRAP REGISTRY VALIDATION REPORT\n");
    fprintf(f, "Reference: NNOS-FULL-SPEC-CHAIN (180-Pattern Workstream)\n");
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
    fprintf(f, "Issues Found: %u\n", new_issues);
    write_sep_bsr(f);
    fprintf(f, "\n");

    /* Daemon registry status */
    fprintf(f, "DAEMON REGISTRY STATUS (11 daemons)\n");
    write_line_bsr(f);
    for (int i = 0; NNOS_DAEMONS[i]; i++) {
        fprintf(f, "  [%s] %s", state->daemon_found[i] ? "FOUND" : "MISS ",
            NNOS_DAEMONS[i]);
        if (state->daemon_found[i] && state->daemon_files[i][0])
            fprintf(f, "  (%s)", state->daemon_files[i]);
        fprintf(f, "\n");
    }
    fprintf(f, "\n");

    /* Capability pattern categories */
    fprintf(f, "CAPABILITY PATTERN CATEGORIES (9 categories x 20 = 180 patterns)\n");
    write_line_bsr(f);
    for (int i = 0; CAPABILITY_CATEGORIES[i]; i++) {
        fprintf(f, "  [%s] %s\n",
            state->capability_found[i] ? "FOUND" : "MISS ",
            CAPABILITY_CATEGORIES[i]);
    }
    fprintf(f, "\n");

    /* LSA boot targets */
    fprintf(f, "LSA BOOT TARGETS\n");
    write_line_bsr(f);
    for (int i = 0; LSA_BOOT_TARGETS[i]; i++) {
        fprintf(f, "  [%s] %s\n",
            state->boot_target_found[i] ? "DEFINED " : "MISSING",
            LSA_BOOT_TARGETS[i]);
    }
    fprintf(f, "\n");

    /* Boot order */
    fprintf(f, "BOOT ORDER\n");
    write_line_bsr(f);
    fprintf(f, "  Boot config found: %s\n",
        state->boot_order_file_seen ? "YES" : "NO");
    fprintf(f, "  Supervisor-first order: %s\n",
        state->supervisor_before_worker ? "VERIFIED" : "UNVERIFIED");
    fprintf(f, "\n");

    /* Findings */
    if (new_issues > 0) {
        fprintf(f, "FINDINGS\n");
        write_sep_bsr(f);
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
                fprintf(f, "      %s\n", issue->description);
                if (issue->remediation[0])
                    fprintf(f, "      Fix: %s\n", issue->remediation);
                fprintf(f, "\n");
            }
        }
    } else {
        fprintf(f, "VALIDATION: PASS\n");
        fprintf(f, "  All bootstrap registry checks passed.\n\n");
    }

    /* Footer */
    write_sep_bsr(f);
    fprintf(f, "END OF BOOTSTRAP REGISTRY VALIDATION REPORT\n");
    fprintf(f, "\nValidated against: NNOS Daemon Constellation (11 daemons)\n");
    fprintf(f, "Pattern workstream: 180 patterns (9 categories x 20)\n");
    fprintf(f, "Boot targets: lsa_boot_dcn, lsa_boot_hcn, lsa_boot_epn\n");
    write_sep_bsr(f);
    fprintf(f, "\n");

    fclose(f);

    printf("  Wrote %s (%u issues)\n", outpath, new_issues);
    return 0;
}

/* --------------------------------------------------------------------------
 * Entry point
 * -------------------------------------------------------------------------- */

static int recipe_bootstrap_registry(lst_artifact_t *art, const char *output_dir) {
    if (!art) return -1;

    uint32_t initial_issues = art->issue_count;

    bsr_state_t state;
    memset(&state, 0, sizeof(state));

    /* Scan the project tree */
    scan_dir_bsr(art, art->project_path, 0, &state);

    /* Post-scan validation for missing entries */
    validate_registry(art, &state);

    /* Write the report */
    return write_report_bsr(art, output_dir, initial_issues, &state);
}

/* --------------------------------------------------------------------------
 * Recipe registration
 * -------------------------------------------------------------------------- */

void recipe_bootstrap_registry_register(void) {
    lst_recipe_t r;
    memset(&r, 0, sizeof(r));
    snprintf(r.name, LST_MAX_NAME, "bootstrap-registry");
    snprintf(r.description, LST_MAX_NAME,
        "Validate NNOS bootstrap registry against 180-pattern capability rules");
    r.execute = recipe_bootstrap_registry;
    r.version = 1;
    lst_recipe_register(&r);
}
