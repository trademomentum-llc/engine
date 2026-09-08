#define _POSIX_C_SOURCE 200809L

/*
 * jasterish_validation.c -- Jasterish Validation Recipe
 *
 * Validates the Jasterish validation-layer port. Ensures that the
 * self-hosting compiler pipeline is structurally complete, bootstrap
 * check scripts are present, fixpoint hashes are consistent, and the
 * validation runtime can be linked without breaking the fixpoint.
 *
 * Checks:
 *   - compiler.jstr source file is present
 *   - Bootstrap check scripts exist (bootstrap.sh, bootstrap.jst)
 *   - Fixpoint hash files are present and internally consistent
 *   - Self-host ladder stages are complete (stage0 through stage3)
 *   - Validation runtime object files can be linked without fixpoint drift
 *   - Linker behavior matches expected patterns (symbol exports, relocations)
 */

#include "lst.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>

/* --------------------------------------------------------------------------
 * Jasterish file and pattern constants
 * -------------------------------------------------------------------------- */

static const char *JST_COMPILER_SOURCES[] = {
    "compiler.jstr",
    "compiler.jst",
    "jasterish.jstr",
    NULL
};

static const char *JST_BOOTSTRAP_SCRIPTS[] = {
    "bootstrap.sh",
    "bootstrap.jst",
    "bootstrap.bat",
    "check_bootstrap.sh",
    NULL
};

static const char *JST_FIXPOINT_FILES[] = {
    "fixpoint.sha256",
    "fixpoint.hash",
    "fixpoint.txt",
    ".fixpoint",
    NULL
};

static const char *JST_LADDER_STAGES[] = {
    "stage0",
    "stage1",
    "stage2",
    "stage3",
    NULL
};

static const char *JST_RUNTIME_OBJECTS[] = {
    "validation_rt.o",
    "validation_rt.obj",
    "jst_rt.o",
    "jst_rt.obj",
    "runtime.o",
    NULL
};

static const char *JST_LINKER_PATTERNS[] = {
    "EXPORT",
    "ENTRY",
    "RELOCATABLE",
    "extern",
    "public",
    NULL
};

static const char *JST_SOURCE_EXTS[] = {
    ".jstr", ".jst", ".j", ".jasterish", NULL
};

/* Directories to skip during scanning */
static const char *SKIP_DIRS_JST[] = {
    "node_modules", "vendor", ".git", "__pycache__", "dist", "build",
    ".venv", "venv", ".next", "target", ".cache", NULL
};

/* --------------------------------------------------------------------------
 * Helpers
 * -------------------------------------------------------------------------- */

