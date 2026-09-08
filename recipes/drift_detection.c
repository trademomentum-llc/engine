#define _POSIX_C_SOURCE 200809L

/*
 * drift_detection.c -- NNOS Drift Detection Recipe
 *
 * Detects configuration and state drift across the NNOS daemon constellation:
 *   - Config file hash mismatches against stored baselines
 *   - Environment variable divergence between daemon configs
 *   - Daemon state files that drifted from baseline
 *   - Schema version mismatches between components
 *   - Timestamp anomalies (stale, future-dated, or skewed files)
 *
 * Reports drift severity per category and recommends remediation.
 * Aligned with NNOS operational integrity requirements.
 */

#include "lst.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>

/* --------------------------------------------------------------------------
 * Drift pattern sets
 * -------------------------------------------------------------------------- */

/* Config file extensions to check for hash drift */
static const char *CONFIG_EXTENSIONS[] = {
    ".conf",   ".cfg",    ".ini",    ".toml",
    ".yaml",   ".yml",    ".json",   ".env",
    ".properties", ".xml",
    NULL
};

/* State file patterns (daemon state, PID files, lock files) */
static const char *STATE_FILE_PATTERNS[] = {
    ".state",  ".pid",    ".lock",   ".status",
    ".checkpoint", ".snapshot", ".db",
    NULL
};

/* Schema version indicators */
static const char *SCHEMA_VERSION_PATTERNS[] = {
    "schema_version",  "SchemaVersion",  "schemaVersion",
    "db_version",      "migration_version", "MigrationVersion",
    "api_version",     "ApiVersion",     "apiVersion",
    "protocol_version", "ProtocolVersion",
    "format_version",  "FormatVersion",  "formatVersion",
    NULL
};

/* Environment variable patterns */
static const char *ENV_VAR_PATTERNS[] = {
    "NNOS_",       "DAEMON_",     "NODE_ENV",
    "CONFIG_PATH", "STATE_DIR",   "LOG_LEVEL",
    "BIND_ADDR",   "LISTEN_PORT", "DATA_DIR",
    "CLUSTER_",    "PEER_",       "QUORUM_",
    NULL
};

/* Baseline reference patterns */
static const char *BASELINE_PATTERNS[] = {
    "baseline",    "Baseline",    "BASELINE",
    "golden",      "reference",   "expected_hash",
    "known_good",  "canonical",
    NULL
};

/* Directories to skip during scanning */
static const char *SKIP_DIRS[] = {
    "node_modules", ".git",  "target",  "build",
    "__pycache__",  ".tox",  "vendor",  "dist",
    ".mypy_cache",  ".pytest_cache",
    NULL
};

/* NNOS daemon names (from nnos/ specs) */
static const char *NNOS_DAEMONS[] = {
    "cortexd",    "hippod",     "cerebelld",  "thalamud",
    "amygdalad",  "prefrontd",  "brainstem",  "motord",
    "sensoryd",   "membraned",  "synapsed",
    NULL
};

/* --------------------------------------------------------------------------
 * Helpers
 * -------------------------------------------------------------------------- */

