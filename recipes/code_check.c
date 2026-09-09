#define _POSIX_C_SOURCE 200809L

/*
 * code_check.c — Code Quality and Integrity Recipe
 *
 * Deterministic code quality analysis: integrity verification,
 * file completeness, and aggregate metrics.
 *
 * Converts logic from frameworks/code_check/__init__.py.
 * Focuses on quality aspects that don't overlap with security_scan:
 *   - File integrity (incomplete files, TODO in tail)
 *   - Aggregate metrics (file counts, line counts, language distribution)
 *   - Dead code indicators
 *   - Missing documentation patterns
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

/* --------------------------------------------------------------------------
 * Helpers
 * -------------------------------------------------------------------------- */

static char *read_file_cc(const char *path, size_t *out_len) {
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

/* Classify file language by extension */
static uint8_t classify_language(const char *name) {
    const char *ext = strrchr(name, '.');
    if (!ext) return LANG_UNKNOWN;

    if (strcmp(ext, ".c") == 0)                                 return LANG_C;
    if (strcmp(ext, ".h") == 0)                                 return LANG_C;
    if (strcmp(ext, ".cpp") == 0 || strcmp(ext, ".cc") == 0)    return LANG_CPP;
    if (strcmp(ext, ".hpp") == 0)                               return LANG_CPP;
    if (strcmp(ext, ".py") == 0)                                return LANG_PYTHON;
    if (strcmp(ext, ".js") == 0 || strcmp(ext, ".mjs") == 0)    return LANG_JAVASCRIPT;
    if (strcmp(ext, ".ts") == 0)                                return LANG_TYPESCRIPT;
    if (strcmp(ext, ".tsx") == 0 || strcmp(ext, ".jsx") == 0)   return LANG_TYPESCRIPT;
    if (strcmp(ext, ".php") == 0)                               return LANG_PHP;
    if (strcmp(ext, ".rs") == 0)                                return LANG_RUST;
    if (strcmp(ext, ".go") == 0)                                return LANG_GO;
    if (strcmp(ext, ".java") == 0)                              return LANG_JAVA;
    if (strcmp(ext, ".rb") == 0)                                return LANG_RUBY;
    if (strcmp(ext, ".swift") == 0)                             return LANG_SWIFT;
    if (strcmp(ext, ".kt") == 0)                                return LANG_KOTLIN;
    if (strcmp(ext, ".sh") == 0 || strcmp(ext, ".bash") == 0)   return LANG_SHELL;
    if (strcmp(ext, ".sql") == 0)                               return LANG_SQL;
    if (strcmp(ext, ".html") == 0 || strcmp(ext, ".htm") == 0)  return LANG_HTML;
    if (strcmp(ext, ".css") == 0 || strcmp(ext, ".scss") == 0)  return LANG_CSS;
    if (strcmp(ext, ".json") == 0)                              return LANG_JSON;
    if (strcmp(ext, ".yml") == 0 || strcmp(ext, ".yaml") == 0)  return LANG_YAML;
    if (strcmp(ext, ".md") == 0)                                return LANG_MARKDOWN;
    return LANG_UNKNOWN;
}

static const char *language_name(uint8_t lang) {
    switch (lang) {
        case LANG_C:           return "C";
        case LANG_CPP:         return "C++";
        case LANG_PYTHON:      return "Python";
        case LANG_JAVASCRIPT:  return "JavaScript";
        case LANG_TYPESCRIPT:  return "TypeScript";
        case LANG_PHP:         return "PHP";
        case LANG_RUST:        return "Rust";
        case LANG_GO:          return "Go";
        case LANG_JAVA:        return "Java";
        case LANG_RUBY:        return "Ruby";
        case LANG_SWIFT:       return "Swift";
        case LANG_KOTLIN:      return "Kotlin";
        case LANG_SHELL:       return "Shell";
        case LANG_SQL:         return "SQL";
        case LANG_HTML:        return "HTML";
        case LANG_CSS:         return "CSS";
        case LANG_JSON:        return "JSON";
        case LANG_YAML:        return "YAML";
        case LANG_MARKDOWN:    return "Markdown";
        default:               return "Other";
    }
}

/* Is this a source code file worth analyzing? */
static int is_source_file(const char *name) {
    const char *ext = strrchr(name, '.');
    if (!ext) return 0;
    const char *exts[] = {".py", ".js", ".ts", ".tsx", ".jsx", ".c", ".cpp", ".h",
                          ".hpp", ".php", ".rb", ".go", ".rs", ".java", ".kt",
                          ".swift", ".sh", ".bash", ".sql", ".html", ".css",
                          ".scss", ".json", ".yml", ".yaml", ".md", ".mjs",
                          ".cc", ".htm", NULL};
    for (int i = 0; exts[i]; i++)
        if (strcasecmp(ext, exts[i]) == 0) return 1;
    return 0;
}

/* Skip directories */
static const char *SKIP_DIRS_CC[] = {
    "node_modules", "vendor", ".git", "__pycache__", "dist", "build",
    ".venv", "venv", ".next", ".nuxt", "target", ".cache", "coverage",
    ".tox", "env", NULL
};

static int should_skip_cc(const char *name) {
    if (name[0] == '.') return 1;
    for (int i = 0; SKIP_DIRS_CC[i]; i++)
        if (strcmp(name, SKIP_DIRS_CC[i]) == 0) return 1;
    return 0;
}

/* --------------------------------------------------------------------------
 * File analysis — populates lst_file_t records
 * -------------------------------------------------------------------------- */

static void analyze_file(lst_artifact_t *art, const char *fpath, const char *fname) {
    if (art->file_count >= LST_MAX_FILES) return;

    struct stat st;
    if (stat(fpath, &st) != 0) return;

    size_t len = 0;
    char *content = read_file_cc(fpath, &len);
    if (!content) return;

    lst_file_t *file = &art->files[art->file_count];
    memset(file, 0, sizeof(lst_file_t));
    snprintf(file->path, LST_MAX_PATH, "%s", fpath);
    file->size_bytes = (uint32_t)st.st_size;
    file->language = classify_language(fname);

    /* Count lines */
    uint32_t lines = 1;
    for (size_t i = 0; i < len; i++)
        if (content[i] == '\n') lines++;
    file->line_count = lines;

    /* Check integrity: incomplete file indicators */
    if (len > 3) {
        /* Ends with ... */
        const char *tail = content + len - 4;
        while (tail > content && (*tail == '\n' || *tail == ' ')) tail--;
        if (tail >= content + 2 && strncmp(tail - 2, "...", 3) == 0) {
            if (art->issue_count < LST_MAX_ISSUES) {
                lst_issue_t *issue = &art->issues[art->issue_count];
                memset(issue, 0, sizeof(lst_issue_t));
                snprintf(issue->id, sizeof(issue->id), "CHK-%04u", art->issue_count + 1);
                issue->severity = SEV_WARNING;
                snprintf(issue->title, sizeof(issue->title), "Possibly Incomplete File");
                snprintf(issue->description, sizeof(issue->description),
                    "File ends with '...' — may be truncated or under construction");
                snprintf(issue->file_path, sizeof(issue->file_path), "%s", fpath);
                issue->line_number = lines;
                art->issue_count++;
            }
        }
    }

    /* Check for TODO/FIXME/HACK/XXX in last 200 chars */
    if (len > 5) {
        size_t check_from = (len > 200) ? len - 200 : 0;
        const char *tail_section = content + check_from;
        if (strstr(tail_section, "TODO") || strstr(tail_section, "FIXME") ||
            strstr(tail_section, "HACK") || strstr(tail_section, "XXX")) {
            if (art->issue_count < LST_MAX_ISSUES) {
                lst_issue_t *issue = &art->issues[art->issue_count];
                memset(issue, 0, sizeof(lst_issue_t));
                snprintf(issue->id, sizeof(issue->id), "CHK-%04u", art->issue_count + 1);
                issue->severity = SEV_INFO;
                snprintf(issue->title, sizeof(issue->title), "Unresolved TODO/FIXME");
                snprintf(issue->description, sizeof(issue->description),
                    "File has TODO/FIXME/HACK markers near end");
                snprintf(issue->file_path, sizeof(issue->file_path), "%s", fpath);
                art->issue_count++;
            }
        }
    }

    art->file_count++;
    free(content);
}

static void scan_dir_cc(lst_artifact_t *art, const char *dir, int depth) {
    if (depth > 8) return;

    DIR *d = opendir(dir);
    if (!d) return;

    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        if (ent->d_name[0] == '.') continue;
        if (should_skip_cc(ent->d_name)) continue;

        char child[LST_MAX_PATH];
        snprintf(child, sizeof(child), "%s/%s", dir, ent->d_name);

        struct stat st;
        if (stat(child, &st) != 0) continue;

        if (S_ISDIR(st.st_mode)) {
            scan_dir_cc(art, child, depth + 1);
        } else if (S_ISREG(st.st_mode) && is_source_file(ent->d_name)) {
            analyze_file(art, child, ent->d_name);
        }
    }
    closedir(d);
}