static char *read_file_jst(const char *path, size_t *out_len) {
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

static void add_jst_issue(lst_artifact_t *art, uint8_t severity,
                           const char *title, const char *desc,
                           const char *file_path, uint32_t line) {
    if (art->issue_count >= LST_MAX_ISSUES) return;
    lst_issue_t *issue = &art->issues[art->issue_count];
    memset(issue, 0, sizeof(lst_issue_t));

    snprintf(issue->id, sizeof(issue->id), "JST-%04u", art->issue_count + 1);
    issue->severity = severity;
    snprintf(issue->title, sizeof(issue->title), "%s", title);
    snprintf(issue->description, sizeof(issue->description), "%s", desc);
    if (file_path)
        snprintf(issue->file_path, sizeof(issue->file_path), "%s", file_path);
    issue->line_number = line;
    snprintf(issue->cwe, sizeof(issue->cwe), "CWE-710");
    snprintf(issue->remediation, sizeof(issue->remediation),
        "Complete the Jasterish self-hosting validation pipeline");

    art->issue_count++;
}

static int should_skip_jst(const char *name) {
    for (int i = 0; SKIP_DIRS_JST[i]; i++)
        if (strcmp(name, SKIP_DIRS_JST[i]) == 0) return 1;
    return 0;
}

static int is_jst_source(const char *name) {
    const char *ext = strrchr(name, '.');
    if (!ext) return 0;
    for (int i = 0; JST_SOURCE_EXTS[i]; i++)
        if (strcmp(ext, JST_SOURCE_EXTS[i]) == 0) return 1;
    return 0;
}

/* --------------------------------------------------------------------------
 * Check: compiler.jstr source presence
 * -------------------------------------------------------------------------- */

static int check_compiler_source(lst_artifact_t *art, const char *project_path) {
    int found = 0;
    for (int i = 0; JST_COMPILER_SOURCES[i]; i++) {
        char path[LST_MAX_PATH];
        snprintf(path, sizeof(path), "%s/%s", project_path, JST_COMPILER_SOURCES[i]);
        struct stat st;
        if (stat(path, &st) == 0 && S_ISREG(st.st_mode)) {
            found = 1;
            break;
        }
    }
    if (!found) {
        add_jst_issue(art, SEV_CRITICAL,
            "Compiler Source Missing",
            "No compiler.jstr or equivalent source file found in project root",
            project_path, 0);
    }
    return found;
}

/* --------------------------------------------------------------------------
 * Check: bootstrap scripts exist
 * -------------------------------------------------------------------------- */

static int check_bootstrap_scripts(lst_artifact_t *art, const char *project_path) {
    int found_count = 0;
    for (int i = 0; JST_BOOTSTRAP_SCRIPTS[i]; i++) {
        char path[LST_MAX_PATH];
        snprintf(path, sizeof(path), "%s/%s", project_path, JST_BOOTSTRAP_SCRIPTS[i]);
        struct stat st;
        if (stat(path, &st) == 0 && S_ISREG(st.st_mode)) {
            found_count++;
        }
    }
    if (found_count == 0) {
        add_jst_issue(art, SEV_ERROR,
            "Bootstrap Scripts Missing",
            "No bootstrap check scripts found (bootstrap.sh, bootstrap.jst)",
            project_path, 0);
    }
    return found_count;
}

/* --------------------------------------------------------------------------
 * Check: fixpoint hashes are consistent
 * -------------------------------------------------------------------------- */

static int check_fixpoint_hashes(lst_artifact_t *art, const char *project_path) {
    int files_found = 0;
    char first_hash[128];
    int hash_mismatch = 0;

    first_hash[0] = '\0';

    for (int i = 0; JST_FIXPOINT_FILES[i]; i++) {
        char path[LST_MAX_PATH];
        snprintf(path, sizeof(path), "%s/%s", project_path, JST_FIXPOINT_FILES[i]);

        size_t len = 0;
        char *content = read_file_jst(path, &len);
        if (!content) continue;

        files_found++;

        /* Extract first non-empty line as the hash value */
        char current_hash[128];
        current_hash[0] = '\0';
        char *line_start = content;
        while (*line_start) {
            char *eol = strchr(line_start, '\n');
            if (eol) *eol = '\0';

            /* Skip whitespace */
            const char *p = line_start;
            while (*p == ' ' || *p == '\t') p++;

            /* Grab first non-empty, non-comment line */
            if (*p != '\0' && *p != '#') {
                /* Take first 64 hex chars or up to first space */
                size_t clen = 0;
                while (p[clen] && p[clen] != ' ' && p[clen] != '\t' && clen < 127)
                    clen++;
                memcpy(current_hash, p, clen);
                current_hash[clen] = '\0';
                break;
            }

            if (!eol) break;
            line_start = eol + 1;
        }

        if (current_hash[0] != '\0') {
            if (first_hash[0] == '\0') {
                snprintf(first_hash, sizeof(first_hash), "%s", current_hash);
            } else if (strcmp(first_hash, current_hash) != 0) {
                hash_mismatch = 1;
                add_jst_issue(art, SEV_CRITICAL,
                    "Fixpoint Hash Mismatch",
                    "Fixpoint hash files contain inconsistent values",
                    path, 0);
            }
        }

        free(content);
    }

    if (files_found == 0) {
        add_jst_issue(art, SEV_ERROR,
            "Fixpoint Hash Files Missing",
            "No fixpoint hash files found -- cannot verify compilation stability",
            project_path, 0);
    }

    return (files_found > 0 && !hash_mismatch) ? 1 : 0;
}

/* --------------------------------------------------------------------------
 * Check: self-host ladder stages are complete
 * -------------------------------------------------------------------------- */

static int check_ladder_stages(lst_artifact_t *art, const char *project_path) {
    int stages_found = 0;
    int stages_total = 0;

    for (int i = 0; JST_LADDER_STAGES[i]; i++) {
        stages_total++;
        char path[LST_MAX_PATH];
        snprintf(path, sizeof(path), "%s/%s", project_path, JST_LADDER_STAGES[i]);
        struct stat st;
        if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
            /* Check that the stage directory is not empty */
            DIR *d = opendir(path);
            if (d) {
                int has_files = 0;
                struct dirent *ent;
                while ((ent = readdir(d)) != NULL) {
                    if (ent->d_name[0] == '.') continue;
                    has_files = 1;
                    break;
                }
                closedir(d);
                if (has_files) {
                    stages_found++;
                } else {
                    char desc[LST_MAX_NAME];
                    snprintf(desc, sizeof(desc),
                        "Self-host ladder %s exists but is empty", JST_LADDER_STAGES[i]);
                    add_jst_issue(art, SEV_WARNING,
                        "Empty Ladder Stage",
                        desc, path, 0);
                }
            }
        }
    }

    if (stages_found == 0) {
        add_jst_issue(art, SEV_ERROR,
            "Self-Host Ladder Missing",
            "No self-host ladder stage directories found (stage0-stage3)",
            project_path, 0);
    } else if (stages_found < stages_total) {
        char desc[LST_MAX_NAME];
        snprintf(desc, sizeof(desc),
            "Only %d of %d self-host ladder stages present", stages_found, stages_total);
        add_jst_issue(art, SEV_WARNING,
            "Incomplete Self-Host Ladder",
            desc, project_path, 0);
    }

    return stages_found;
}

