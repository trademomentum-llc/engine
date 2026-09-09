#define _POSIX_C_SOURCE 200809L

/*
 * daemon_constellation.c -- Daemon Constellation Check Recipe
 *
 * Validates the NNOS daemon constellation -- the set of daemons that
 * form the runtime environment for the Neural Network Operating System.
 *
 * Checks performed:
 *   - All expected daemons are declared in project sources
 *   - Systemd unit files or supervisor configs exist for each daemon
 *   - Inter-daemon communication patterns are defined (IPC/gRPC/socket)
 *   - Health check endpoints are present for each daemon
 *   - Baseline vs expanded daemon sets are classified
 *   - Boot daemon initialization order is validated
 *   - Child process spawning patterns are accounted for
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
 * Daemon constellation definitions
 * -------------------------------------------------------------------------- */

/* Baseline daemons -- always required */
static const char *BASELINE_DAEMONS[] = {
    "nnos_morph_engine",
    "nnos_threat_scanner",
    "nnos_neuro_analyzer",
    "boot_daemon",
    NULL
};

/* Expanded daemons -- optional child processes under nnos_* namespace */
static const char *EXPANDED_DAEMONS[] = {
    "nnos_context_gating",
    "nnos_profile_matcher",
    "nnos_state_monitor",
    "nnos_task_manager",
    "nnos_comm_bridge",
    NULL
};

/* Systemd unit file patterns */
static const char *UNIT_FILE_PATTERNS[] = {
    ".service",
    ".timer",
    ".socket",
    NULL
};

/* Supervisor config patterns */
static const char *SUPERVISOR_PATTERNS[] = {
    "supervisord",     "supervisor",
    "[program:",       "command=",
    "process_name",    "numprocs",
    NULL
};

/* Inter-daemon communication patterns */
static const char *IPC_PATTERNS[] = {
    "grpc",            "gRPC",           "grpc_channel",
    "unix_socket",     "unix_domain",    "AF_UNIX",
    "ipc_channel",     "IpcChannel",     "ipc_connect",
    "message_bus",     "MessageBus",     "message_queue",
    "dbus",            "D-Bus",          "sd_bus",
    "pipe",            "mkfifo",         "named_pipe",
    "shared_memory",   "shm_open",       "mmap",
    NULL
};

/* Health check patterns */
static const char *HEALTH_CHECK_PATTERNS[] = {
    "health_check",    "healthCheck",    "HealthCheck",
    "health_endpoint", "healthEndpoint",
    "/health",         "/healthz",       "/readyz",
    "liveness",        "readiness",      "startup_probe",
    "is_healthy",      "isHealthy",      "check_health",
    "sd_notify",       "READY=1",        "WATCHDOG=1",
    NULL
};

/* Boot order patterns */
static const char *BOOT_ORDER_PATTERNS[] = {
    "boot_order",      "bootOrder",      "BootOrder",
    "startup_order",   "startupOrder",   "StartupOrder",
    "init_sequence",   "initSequence",   "InitSequence",
    "After=",          "Before=",        "Requires=",
    "Wants=",          "PartOf=",        "BindsTo=",
    "depends_on",      "dependency_order",
    NULL
};

/* Child process patterns */
static const char *CHILD_PROCESS_PATTERNS[] = {
    "fork",            "spawn",          "exec",
    "child_process",   "ChildProcess",   "childProcess",
    "subprocess",      "Subprocess",
    "posix_spawn",     "clone(",
    "nnos_spawn",      "daemon_spawn",
    NULL
};

/* Directories to skip during scan */
static const char *SKIP_DIRS_DCN[] = {
    "node_modules", "vendor", ".git", "__pycache__", "dist", "build",
    ".venv", "venv", ".next", "target", ".cache", NULL
};

/* --------------------------------------------------------------------------
 * Helpers
 * -------------------------------------------------------------------------- */