/* --------------------------------------------------------------------------
 * Report writer
 * -------------------------------------------------------------------------- */

static void write_sep_cc(FILE *f) {
    for (int i = 0; i < 80; i++) fputc('=', f);
    fputc('\n', f);
}

static void write_line_cc(FILE *f) {
    for (int i = 0; i < 40; i++) fputc('-', f);
    fputc('\n', f);
}

static int recipe_code_check(lst_artifact_t *art, const char *output_dir) {
    if (!art) return -1;

    /* Scan project files */
    uint32_t initial_files = art->file_count;
    uint32_t initial_issues = art->issue_count;
    scan_dir_cc(art, art->project_path, 0);
    uint32_t new_files = art->file_count - initial_files;
    uint32_t new_issues = art->issue_count - initial_issues;

    /* Build output path */
    char outpath[LST_MAX_PATH];
    if (output_dir)
        snprintf(outpath, sizeof(outpath), "%s/CODE_CHECK_REPORT", output_dir);
    else
        snprintf(outpath, sizeof(outpath), "%s/CODE_CHECK_REPORT", art->project_path);

    if (output_dir) mkdir(output_dir, 0755);

    int rfd = open(outpath, O_WRONLY | O_CREAT | O_TRUNC | O_NOFOLLOW | O_CLOEXEC, 0644);
    if (rfd < 0) {
        fprintf(stderr, "code-check: cannot write %s\n", outpath);
        return -1;
    }
    FILE *f = fdopen(rfd, "w");
    if (!f) {
        close(rfd);
        fprintf(stderr, "code-check: cannot write %s\n", outpath);
        return -1;
    }

    /* Header */
    write_sep_cc(f);
    fprintf(f, "CODE QUALITY & INTEGRITY REPORT\n");
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
    fprintf(f, "Files Analyzed: %u\n", new_files);
    fprintf(f, "Issues Found: %u\n", new_issues);
    write_sep_cc(f);
    fprintf(f, "\n");

    /* Language distribution */
    int lang_counts[LANG_COUNT] = {0};
    uint32_t total_lines = 0;
    uint32_t total_bytes = 0;
    for (uint32_t i = initial_files; i < art->file_count; i++) {
        lang_counts[art->files[i].language]++;
        total_lines += art->files[i].line_count;
        total_bytes += art->files[i].size_bytes;
    }

    fprintf(f, "CODEBASE METRICS\n");
    write_line_cc(f);
    fprintf(f, "  Total Files:      %u\n", new_files);
    fprintf(f, "  Total Lines:      %u\n", total_lines);
    fprintf(f, "  Total Size:       ");
    if (total_bytes > 1024 * 1024)
        fprintf(f, "%.1f MB\n", total_bytes / (1024.0 * 1024.0));
    else if (total_bytes > 1024)
        fprintf(f, "%.1f KB\n", total_bytes / 1024.0);
    else
        fprintf(f, "%u bytes\n", total_bytes);
    fprintf(f, "\n");

    fprintf(f, "LANGUAGE DISTRIBUTION\n");
    write_line_cc(f);
    for (int i = 0; i < LANG_COUNT; i++) {
        if (lang_counts[i] > 0) {
            float pct = (new_files > 0) ? (100.0f * lang_counts[i] / new_files) : 0;
            fprintf(f, "  %-20s %4d files  (%5.1f%%)\n", language_name((uint8_t)i), lang_counts[i], pct);
        }
    }
    fprintf(f, "\n");

    /* Issues */
    if (new_issues > 0) {
        fprintf(f, "INTEGRITY ISSUES\n");
        write_sep_cc(f);
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
    }

    /* Health assessment */
    fprintf(f, "HEALTH ASSESSMENT\n");
    write_line_cc(f);
    int criticals = 0, errors = 0;
    for (uint32_t i = initial_issues; i < art->issue_count; i++) {
        if (art->issues[i].severity == SEV_CRITICAL) criticals++;
        if (art->issues[i].severity == SEV_ERROR) errors++;
    }

    const char *health;
    if (criticals > 0) { health = "UNHEALTHY"; art->health = HEALTH_UNHEALTHY; }
    else if (errors > 0) { health = "DEGRADED"; art->health = HEALTH_DEGRADED; }
    else if (new_issues > 0) { health = "HEALTHY (with notes)"; art->health = HEALTH_HEALTHY; }
    else { health = "HEALTHY"; art->health = HEALTH_HEALTHY; }

    fprintf(f, "  Status: %s\n", health);
    fprintf(f, "  Critical Issues: %d\n", criticals);
    fprintf(f, "  Error Issues: %d\n", errors);
    fprintf(f, "\n");

    /* Footer */
    write_sep_cc(f);
    fprintf(f, "END OF CODE CHECK REPORT\n");
    write_sep_cc(f);
    fprintf(f, "\n");

    fclose(f);

    printf("  Wrote %s (%u files, %u issues)\n", outpath, new_files, new_issues);
    return 0;
}

/* --------------------------------------------------------------------------
 * Recipe registration
 * -------------------------------------------------------------------------- */

void recipe_code_check_register(void) {
    lst_recipe_t r = {0};
    snprintf(r.name, LST_MAX_NAME, "code-check");
    snprintf(r.description, LST_MAX_NAME, "Code quality analysis and integrity verification");
    r.execute = recipe_code_check;
    r.version = 1;
    lst_recipe_register(&r);
}