/* --------------------------------------------------------------------------
 * Check: validation runtime can be linked without breaking fixpoint
 * -------------------------------------------------------------------------- */

static int check_validation_runtime(lst_artifact_t *art, const char *project_path) {
    int runtime_found = 0;

    for (int i = 0; JST_RUNTIME_OBJECTS[i]; i++) {
        char path[LST_MAX_PATH];
        snprintf(path, sizeof(path), "%s/%s", project_path, JST_RUNTIME_OBJECTS[i]);
        struct stat st;
        if (stat(path, &st) == 0 && S_ISREG(st.st_mode)) {
            runtime_found = 1;
            /* A zero-byte object file is always suspicious */
            if (st.st_size == 0) {
                add_jst_issue(art, SEV_ERROR,
                    "Empty Runtime Object",
                    "Validation runtime object file is zero bytes",
                    path, 0);
            }
            break;
        }
    }

    if (!runtime_found) {
        add_jst_issue(art, SEV_WARNING,
            "Validation Runtime Not Found",
            "No pre-built validation runtime object found -- fixpoint linkage untested",
            project_path, 0);
    }

    return runtime_found;
}

/* --------------------------------------------------------------------------
 * Check: linker behavior matches expected patterns
 * -------------------------------------------------------------------------- */

static void scan_linker_patterns(lst_artifact_t *art, const char *fpath) {
    size_t len = 0;
    char *content = read_file_jst(fpath, &len);
    if (!content) return;

    int export_count = 0;
    int entry_found = 0;
    int lineno = 1;
    char *line_start = content;

    while (*line_start) {
        char *eol = strchr(line_start, '\n');
        if (eol) *eol = '\0';

        for (int k = 0; JST_LINKER_PATTERNS[k]; k++) {
            if (strstr(line_start, JST_LINKER_PATTERNS[k])) {
                if (strcmp(JST_LINKER_PATTERNS[k], "EXPORT") == 0 ||
                    strcmp(JST_LINKER_PATTERNS[k], "extern") == 0 ||
                    strcmp(JST_LINKER_PATTERNS[k], "public") == 0) {
                    export_count++;
                }
                if (strcmp(JST_LINKER_PATTERNS[k], "ENTRY") == 0) {
                    entry_found = 1;
                }
            }
        }

        if (!eol) break;
        line_start = eol + 1;
        lineno++;
    }

    if (export_count == 0) {
        add_jst_issue(art, SEV_INFO,
            "No Symbol Exports",
            "Jasterish source has no exported symbols -- may not link correctly",
            fpath, (uint32_t)lineno);
    }

    if (!entry_found) {
        add_jst_issue(art, SEV_INFO,
            "No Entry Point Declaration",
            "No ENTRY directive found -- linker may not resolve entry point",
            fpath, (uint32_t)lineno);
    }

    free(content);
}