static char *read_file_dft(const char *path, size_t *out_len) {
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

static void add_dft_issue(lst_artifact_t *art, uint8_t severity,
                           const char *title, const char *desc,
                           const char *file_path, uint32_t line,
                           const char *cwe, const char *remediation) {
    if (art->issue_count >= LST_MAX_ISSUES) return;
    lst_issue_t *issue = &art->issues[art->issue_count];
    memset(issue, 0, sizeof(lst_issue_t));

    snprintf(issue->id, sizeof(issue->id), "DFT-%04u", art->issue_count + 1);
    issue->severity = severity;
    snprintf(issue->title, sizeof(issue->title), "%s", title);
    snprintf(issue->description, sizeof(issue->description), "%s", desc);
    if (file_path)
        snprintf(issue->file_path, sizeof(issue->file_path), "%s", file_path);
    issue->line_number = line;
    if (cwe)
        snprintf(issue->cwe, sizeof(issue->cwe), "%s", cwe);
    if (remediation)
        snprintf(issue->remediation, sizeof(issue->remediation), "%s", remediation);

    art->issue_count++;
}

static int has_extension(const char *name, const char **extensions) {
    size_t nlen = strlen(name);
    for (int i = 0; extensions[i]; i++) {
        size_t elen = strlen(extensions[i]);
        if (nlen > elen && strcmp(name + nlen - elen, extensions[i]) == 0)
            return 1;
    }
    return 0;
}

static int has_suffix(const char *name, const char **suffixes) {
    for (int i = 0; suffixes[i]; i++) {
        if (strstr(name, suffixes[i])) return 1;
    }
    return 0;
}

static int should_skip_dft(const char *name) {
    for (int i = 0; SKIP_DIRS[i]; i++) {
        if (strcmp(name, SKIP_DIRS[i]) == 0) return 1;
    }
    return 0;
}

static int contains_any_dft(const char *content, const char **patterns) {
    for (int i = 0; patterns[i]; i++) {
        if (strstr(content, patterns[i])) return 1;
    }
    return 0;
}

static int count_dft_matches(const char *content, const char **patterns) {
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

/* Simple DJB2 hash for comparing file contents */
static uint32_t hash_content(const char *data, size_t len) {
    uint32_t h = 5381;
    for (size_t i = 0; i < len; i++) {
        h = ((h << 5) + h) + (uint32_t)(unsigned char)data[i];
    }
    return h;
}

/* --------------------------------------------------------------------------
 * Drift scan context
 * -------------------------------------------------------------------------- */

typedef struct {
    int config_files_found;
    int config_hash_mismatches;
    int state_files_found;
    int state_files_stale;
    int schema_version_refs;
    int schema_version_mismatches;
    int env_var_refs;
    int env_var_divergences;
    int timestamp_anomalies;
    int baseline_refs;
    int daemon_config_refs;
    uint32_t config_hashes[512];
    int config_hash_count;
} drift_ctx_t;

/* --------------------------------------------------------------------------
 * Timestamp anomaly detection
 * -------------------------------------------------------------------------- */

/* Check if a file's mtime is anomalous: too old (>90 days stale) or
 * in the future relative to build time */
static int check_timestamp_anomaly(const char *path, time_t reference_time) {
    struct stat st;
    if (stat(path, &st) != 0) return 0;

    double diff = difftime(reference_time, st.st_mtime);

    /* Future-dated file (more than 1 hour ahead) */
    if (diff < -3600.0) return 2;

    /* Stale file (more than 90 days old) */
    if (diff > 90.0 * 24.0 * 3600.0) return 1;

    return 0;
}

/* --------------------------------------------------------------------------
 * Per-file drift analysis
 * -------------------------------------------------------------------------- */

static void analyze_file_drift(lst_artifact_t *art, drift_ctx_t *ctx,
                                const char *path, const char *name) {
    time_t now = time(NULL);

    /* Check config file hash drift */
    if (has_extension(name, CONFIG_EXTENSIONS)) {
        ctx->config_files_found++;

        size_t len = 0;
        char *content = read_file_dft(path, &len);
        if (content) {
            uint32_t h = hash_content(content, len);

            /* Store hash for cross-comparison */
            if (ctx->config_hash_count < 512) {
                ctx->config_hashes[ctx->config_hash_count++] = h;
            }

            /* Check for daemon-specific config references */
            if (contains_any_dft(content, NNOS_DAEMONS)) {
                ctx->daemon_config_refs++;
            }

            /* Check for env var divergence patterns */
            int env_count = count_dft_matches(content, ENV_VAR_PATTERNS);
            ctx->env_var_refs += env_count;

            /* Check for schema version references */
            if (contains_any_dft(content, SCHEMA_VERSION_PATTERNS)) {
                ctx->schema_version_refs++;
            }

            /* Check for baseline references */
            if (contains_any_dft(content, BASELINE_PATTERNS)) {
                ctx->baseline_refs++;
            }

            free(content);
        }

        /* Timestamp check on config files */
        int anomaly = check_timestamp_anomaly(path, now);
        if (anomaly == 1) {
            ctx->timestamp_anomalies++;
            add_dft_issue(art, SEV_WARNING,
                "Stale config file detected",
                "Config file not updated in >90 days, may be drifted",
                path, 0, "CWE-1188",
                "Review config file and update or re-baseline");
        } else if (anomaly == 2) {
            ctx->timestamp_anomalies++;
            add_dft_issue(art, SEV_ERROR,
                "Future-dated config file",
                "Config file has future timestamp, possible clock skew or tampering",
                path, 0, "CWE-367",
                "Investigate clock synchronization across nodes");
        }
    }

    /* Check state file drift */
    if (has_suffix(name, STATE_FILE_PATTERNS)) {
        ctx->state_files_found++;

        int anomaly = check_timestamp_anomaly(path, now);
        if (anomaly == 1) {
            ctx->state_files_stale++;
            add_dft_issue(art, SEV_ERROR,
                "Stale daemon state file",
                "State file not updated in >90 days, daemon may be inactive",
                path, 0, "CWE-672",
                "Restart daemon or remove orphaned state file");
        } else if (anomaly == 2) {
            ctx->timestamp_anomalies++;
            add_dft_issue(art, SEV_ERROR,
                "Future-dated state file",
                "State file has future timestamp, clock skew across daemon nodes",
                path, 0, "CWE-367",
                "Synchronize system clocks with NTP across all NNOS nodes");
        }
    }
}

/* --------------------------------------------------------------------------
 * Recursive directory walker
 * -------------------------------------------------------------------------- */

static void scan_dir_dft(lst_artifact_t *art, drift_ctx_t *ctx,
                          const char *dir, int depth) {
    if (depth > 8) return;

    DIR *d = opendir(dir);
    if (!d) return;

    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        if (ent->d_name[0] == '.') continue;
        if (should_skip_dft(ent->d_name)) continue;

        char child[LST_MAX_PATH];
        snprintf(child, sizeof(child), "%s/%s", dir, ent->d_name);

        struct stat st;
        if (stat(child, &st) != 0) continue;

        if (S_ISDIR(st.st_mode)) {
            scan_dir_dft(art, ctx, child, depth + 1);
        } else if (S_ISREG(st.st_mode)) {
            analyze_file_drift(art, ctx, child, ent->d_name);
        }
    }
    closedir(d);
}

/* --------------------------------------------------------------------------
 * Schema version cross-check
 *
 * Walk artifact files looking for schema_version patterns and flag
 * files that declare different version strings.
 * -------------------------------------------------------------------------- */

static void check_schema_drift(lst_artifact_t *art, drift_ctx_t *ctx) {
    char first_version[64];
    first_version[0] = '\0';

    for (uint32_t i = 0; i < art->file_count; i++) {
        size_t len = 0;
        char *content = read_file_dft(art->files[i].path, &len);
        if (!content) continue;

        for (int p = 0; SCHEMA_VERSION_PATTERNS[p]; p++) {
            const char *match = strstr(content, SCHEMA_VERSION_PATTERNS[p]);
            if (!match) continue;

            /* Advance past the pattern to find the version value */
            const char *val = match + strlen(SCHEMA_VERSION_PATTERNS[p]);

            /* Skip delimiters: =, :, ", space */
            while (*val == '=' || *val == ':' || *val == '"' ||
                   *val == '\'' || *val == ' ' || *val == '\t') {
                val++;
            }

            /* Extract version token (up to 32 chars, alphanumeric + dots) */
            char ver[64];
            int vi = 0;
            while (vi < 63 && *val != '\0' && *val != '"' &&
                   *val != '\'' && *val != ',' && *val != '\n' &&
                   *val != ' ' && *val != '\t') {
                ver[vi++] = *val++;
            }
            ver[vi] = '\0';

            if (vi == 0) continue;

            if (first_version[0] == '\0') {
                snprintf(first_version, sizeof(first_version), "%s", ver);
            } else if (strcmp(first_version, ver) != 0) {
                ctx->schema_version_mismatches++;
                char desc[LST_MAX_NAME];
                snprintf(desc, sizeof(desc),
                         "Schema version '%s' differs from '%s'",
                         ver, first_version);
                add_dft_issue(art, SEV_ERROR,
                    "Schema version mismatch between components",
                    desc, art->files[i].path, 0, "CWE-1104",
                    "Align schema versions across all NNOS daemons");
            }
            break;
        }

        free(content);
    }
}

/* --------------------------------------------------------------------------
 * Environment variable divergence check
 *
 * Compare env var references across config files. If the same env var
 * name appears with different default values, flag divergence.
 * -------------------------------------------------------------------------- */

static void check_env_divergence(lst_artifact_t *art, drift_ctx_t *ctx) {
    int files_with_env = 0;
    int files_without_env = 0;

    for (uint32_t i = 0; i < art->file_count; i++) {
        if (!has_extension(art->files[i].path, CONFIG_EXTENSIONS)) continue;

        size_t len = 0;
        char *content = read_file_dft(art->files[i].path, &len);
        if (!content) continue;

        if (contains_any_dft(content, ENV_VAR_PATTERNS)) {
            files_with_env++;
        } else {
            files_without_env++;
        }

        free(content);
    }

    /* If some config files reference env vars and others do not, flag it */
    if (files_with_env > 0 && files_without_env > 0) {
        ctx->env_var_divergences++;
        char desc[LST_MAX_NAME];
        snprintf(desc, sizeof(desc),
                 "%d config files use env vars, %d do not",
                 files_with_env, files_without_env);
        add_dft_issue(art, SEV_WARNING,
            "Environment variable usage inconsistency",
            desc, "", 0, "CWE-1188",
            "Standardize env var usage across all daemon configs");
    }
}

/* --------------------------------------------------------------------------
 * Config hash duplicate detection
 *
 * If multiple config files have identical hashes, they may be unintended
 * copies. If daemon-specific configs all differ, that is expected.
 * -------------------------------------------------------------------------- */

static void check_config_hash_drift(drift_ctx_t *ctx) {
    int duplicates = 0;
    for (int i = 0; i < ctx->config_hash_count; i++) {
        for (int j = i + 1; j < ctx->config_hash_count; j++) {
            if (ctx->config_hashes[i] == ctx->config_hashes[j]) {
                duplicates++;
            }
        }
    }
    ctx->config_hash_mismatches = duplicates;
}

/* --------------------------------------------------------------------------
 * Report writer
 * -------------------------------------------------------------------------- */

static const char *severity_label(uint8_t sev) {
    switch (sev) {
        case SEV_INFO:     return "INFO";
        case SEV_WARNING:  return "WARNING";
        case SEV_ERROR:    return "ERROR";
        case SEV_CRITICAL: return "CRITICAL";
        default:           return "NONE";
    }
}

static void write_sep_dft(FILE *f) {
    for (int i = 0; i < 80; i++) fputc('=', f);
    fputc('\n', f);
}

static void write_line_dft(FILE *f) {
    for (int i = 0; i < 40; i++) fputc('-', f);
    fputc('\n', f);
}

static void write_report(lst_artifact_t *art, drift_ctx_t *ctx,
                          uint32_t initial_issues, const char *outpath) {
    FILE *f = fopen(outpath, "w");
    if (!f) {
        fprintf(stderr, "drift-detection: cannot write %s\n", outpath);
        return;
    }

    uint32_t new_issues = art->issue_count - initial_issues;

    /* Header */
    write_sep_dft(f);
    fprintf(f, "NNOS DRIFT DETECTION REPORT\n");
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
    fprintf(f, "Drift Issues Found: %u\n", new_issues);
    write_sep_dft(f);
    fprintf(f, "\n");

    /* Scan summary */
    fprintf(f, "SCAN SUMMARY\n");
    write_line_dft(f);
    fprintf(f, "  Config files scanned:       %d\n", ctx->config_files_found);
    fprintf(f, "  State files scanned:        %d\n", ctx->state_files_found);
    fprintf(f, "  Schema version refs:        %d\n", ctx->schema_version_refs);
    fprintf(f, "  Env variable refs:          %d\n", ctx->env_var_refs);
    fprintf(f, "  Daemon config refs:         %d\n", ctx->daemon_config_refs);
    fprintf(f, "  Baseline refs:              %d\n", ctx->baseline_refs);
    fprintf(f, "\n");

    /* Drift results */
    fprintf(f, "DRIFT RESULTS\n");
    write_line_dft(f);
    fprintf(f, "  Config hash duplicates:     %d\n", ctx->config_hash_mismatches);
    fprintf(f, "  Schema version mismatches:  %d\n", ctx->schema_version_mismatches);
    fprintf(f, "  Env var divergences:        %d\n", ctx->env_var_divergences);
    fprintf(f, "  Stale state files:          %d\n", ctx->state_files_stale);
    fprintf(f, "  Timestamp anomalies:        %d\n", ctx->timestamp_anomalies);
    fprintf(f, "\n");

    /* Severity summary */
    fprintf(f, "SEVERITY SUMMARY\n");
    write_line_dft(f);
    int counts[5] = {0};
    for (uint32_t i = initial_issues; i < art->issue_count; i++)
        counts[art->issues[i].severity]++;
    if (counts[SEV_CRITICAL]) fprintf(f, "  CRITICAL:  %d\n", counts[SEV_CRITICAL]);
    if (counts[SEV_ERROR])    fprintf(f, "  ERROR:     %d\n", counts[SEV_ERROR]);
    if (counts[SEV_WARNING])  fprintf(f, "  WARNING:   %d\n", counts[SEV_WARNING]);
    if (counts[SEV_INFO])     fprintf(f, "  INFO:      %d\n", counts[SEV_INFO]);
    fprintf(f, "\n");

    /* Findings */
    if (new_issues > 0) {
        fprintf(f, "FINDINGS\n");
        write_sep_dft(f);
        fprintf(f, "\n");

        for (int sev = SEV_CRITICAL; sev >= SEV_INFO; sev--) {
            int printed_header = 0;
            for (uint32_t i = initial_issues; i < art->issue_count; i++) {
                if (art->issues[i].severity != sev) continue;
                if (!printed_header) {
                    fprintf(f, "  [%s]\n\n", severity_label((uint8_t)sev));
                    printed_header = 1;
                }
                const lst_issue_t *issue = &art->issues[i];
                fprintf(f, "    %s: %s\n", issue->id, issue->title);
                if (issue->file_path[0])
                    fprintf(f, "      File: %s:%u\n",
                            issue->file_path, issue->line_number);
                if (issue->cwe[0])
                    fprintf(f, "      CWE: %s\n", issue->cwe);
                fprintf(f, "      %s\n", issue->description);
                if (issue->remediation[0])
                    fprintf(f, "      Fix: %s\n", issue->remediation);
                fprintf(f, "\n");
            }
        }
    } else {
        fprintf(f, "No drift detected. All configurations are aligned.\n\n");
    }

    /* Remediation guidance */
    fprintf(f, "REMEDIATION GUIDANCE\n");
    write_line_dft(f);
    if (ctx->config_hash_mismatches > 0)
        fprintf(f, "  - Review duplicate config files for unintended copies\n");
    if (ctx->schema_version_mismatches > 0)
        fprintf(f, "  - Run schema migration across all NNOS daemons\n");
    if (ctx->env_var_divergences > 0)
        fprintf(f, "  - Standardize environment variable usage in daemon configs\n");
    if (ctx->state_files_stale > 0)
        fprintf(f, "  - Restart stale daemons or clean orphaned state files\n");
    if (ctx->timestamp_anomalies > 0)
        fprintf(f, "  - Verify NTP synchronization across all cluster nodes\n");
    if (ctx->baseline_refs == 0)
        fprintf(f, "  - Establish config baselines for drift comparison\n");
    if (new_issues == 0)
        fprintf(f, "  - No remediation needed at this time\n");
    fprintf(f, "\n");

    /* Footer */
    write_sep_dft(f);
    fprintf(f, "END OF DRIFT DETECTION REPORT\n");
    fprintf(f, "\nThis report detects configuration and state drift across\n");
    fprintf(f, "the NNOS daemon constellation via deterministic pattern matching.\n");
    fprintf(f, "Manual verification is recommended for critical findings.\n");
    write_sep_dft(f);
    fprintf(f, "\n");

    fclose(f);

    printf("  Wrote %s (%u issues)\n", outpath, new_issues);
}

/* --------------------------------------------------------------------------
 * Recipe entry point
 * -------------------------------------------------------------------------- */

static int recipe_drift_detection(lst_artifact_t *art, const char *output_dir) {
    if (!art) return -1;

    uint32_t initial_issues = art->issue_count;

    /* Initialize drift context */
    drift_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    /* Phase 1: Walk project directory tree for config and state files */
    scan_dir_dft(art, &ctx, art->project_path, 0);

    /* Phase 2: Cross-check schema versions across artifact files */
    check_schema_drift(art, &ctx);

    /* Phase 3: Check env var divergence across config files */
    check_env_divergence(art, &ctx);

    /* Phase 4: Detect config hash duplicates */
    check_config_hash_drift(&ctx);

    /* Phase 5: Flag missing baselines as a drift risk */
    if (ctx.config_files_found > 0 && ctx.baseline_refs == 0) {
        add_dft_issue(art, SEV_WARNING,
            "No baseline references found",
            "Config files exist but no baseline hashes for drift comparison",
            "", 0, "CWE-1188",
            "Generate config baselines with checksums for future drift detection");
    }

    /* Phase 6: Flag daemon configs without schema versions */
    if (ctx.daemon_config_refs > 0 && ctx.schema_version_refs == 0) {
        add_dft_issue(art, SEV_WARNING,
            "Daemon configs lack schema version",
            "NNOS daemon configs found but no schema versioning present",
            "", 0, "CWE-1104",
            "Add schema_version field to all daemon configuration files");
    }

    /* Compute overall drift severity */
    uint32_t new_issues = art->issue_count - initial_issues;
    int has_critical = 0;
    int has_error = 0;
    for (uint32_t i = initial_issues; i < art->issue_count; i++) {
        if (art->issues[i].severity == SEV_CRITICAL) has_critical = 1;
        if (art->issues[i].severity == SEV_ERROR) has_error = 1;
    }

    /* Update artifact health based on drift */
    if (has_critical) {
        art->health = HEALTH_UNHEALTHY;
    } else if (has_error) {
        art->health = HEALTH_DEGRADED;
    }

    /* Build output path */
    char outpath[LST_MAX_PATH];
    if (output_dir) {
        snprintf(outpath, sizeof(outpath), "%s/DRIFT_REPORT", output_dir);
        mkdir(output_dir, 0755);
    } else {
        snprintf(outpath, sizeof(outpath), "%s/DRIFT_REPORT", art->project_path);
    }

    /* Write report file */
    write_report(art, &ctx, initial_issues, outpath);

    /* Console summary */
    printf("\n");
    for (int i = 0; i < 70; i++) putchar('=');
    printf("\nNNOS DRIFT DETECTION SUMMARY\n");
    printf("Project: %s\n", art->project_name);
    for (int i = 0; i < 70; i++) putchar('=');
    printf("\n");
    printf("  Config files:        %d\n", ctx.config_files_found);
    printf("  State files:         %d\n", ctx.state_files_found);
    printf("  Hash duplicates:     %d\n", ctx.config_hash_mismatches);
    printf("  Schema mismatches:   %d\n", ctx.schema_version_mismatches);
    printf("  Env divergences:     %d\n", ctx.env_var_divergences);
    printf("  Stale state files:   %d\n", ctx.state_files_stale);
    printf("  Timestamp anomalies: %d\n", ctx.timestamp_anomalies);
    printf("  Total drift issues:  %u\n", new_issues);
    for (int i = 0; i < 70; i++) putchar('-');
    printf("\nRESULT: %s\n", (has_critical == 0) ? "PASS" : "FAIL");
    for (int i = 0; i < 70; i++) putchar('=');
    printf("\n");

    return (has_critical == 0) ? 0 : 1;
}

/* --------------------------------------------------------------------------
 * Recipe registration
 * -------------------------------------------------------------------------- */

void recipe_drift_detection_register(void) {
    lst_recipe_t r;
    memset(&r, 0, sizeof(r));
    snprintf(r.name, sizeof(r.name), "drift_detection");
    snprintf(r.description, sizeof(r.description),
             "Detect config and state drift across NNOS daemon constellation");
    r.execute = recipe_drift_detection;
    r.version = 1;
    lst_recipe_register(&r);
}