static char *read_file_dcn(const char *path, size_t *out_len) {
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

static void add_dcn_issue(lst_artifact_t *art, uint8_t severity,
                           const char *title, const char *desc,
                           const char *file_path, uint32_t line,
                           const char *ref) {
    if (art->issue_count >= LST_MAX_ISSUES) return;
    lst_issue_t *issue = &art->issues[art->issue_count];
    memset(issue, 0, sizeof(lst_issue_t));

    snprintf(issue->id, sizeof(issue->id), "DCN-%04u", art->issue_count + 1);
    issue->severity = severity;
    snprintf(issue->title, sizeof(issue->title), "%s", title);
    snprintf(issue->description, sizeof(issue->description), "%s", desc);
    if (file_path)
        snprintf(issue->file_path, sizeof(issue->file_path), "%s", file_path);
    issue->line_number = line;
    if (ref)
        snprintf(issue->cwe, sizeof(issue->cwe), "%s", ref);
    snprintf(issue->remediation, sizeof(issue->remediation),
        "See engine/nnos/ specs and systemd/ unit files");

    art->issue_count++;
}

static int contains_any_dcn(const char *content, const char **patterns) {
    for (int i = 0; patterns[i]; i++) {
        if (strstr(content, patterns[i])) return 1;
    }
    return 0;
}

static int count_dcn_matches(const char *content, const char **patterns) {
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
 * Scan state
 * -------------------------------------------------------------------------- */

typedef struct {
    /* Per-daemon declaration tracking */
    int baseline_declared[16];
    int expanded_declared[16];
    /* Per-daemon unit file tracking */
    int baseline_has_unit[16];
    int expanded_has_unit[16];
    /* Per-daemon health check tracking */
    int baseline_has_health[16];
    int expanded_has_health[16];
    /* Global patterns */
    int has_ipc;
    int has_boot_order;
    int has_child_spawning;
    int has_supervisor;
    int ipc_count;
    int health_count;
    int boot_order_count;
    int child_spawn_count;
    int unit_file_count;
    int supervisor_count;
    int files_scanned;
} dcn_scan_result_t;

/* --------------------------------------------------------------------------
 * Directory scanning
 * -------------------------------------------------------------------------- */

static int should_skip_dcn(const char *name) {
    if (name[0] == '.') return 1;
    for (int i = 0; SKIP_DIRS_DCN[i]; i++)
        if (strcmp(name, SKIP_DIRS_DCN[i]) == 0) return 1;
    return 0;
}

static int is_dcn_scannable(const char *name) {
    const char *ext = strrchr(name, '.');
    if (!ext) return 0;
    const char *exts[] = {".c", ".cpp", ".h", ".hpp", ".py", ".js", ".ts",
                          ".yaml", ".yml", ".toml", ".json", ".conf",
                          ".go", ".rs", ".java", ".service", ".ini",
                          ".cfg", ".sh", NULL};
    for (int i = 0; exts[i]; i++)
        if (strcasecmp(ext, exts[i]) == 0) return 1;
    return 0;
}

static int is_unit_file(const char *name) {
    for (int i = 0; UNIT_FILE_PATTERNS[i]; i++) {
        const char *ext = strstr(name, UNIT_FILE_PATTERNS[i]);
        if (ext && ext[strlen(UNIT_FILE_PATTERNS[i])] == '\0') return 1;
    }
    return 0;
}

static void scan_file_dcn(lst_artifact_t *art, dcn_scan_result_t *result,
                           const char *fpath) {
    size_t len = 0;
    char *content = read_file_dcn(fpath, &len);
    if (!content) return;

    result->files_scanned++;

    /* Check baseline daemon declarations */
    for (int i = 0; BASELINE_DAEMONS[i]; i++) {
        if (strstr(content, BASELINE_DAEMONS[i])) {
            result->baseline_declared[i] = 1;

            /* Check if this file also has a health check for this daemon */
            if (contains_any_dcn(content, HEALTH_CHECK_PATTERNS)) {
                result->baseline_has_health[i] = 1;
            }
        }
    }

    /* Check expanded daemon declarations */
    for (int i = 0; EXPANDED_DAEMONS[i]; i++) {
        if (strstr(content, EXPANDED_DAEMONS[i])) {
            result->expanded_declared[i] = 1;

            if (contains_any_dcn(content, HEALTH_CHECK_PATTERNS)) {
                result->expanded_has_health[i] = 1;
            }
        }
    }

    /* Check if this is a unit file for a known daemon */
    const char *basename = strrchr(fpath, '/');
    basename = basename ? basename + 1 : fpath;
    if (is_unit_file(basename)) {
        result->unit_file_count++;
        for (int i = 0; BASELINE_DAEMONS[i]; i++) {
            if (strstr(basename, BASELINE_DAEMONS[i]))
                result->baseline_has_unit[i] = 1;
        }
        for (int i = 0; EXPANDED_DAEMONS[i]; i++) {
            if (strstr(basename, EXPANDED_DAEMONS[i]))
                result->expanded_has_unit[i] = 1;
        }
    }

    /* Check for supervisor configs */
    if (contains_any_dcn(content, SUPERVISOR_PATTERNS)) {
        result->has_supervisor = 1;
        result->supervisor_count += count_dcn_matches(content, SUPERVISOR_PATTERNS);

        /* Check supervisor program blocks for baseline daemons */
        for (int i = 0; BASELINE_DAEMONS[i]; i++) {
            if (strstr(content, BASELINE_DAEMONS[i]))
                result->baseline_has_unit[i] = 1;
        }
        for (int i = 0; EXPANDED_DAEMONS[i]; i++) {
            if (strstr(content, EXPANDED_DAEMONS[i]))
                result->expanded_has_unit[i] = 1;
        }
    }

    /* IPC patterns */
    if (contains_any_dcn(content, IPC_PATTERNS)) {
        result->has_ipc = 1;
        result->ipc_count += count_dcn_matches(content, IPC_PATTERNS);
    }

    /* Health checks */
    if (contains_any_dcn(content, HEALTH_CHECK_PATTERNS)) {
        result->health_count += count_dcn_matches(content, HEALTH_CHECK_PATTERNS);
    }

    /* Boot order */
    if (contains_any_dcn(content, BOOT_ORDER_PATTERNS)) {
        result->has_boot_order = 1;
        result->boot_order_count += count_dcn_matches(content, BOOT_ORDER_PATTERNS);
    }

    /* Child process spawning */
    if (contains_any_dcn(content, CHILD_PROCESS_PATTERNS)) {
        result->has_child_spawning = 1;
        result->child_spawn_count += count_dcn_matches(content, CHILD_PROCESS_PATTERNS);
    }

    /* Line-level checks for daemon references without health endpoints */
    int lineno = 1;
    char *line_start = content;
    while (*line_start) {
        char *eol = strchr(line_start, '\n');
        if (eol) *eol = '\0';

        /* Detect daemon declaration without corresponding health check nearby */
        for (int i = 0; BASELINE_DAEMONS[i]; i++) {
            if (strstr(line_start, BASELINE_DAEMONS[i]) &&
                (strstr(line_start, "start") || strstr(line_start, "exec") ||
                 strstr(line_start, "launch") || strstr(line_start, "run"))) {
                if (!contains_any_dcn(line_start, HEALTH_CHECK_PATTERNS)) {
                    char desc[LST_MAX_NAME];
                    snprintf(desc, sizeof(desc),
                        "Daemon '%s' launched without inline health check",
                        BASELINE_DAEMONS[i]);
                    add_dcn_issue(art, SEV_INFO,
                        "Daemon Launch Without Health Check",
                        desc, fpath, (uint32_t)lineno, "DCN-HEALTH");
                }
            }
        }

        if (!eol) break;
        line_start = eol + 1;
        lineno++;
    }

    free(content);
}

static void scan_dir_dcn(lst_artifact_t *art, dcn_scan_result_t *result,
                          const char *dir, int depth) {
    if (depth > 8) return;

    DIR *d = opendir(dir);
    if (!d) return;

    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        if (ent->d_name[0] == '.') continue;
        if (should_skip_dcn(ent->d_name)) continue;

        char child[LST_MAX_PATH];
        snprintf(child, sizeof(child), "%s/%s", dir, ent->d_name);

        struct stat st;
        if (stat(child, &st) != 0) continue;

        if (S_ISDIR(st.st_mode)) {
            scan_dir_dcn(art, result, child, depth + 1);
        } else if (S_ISREG(st.st_mode) && is_dcn_scannable(ent->d_name)) {
            scan_file_dcn(art, result, child);
        }
    }
    closedir(d);
}

/* --------------------------------------------------------------------------
 * Issue generation from scan results
 * -------------------------------------------------------------------------- */

static void generate_dcn_issues(lst_artifact_t *art, dcn_scan_result_t *result) {
    /* Check baseline daemon declarations */
    for (int i = 0; BASELINE_DAEMONS[i]; i++) {
        if (!result->baseline_declared[i]) {
            char desc[LST_MAX_NAME];
            snprintf(desc, sizeof(desc),
                "Baseline daemon '%s' not found in project sources",
                BASELINE_DAEMONS[i]);
            add_dcn_issue(art, SEV_ERROR,
                "Missing Baseline Daemon Declaration",
                desc, NULL, 0, "DCN-BASELINE");
        }
    }

    /* Check baseline daemon unit files / supervisor configs */
    for (int i = 0; BASELINE_DAEMONS[i]; i++) {
        if (result->baseline_declared[i] && !result->baseline_has_unit[i]) {
            char desc[LST_MAX_NAME];
            snprintf(desc, sizeof(desc),
                "No systemd unit or supervisor config for '%s'",
                BASELINE_DAEMONS[i]);
            add_dcn_issue(art, SEV_WARNING,
                "Missing Daemon Service Config",
                desc, NULL, 0, "DCN-UNIT");
        }
    }

    /* Check baseline daemon health checks */
    for (int i = 0; BASELINE_DAEMONS[i]; i++) {
        if (result->baseline_declared[i] && !result->baseline_has_health[i]) {
            char desc[LST_MAX_NAME];
            snprintf(desc, sizeof(desc),
                "No health check endpoint found for '%s'",
                BASELINE_DAEMONS[i]);
            add_dcn_issue(art, SEV_WARNING,
                "Missing Daemon Health Check",
                desc, NULL, 0, "DCN-HEALTH");
        }
    }

    /* Check expanded daemon health checks (info-level) */
    for (int i = 0; EXPANDED_DAEMONS[i]; i++) {
        if (result->expanded_declared[i] && !result->expanded_has_health[i]) {
            char desc[LST_MAX_NAME];
            snprintf(desc, sizeof(desc),
                "Expanded daemon '%s' has no health check endpoint",
                EXPANDED_DAEMONS[i]);
            add_dcn_issue(art, SEV_INFO,
                "Expanded Daemon Missing Health Check",
                desc, NULL, 0, "DCN-HEALTH");
        }
    }

    /* IPC communication */
    if (!result->has_ipc) {
        add_dcn_issue(art, SEV_ERROR,
            "No Inter-Daemon Communication",
            "No IPC/gRPC/socket patterns found between daemons",
            NULL, 0, "DCN-IPC");
    }

    /* Boot order */
    if (!result->has_boot_order) {
        add_dcn_issue(art, SEV_WARNING,
            "No Boot Order Defined",
            "No daemon startup ordering (After=/Before=/depends_on) found",
            NULL, 0, "DCN-BOOT");
    }

    /* Child process management */
    if (!result->has_child_spawning) {
        add_dcn_issue(art, SEV_INFO,
            "No Child Process Spawning Detected",
            "No fork/spawn patterns for nnos_* child processes found",
            NULL, 0, "DCN-CHILD");
    }

    /* No unit files at all */
    if (result->unit_file_count == 0 && !result->has_supervisor) {
        add_dcn_issue(art, SEV_CRITICAL,
            "No Service Management Configuration",
            "No systemd units or supervisor configs found for any daemon",
            NULL, 0, "DCN-UNIT");
    }
}

/* --------------------------------------------------------------------------
 * Constellation score
 * -------------------------------------------------------------------------- */

static float compute_constellation_score(dcn_scan_result_t *result) {
    float score = 0.0f;
    float max_score = 0.0f;

    /* Baseline daemons declared: 4 points each (weight: critical) */
    for (int i = 0; BASELINE_DAEMONS[i]; i++) {
        max_score += 4.0f;
        if (result->baseline_declared[i]) score += 4.0f;
    }

    /* Baseline daemons have unit files: 2 points each */
    for (int i = 0; BASELINE_DAEMONS[i]; i++) {
        max_score += 2.0f;
        if (result->baseline_has_unit[i]) score += 2.0f;
    }

    /* Baseline daemons have health checks: 2 points each */
    for (int i = 0; BASELINE_DAEMONS[i]; i++) {
        max_score += 2.0f;
        if (result->baseline_has_health[i]) score += 2.0f;
    }

    /* IPC: 4 points */
    max_score += 4.0f;
    if (result->has_ipc) score += 4.0f;

    /* Boot order: 3 points */
    max_score += 3.0f;
    if (result->has_boot_order) score += 3.0f;

    /* Child spawning: 1 point */
    max_score += 1.0f;
    if (result->has_child_spawning) score += 1.0f;

    if (max_score <= 0.0f) return 0.0f;
    return (score / max_score) * 100.0f;
}

/* --------------------------------------------------------------------------
 * Report writer
 * -------------------------------------------------------------------------- */

static void write_sep_dcn(FILE *f) {
    for (int i = 0; i < 80; i++) fputc('=', f);
    fputc('\n', f);
}

static void write_line_dcn(FILE *f) {
    for (int i = 0; i < 40; i++) fputc('-', f);
    fputc('\n', f);
}

/* --------------------------------------------------------------------------
 * Recipe entry point
 * -------------------------------------------------------------------------- */

static int recipe_daemon_constellation(lst_artifact_t *art, const char *output_dir) {
    if (!art) return -1;

    uint32_t initial_issues = art->issue_count;

    /* Scan project tree */
    dcn_scan_result_t scan;
    memset(&scan, 0, sizeof(scan));
    scan_dir_dcn(art, &scan, art->project_path, 0);

    /* Generate issues from scan results */
    generate_dcn_issues(art, &scan);

    uint32_t new_issues = art->issue_count - initial_issues;

    /* Build output path */
    char outpath[LST_MAX_PATH];
    if (output_dir)
        snprintf(outpath, sizeof(outpath), "%s/DAEMON_CONSTELLATION_REPORT", output_dir);
    else
        snprintf(outpath, sizeof(outpath), "%s/DAEMON_CONSTELLATION_REPORT", art->project_path);

    if (output_dir) mkdir(output_dir, 0755);

    int rfd = open(outpath, O_WRONLY | O_CREAT | O_TRUNC | O_NOFOLLOW | O_CLOEXEC, 0644);
    if (rfd < 0) {
        fprintf(stderr, "daemon-constellation: cannot write %s\n", outpath);
        return -1;
    }
    FILE *f = fdopen(rfd, "w");
    if (!f) {
        close(rfd);
        fprintf(stderr, "daemon-constellation: cannot write %s\n", outpath);
        return -1;
    }

    /* Header */
    write_sep_dcn(f);
    fprintf(f, "DAEMON CONSTELLATION CHECK REPORT\n");
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
    fprintf(f, "Files Scanned: %d\n", scan.files_scanned);
    fprintf(f, "Issues Found: %u\n", new_issues);
    write_sep_dcn(f);
    fprintf(f, "\n");

    /* Constellation overview */
    fprintf(f, "DAEMON CONSTELLATION OVERVIEW\n");
    write_line_dcn(f);
    fprintf(f, "\n");
    fprintf(f, "  Baseline Daemons (required):\n");
    for (int i = 0; BASELINE_DAEMONS[i]; i++) {
        fprintf(f, "    [%s] %-25s  unit:[%s]  health:[%s]\n",
            scan.baseline_declared[i] ? "FOUND" : "MISS ",
            BASELINE_DAEMONS[i],
            scan.baseline_has_unit[i] ? "YES" : "NO ",
            scan.baseline_has_health[i] ? "YES" : "NO ");
    }
    fprintf(f, "\n");

    fprintf(f, "  Expanded Daemons (nnos_* children):\n");
    int expanded_count = 0;
    for (int i = 0; EXPANDED_DAEMONS[i]; i++) {
        if (scan.expanded_declared[i]) expanded_count++;
        fprintf(f, "    [%s] %-25s  unit:[%s]  health:[%s]\n",
            scan.expanded_declared[i] ? "FOUND" : "----",
            EXPANDED_DAEMONS[i],
            scan.expanded_has_unit[i] ? "YES" : "NO ",
            scan.expanded_has_health[i] ? "YES" : "NO ");
    }
    fprintf(f, "\n");

    /* Communication and infrastructure */
    fprintf(f, "  Infrastructure:\n");
    fprintf(f, "    IPC/Communication:    %s (%d references)\n",
        scan.has_ipc ? "PRESENT" : "ABSENT", scan.ipc_count);
    fprintf(f, "    Health Checks:        %d references\n", scan.health_count);
    fprintf(f, "    Boot Ordering:        %s (%d references)\n",
        scan.has_boot_order ? "PRESENT" : "ABSENT", scan.boot_order_count);
    fprintf(f, "    Child Spawning:       %s (%d references)\n",
        scan.has_child_spawning ? "PRESENT" : "ABSENT", scan.child_spawn_count);
    fprintf(f, "    Unit Files Found:     %d\n", scan.unit_file_count);
    fprintf(f, "    Supervisor Configs:   %s (%d references)\n",
        scan.has_supervisor ? "PRESENT" : "ABSENT", scan.supervisor_count);
    fprintf(f, "\n");

    /* Daemon set classification */
    fprintf(f, "  Daemon Set Classification:\n");
    int baseline_total = 0;
    for (int i = 0; BASELINE_DAEMONS[i]; i++) baseline_total++;
    int baseline_found = 0;
    for (int i = 0; BASELINE_DAEMONS[i]; i++) {
        if (scan.baseline_declared[i]) baseline_found++;
    }
    fprintf(f, "    Baseline:  %d / %d declared\n", baseline_found, baseline_total);
    int expanded_total = 0;
    for (int i = 0; EXPANDED_DAEMONS[i]; i++) expanded_total++;
    fprintf(f, "    Expanded:  %d / %d declared\n", expanded_count, expanded_total);
    if (baseline_found == baseline_total && expanded_count > 0)
        fprintf(f, "    Status:    FULL CONSTELLATION\n");
    else if (baseline_found == baseline_total)
        fprintf(f, "    Status:    BASELINE ONLY\n");
    else if (baseline_found > 0)
        fprintf(f, "    Status:    PARTIAL (incomplete baseline)\n");
    else
        fprintf(f, "    Status:    NO CONSTELLATION\n");
    fprintf(f, "\n");

    /* Findings */
    if (new_issues > 0) {
        fprintf(f, "CONSTELLATION FINDINGS\n");
        write_sep_dcn(f);
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
                    fprintf(f, "      Ref: %s\n", issue->cwe);
                fprintf(f, "      %s\n\n", issue->description);
            }
        }
    } else {
        fprintf(f, "CONSTELLATION VALIDATION: PASS\n");
        fprintf(f, "  All daemon constellation checks passed.\n\n");
    }

    /* Constellation checklist */
    fprintf(f, "CONSTELLATION CHECKLIST\n");
    write_line_dcn(f);

    int has_all_baseline = (baseline_found == baseline_total);
    int has_all_units = 1;
    for (int i = 0; BASELINE_DAEMONS[i]; i++) {
        if (scan.baseline_declared[i] && !scan.baseline_has_unit[i])
            has_all_units = 0;
    }
    int has_all_health = 1;
    for (int i = 0; BASELINE_DAEMONS[i]; i++) {
        if (scan.baseline_declared[i] && !scan.baseline_has_health[i])
            has_all_health = 0;
    }

    fprintf(f, "  [%s] All baseline daemons declared\n",
        has_all_baseline ? "PASS" : "FAIL");
    fprintf(f, "  [%s] Service configs for all baseline daemons\n",
        has_all_units ? "PASS" : "FAIL");
    fprintf(f, "  [%s] Health checks for all baseline daemons\n",
        has_all_health ? "PASS" : "FAIL");
    fprintf(f, "  [%s] Inter-daemon communication defined\n",
        scan.has_ipc ? "PASS" : "FAIL");
    fprintf(f, "  [%s] Boot/startup ordering defined\n",
        scan.has_boot_order ? "PASS" : "FAIL");
    fprintf(f, "  [%s] Child process spawning patterns present\n",
        scan.has_child_spawning ? "PASS" : "WARN");
    fprintf(f, "\n");

    /* Score */
    float constellation_score = compute_constellation_score(&scan);

    fprintf(f, "CONSTELLATION SCORE: %.0f%%\n", (double)constellation_score);
    fprintf(f, "\n");

    /* Footer */
    write_sep_dcn(f);
    fprintf(f, "END OF DAEMON CONSTELLATION REPORT\n");
    fprintf(f, "\nThis report validates the NNOS daemon constellation as defined in\n");
    fprintf(f, "engine/nnos/ (boot.cpp, main_engine.cpp, systemd/ units, specs/).\n");
    fprintf(f, "Baseline daemons: nnos_morph_engine, nnos_threat_scanner,\n");
    fprintf(f, "nnos_neuro_analyzer, boot_daemon.\n");
    fprintf(f, "Expanded set: nnos_* child processes (context_gating, profile_matcher,\n");
    fprintf(f, "state_monitor, task_manager, comm_bridge).\n");
    write_sep_dcn(f);
    fprintf(f, "\n");

    fclose(f);

    printf("  Wrote %s (%u issues)\n", outpath, new_issues);
    return 0;
}

/* --------------------------------------------------------------------------
 * Recipe registration
 * -------------------------------------------------------------------------- */

void recipe_daemon_constellation_register(void) {
    lst_recipe_t r;
    memset(&r, 0, sizeof(r));
    snprintf(r.name, sizeof(r.name), "daemon-constellation");
    snprintf(r.description, sizeof(r.description),
        "Validate NNOS daemon constellation (baseline/expanded daemons, IPC, health)");
    r.execute = recipe_daemon_constellation;
    r.version = 1;
    lst_recipe_register(&r);
}