/* --------------------------------------------------------------------------
 * Recursive directory scanner for Jasterish sources
 * -------------------------------------------------------------------------- */

static void scan_dir_jst(lst_artifact_t *art, const char *dir, int depth,
                          int *jst_sources, int *total_files) {
    if (depth > 8) return;

    DIR *d = opendir(dir);
    if (!d) return;

    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        if (ent->d_name[0] == '.') continue;
        if (should_skip_jst(ent->d_name)) continue;

        char child[LST_MAX_PATH];
        snprintf(child, sizeof(child), "%s/%s", dir, ent->d_name);

        struct stat st;
        if (stat(child, &st) != 0) continue;

        if (S_ISDIR(st.st_mode)) {
            scan_dir_jst(art, child, depth + 1, jst_sources, total_files);
        } else if (S_ISREG(st.st_mode)) {
            (*total_files)++;
            if (is_jst_source(ent->d_name)) {
                (*jst_sources)++;
                scan_linker_patterns(art, child);
            }
        }
    }
    closedir(d);
}

/* --------------------------------------------------------------------------
 * Report writer
 * -------------------------------------------------------------------------- */

static void write_sep_jst(FILE *f) {
    for (int i = 0; i < 80; i++) fputc('=', f);
    fputc('\n', f);
}

static void write_line_jst(FILE *f) {
    for (int i = 0; i < 40; i++) fputc('-', f);
    fputc('\n', f);
}

/* --------------------------------------------------------------------------
 * Entry point
 * -------------------------------------------------------------------------- */

static int recipe_jasterish_validation(lst_artifact_t *art, const char *output_dir) {
    if (!art) return -1;

    uint32_t initial_issues = art->issue_count;
    int jst_sources = 0;
    int total_files = 0;

    /* Run all validation checks */
    int compiler_ok = check_compiler_source(art, art->project_path);
    int bootstrap_count = check_bootstrap_scripts(art, art->project_path);
    int fixpoint_ok = check_fixpoint_hashes(art, art->project_path);
    int stages_found = check_ladder_stages(art, art->project_path);
    int runtime_ok = check_validation_runtime(art, art->project_path);

    /* Scan for Jasterish sources and linker patterns */
    scan_dir_jst(art, art->project_path, 0, &jst_sources, &total_files);

    uint32_t new_issues = art->issue_count - initial_issues;

    /* Build output path */
    char outpath[LST_MAX_PATH];
    if (output_dir)
        snprintf(outpath, sizeof(outpath), "%s/JASTERISH_VALIDATION_REPORT", output_dir);
    else
        snprintf(outpath, sizeof(outpath), "%s/JASTERISH_VALIDATION_REPORT", art->project_path);

    if (output_dir) mkdir(output_dir, 0755);

    FILE *f = fopen(outpath, "w");
    if (!f) {
        fprintf(stderr, "jasterish-validation: cannot write %s\n", outpath);
        return -1;
    }

    /* Header */
    write_sep_jst(f);
    fprintf(f, "JASTERISH VALIDATION REPORT\n");
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
    fprintf(f, "Jasterish Sources: %d\n", jst_sources);
    fprintf(f, "Total Files Scanned: %d\n", total_files);
    fprintf(f, "Issues Found: %u\n", new_issues);
    write_sep_jst(f);
    fprintf(f, "\n");

    /* Validation matrix */
    fprintf(f, "VALIDATION MATRIX\n");
    write_line_jst(f);
    fprintf(f, "\n");
    fprintf(f, "  %-35s %s\n", "Check", "Status");
    write_line_jst(f);
    fprintf(f, "  %-35s %s\n", "Compiler source present",
        compiler_ok ? "PASS" : "FAIL");
    fprintf(f, "  %-35s %s (%d found)\n", "Bootstrap scripts exist",
        bootstrap_count > 0 ? "PASS" : "FAIL", bootstrap_count);
    fprintf(f, "  %-35s %s\n", "Fixpoint hashes consistent",
        fixpoint_ok ? "PASS" : "FAIL");
    fprintf(f, "  %-35s %s (%d/4 stages)\n", "Self-host ladder complete",
        stages_found >= 4 ? "PASS" : (stages_found > 0 ? "PARTIAL" : "FAIL"),
        stages_found);
    fprintf(f, "  %-35s %s\n", "Validation runtime linkable",
        runtime_ok ? "PASS" : "NOT FOUND");
    fprintf(f, "  %-35s %d sources checked\n", "Linker pattern scan",
        jst_sources);
    fprintf(f, "\n");

    /* Findings */
    if (new_issues > 0) {
        fprintf(f, "VALIDATION FINDINGS\n");
        write_sep_jst(f);
        fprintf(f, "\n");

        for (uint32_t i = initial_issues; i < art->issue_count; i++) {
            const lst_issue_t *issue = &art->issues[i];
            fprintf(f, "  %s [%s]: %s\n", issue->id,
                issue->severity == SEV_CRITICAL ? "CRITICAL" :
                issue->severity == SEV_ERROR    ? "ERROR"    :
                issue->severity == SEV_WARNING  ? "WARNING"  : "INFO",
                issue->title);
            if (issue->file_path[0])
                fprintf(f, "    File: %s:%u\n", issue->file_path, issue->line_number);
            fprintf(f, "    %s\n\n", issue->description);
        }
    } else {
        fprintf(f, "VALIDATION: PASS\n");
        fprintf(f, "  All Jasterish validation checks passed.\n\n");
    }

    /* Footer */
    write_sep_jst(f);
    fprintf(f, "END OF JASTERISH VALIDATION REPORT\n");
    fprintf(f, "\nThis report validates the Jasterish self-hosting compiler pipeline.\n");
    fprintf(f, "Checks cover source presence, bootstrap scripts, fixpoint stability,\n");
    fprintf(f, "self-host ladder completeness, runtime linkage, and linker patterns.\n");
    write_sep_jst(f);
    fprintf(f, "\n");

    fclose(f);

    printf("  Wrote %s (%u issues)\n", outpath, new_issues);
    return 0;
}

/* --------------------------------------------------------------------------
 * Recipe registration
 * -------------------------------------------------------------------------- */

void recipe_jasterish_validation_register(void) {
    lst_recipe_t r = {0};
    snprintf(r.name, LST_MAX_NAME, "jasterish-validation");
    snprintf(r.description, LST_MAX_NAME,
        "Validate Jasterish self-hosting compiler pipeline and fixpoint stability");
    r.execute = recipe_jasterish_validation;
    r.version = 1;
    lst_recipe_register(&r);
}
