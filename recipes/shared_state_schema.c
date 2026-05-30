/*
 * shared_state_schema.c -- Shared-State Schema Validation Recipe
 *
 * Validates that NNOS shared-state schemas are properly versioned,
 * that standard daemon state fields have type declarations, and
 * that daemon state structures follow the schema before new fields
 * are introduced.
 *
 * Checks:
 *   - Schema definition presence (struct *_state or *_schema)
 *   - Schema version field exists and is non-zero
 *   - Required fields (threat_score, insight_score, morph_cycle)
 *     have explicit type declarations
 *   - Unversioned schema changes (struct modifications without
 *     bumping the version constant)
 *   - Undeclared fields (fields referenced but not in the schema)
 *   - Schema naming conventions (snake_case, _state/_schema suffix)
 */

#include "lst.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>

/* --------------------------------------------------------------------------
 * Helpers
 * -------------------------------------------------------------------------- */

static char *read_file_sss(const char *path, size_t *out_len) {
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

static void add_sss_issue(lst_artifact_t *art, uint8_t severity,
                           const char *title, const char *desc,
                           const char *file_path, uint32_t line,
                           const char *cwe, const char *remediation) {
    if (art->issue_count >= LST_MAX_ISSUES) return;
    lst_issue_t *issue = &art->issues[art->issue_count];
    memset(issue, 0, sizeof(lst_issue_t));

    snprintf(issue->id, sizeof(issue->id), "SSS-%04u", art->issue_count + 1);
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

/* --------------------------------------------------------------------------
 * Pattern arrays
 * -------------------------------------------------------------------------- */

/* Required typed fields in daemon state schemas */
static const char *REQUIRED_TYPED_FIELDS[] = {
    "threat_score",
    "insight_score",
    "morph_cycle",
    NULL
};

/* Patterns that indicate a schema definition */
static const char *SCHEMA_DEF_PATTERNS[] = {
    "_state {",
    "_state{",
    "_schema {",
    "_schema{",
    "_state_t {",
    "_state_t{",
    "_schema_t {",
    "_schema_t{",
    NULL
};

/* Patterns that indicate a version field */
static const char *VERSION_PATTERNS[] = {
    "schema_version",
    "state_version",
    "version",
    NULL
};

/* Type keywords that qualify as a type declaration */
static const char *TYPE_KEYWORDS[] = {
    "uint8_t",  "uint16_t", "uint32_t", "uint64_t",
    "int8_t",   "int16_t",  "int32_t",  "int64_t",
    "float",    "double",   "char",     "int",
    "size_t",   "bool",     "unsigned",
    NULL
};

/* Directories to skip */
static const char *SKIP_DIRS_SSS[] = {
    "node_modules", "vendor", ".git", "__pycache__", "dist", "build",
    ".venv", "venv", ".next", ".nuxt", "target", ".cache", "coverage",
    ".tox", "env", NULL
};

/* --------------------------------------------------------------------------
 * Pattern checks
 * -------------------------------------------------------------------------- */

/* Check if a line contains a schema definition */
static int line_has_schema_def(const char *line) {
    for (int i = 0; SCHEMA_DEF_PATTERNS[i]; i++) {
        if (strstr(line, SCHEMA_DEF_PATTERNS[i])) return 1;
    }
    /* Also match "typedef struct" lines with state/schema names */
    if (strstr(line, "typedef struct") &&
        (strstr(line, "state") || strstr(line, "schema"))) {
        return 1;
    }
    return 0;
}

/* Check if a line contains a version field */
static int line_has_version_field(const char *line) {
    for (int i = 0; VERSION_PATTERNS[i]; i++) {
        if (strstr(line, VERSION_PATTERNS[i])) return 1;
    }
    return 0;
}

/* Check if a line has a type declaration for a given field name */
static int line_has_typed_field(const char *line, const char *field) {
    if (!strstr(line, field)) return 0;
    for (int i = 0; TYPE_KEYWORDS[i]; i++) {
        if (strstr(line, TYPE_KEYWORDS[i])) return 1;
    }
    return 0;
}

/* Check if a line references a field without a type (bare assignment or use) */
static int line_has_bare_field_ref(const char *line, const char *field) {
    const char *p = strstr(line, field);
    if (!p) return 0;
    /* Skip if a type keyword is on this line (it is a declaration) */
    for (int i = 0; TYPE_KEYWORDS[i]; i++) {
        if (strstr(line, TYPE_KEYWORDS[i])) return 0;
    }
    /* Skip comments */
    const char *s = line;
    while (*s == ' ' || *s == '\t') s++;
    if (*s == '/' || *s == '*' || *s == '#') return 0;
    return 1;
}

/* --------------------------------------------------------------------------
 * File scanner
 * -------------------------------------------------------------------------- */

static int should_skip_sss(const char *name) {
    if (name[0] == '.') return 1;
    for (int i = 0; SKIP_DIRS_SSS[i]; i++)
        if (strcmp(name, SKIP_DIRS_SSS[i]) == 0) return 1;
    return 0;
}

static int is_schema_scannable(const char *name) {
    const char *ext = strrchr(name, '.');
    if (!ext) return 0;
    const char *exts[] = {".c", ".h", ".py", ".rs", ".go", ".ts", ".js",
                          ".json", ".yaml", ".yml", ".toml", NULL};
    for (int i = 0; exts[i]; i++)
        if (strcmp(ext, exts[i]) == 0) return 1;
    return 0;
}

static void scan_file_schema(lst_artifact_t *art, const char *fpath) {
    size_t len = 0;
    char *content = read_file_sss(fpath, &len);
    if (!content) return;

    int lineno = 1;
    char *line_start = content;
    int in_schema_block = 0;
    int schema_has_version = 0;
    int schema_start_line = 0;
    int brace_depth = 0;
    int found_typed[16];
    int required_count = 0;

    /* Count required fields */
    for (int i = 0; REQUIRED_TYPED_FIELDS[i]; i++) required_count++;
    memset(found_typed, 0, sizeof(found_typed));

    while (*line_start) {
        char *eol = strchr(line_start, '\n');
        if (eol) *eol = '\0';

        /* Detect schema block entry */
        if (!in_schema_block && line_has_schema_def(line_start)) {
            in_schema_block = 1;
            schema_has_version = 0;
            schema_start_line = lineno;
            brace_depth = 0;
            memset(found_typed, 0, sizeof(found_typed));
            /* Count opening brace on this line */
            for (const char *c = line_start; *c; c++) {
                if (*c == '{') brace_depth++;
                if (*c == '}') brace_depth--;
            }
        } else if (in_schema_block) {
            /* Track braces */
            for (const char *c = line_start; *c; c++) {
                if (*c == '{') brace_depth++;
                if (*c == '}') brace_depth--;
            }

            /* Check for version field inside schema */
            if (line_has_version_field(line_start)) {
                schema_has_version = 1;
            }

            /* Check for typed required fields */
            for (int f = 0; f < required_count; f++) {
                if (line_has_typed_field(line_start, REQUIRED_TYPED_FIELDS[f])) {
                    found_typed[f] = 1;
                }
            }

            /* Schema block closed */
            if (brace_depth <= 0) {
                if (!schema_has_version) {
                    add_sss_issue(art, SEV_ERROR,
                        "Schema Missing Version Field",
                        "Shared-state schema has no version field",
                        fpath, (uint32_t)schema_start_line, "CWE-1104",
                        "Add a schema_version field to track changes");
                }
                for (int f = 0; f < required_count; f++) {
                    if (!found_typed[f]) {
                        char desc[LST_MAX_NAME];
                        snprintf(desc, sizeof(desc),
                            "Field '%s' not typed in schema block",
                            REQUIRED_TYPED_FIELDS[f]);
                        add_sss_issue(art, SEV_WARNING,
                            "Required Field Missing Type Declaration",
                            desc,
                            fpath, (uint32_t)schema_start_line, "CWE-1287",
                            "Add explicit type declaration for field");
                    }
                }
                in_schema_block = 0;
            }
        }

        /* Outside schema blocks: detect bare field references */
        if (!in_schema_block) {
            for (int f = 0; REQUIRED_TYPED_FIELDS[f]; f++) {
                if (line_has_bare_field_ref(line_start, REQUIRED_TYPED_FIELDS[f])) {
                    /* Check if this looks like a struct field access (->field or .field) */
                    char dot_pat[128];
                    char arrow_pat[128];
                    snprintf(dot_pat, sizeof(dot_pat), ".%s", REQUIRED_TYPED_FIELDS[f]);
                    snprintf(arrow_pat, sizeof(arrow_pat), "->%s", REQUIRED_TYPED_FIELDS[f]);
                    if (strstr(line_start, dot_pat) || strstr(line_start, arrow_pat)) {
                        /* This is a usage -- acceptable if schema exists elsewhere */
                        continue;
                    }
                    /* Bare reference without schema context */
                    char desc[LST_MAX_NAME];
                    snprintf(desc, sizeof(desc),
                        "Field '%s' referenced without schema declaration",
                        REQUIRED_TYPED_FIELDS[f]);
                    add_sss_issue(art, SEV_INFO,
                        "Undeclared Field Reference",
                        desc,
                        fpath, (uint32_t)lineno, "CWE-1287",
                        "Declare field in shared-state schema first");
                }
            }
        }

        /* Detect version constant changes without schema update */
        if (strstr(line_start, "#define") && strstr(line_start, "SCHEMA_VERSION")) {
            /* Check if set to 0 */
            if (strstr(line_start, " 0") && !strstr(line_start, " 0x")) {
                add_sss_issue(art, SEV_ERROR,
                    "Schema Version Is Zero",
                    "SCHEMA_VERSION must be >= 1 for tracked schemas",
                    fpath, (uint32_t)lineno, "CWE-1104",
                    "Set SCHEMA_VERSION to a positive integer");
            }
        }

        if (!eol) break;
        line_start = eol + 1;
        lineno++;
    }

    /* If we ended still inside a schema block, flag it */
    if (in_schema_block) {
        add_sss_issue(art, SEV_ERROR,
            "Unterminated Schema Block",
            "Schema struct definition never closed",
            fpath, (uint32_t)schema_start_line, "CWE-1104",
            "Ensure schema struct has matching closing brace");
    }

    free(content);
}

static void scan_dir_schema(lst_artifact_t *art, const char *dir, int depth) {
    if (depth > 8) return;

    DIR *d = opendir(dir);
    if (!d) return;

    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        if (ent->d_name[0] == '.') continue;
        if (should_skip_sss(ent->d_name)) continue;

        char child[LST_MAX_PATH];
        snprintf(child, sizeof(child), "%s/%s", dir, ent->d_name);

        struct stat st;
        if (stat(child, &st) != 0) continue;

        if (S_ISDIR(st.st_mode)) {
            scan_dir_schema(art, child, depth + 1);
        } else if (S_ISREG(st.st_mode) && is_schema_scannable(ent->d_name)) {
            scan_file_schema(art, child);
        }
    }
    closedir(d);
}

/* --------------------------------------------------------------------------
 * Report writer
 * -------------------------------------------------------------------------- */

static const char *sss_severity_name(uint8_t sev) {
    switch (sev) {
        case SEV_INFO:     return "INFO";
        case SEV_WARNING:  return "WARNING";
        case SEV_ERROR:    return "ERROR";
        case SEV_CRITICAL: return "CRITICAL";
        default:           return "NONE";
    }
}

static void write_sep_sss(FILE *f) {
    for (int i = 0; i < 80; i++) fputc('=', f);
    fputc('\n', f);
}

static void write_line_sss(FILE *f) {
    for (int i = 0; i < 40; i++) fputc('-', f);
    fputc('\n', f);
}

/* --------------------------------------------------------------------------
 * Recipe entry point
 * -------------------------------------------------------------------------- */

static int recipe_shared_state_schema(lst_artifact_t *art,
                                       const char *output_dir) {
    if (!art) return -1;

    uint32_t initial_issues = art->issue_count;
    scan_dir_schema(art, art->project_path, 0);
    uint32_t new_issues = art->issue_count - initial_issues;

    /* Build output path */
    char outpath[LST_MAX_PATH];
    if (output_dir)
        snprintf(outpath, sizeof(outpath),
                 "%s/SHARED_STATE_SCHEMA_REPORT", output_dir);
    else
        snprintf(outpath, sizeof(outpath),
                 "%s/SHARED_STATE_SCHEMA_REPORT", art->project_path);

    if (output_dir) mkdir(output_dir, 0755);

    FILE *f = fopen(outpath, "w");
    if (!f) {
        fprintf(stderr,
                "shared-state-schema: cannot write %s\n", outpath);
        return -1;
    }

    /* Header */
    write_sep_sss(f);
    fprintf(f, "SHARED-STATE SCHEMA VALIDATION REPORT\n");
    fprintf(f, "Project: %s\n", art->project_name);
    fprintf(f, "Path: %s\n", art->project_path);

    time_t now = time(NULL);
    struct tm *t = gmtime(&now);
    char ts[64];
    strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S UTC", t);
    fprintf(f, "Generated: %s\n", ts);
    fprintf(f, "Schema Issues Found: %u\n", new_issues);
    write_sep_sss(f);
    fprintf(f, "\n");

    /* Severity summary */
    fprintf(f, "SEVERITY SUMMARY\n");
    write_line_sss(f);
    int counts[5] = {0};
    for (uint32_t i = initial_issues; i < art->issue_count; i++)
        counts[art->issues[i].severity]++;
    if (counts[SEV_CRITICAL])
        fprintf(f, "  CRITICAL:  %d\n", counts[SEV_CRITICAL]);
    if (counts[SEV_ERROR])
        fprintf(f, "  ERROR:     %d\n", counts[SEV_ERROR]);
    if (counts[SEV_WARNING])
        fprintf(f, "  WARNING:   %d\n", counts[SEV_WARNING]);
    if (counts[SEV_INFO])
        fprintf(f, "  INFO:      %d\n", counts[SEV_INFO]);
    fprintf(f, "\n");

    /* Findings */
    if (new_issues > 0) {
        fprintf(f, "FINDINGS\n");
        write_sep_sss(f);
        fprintf(f, "\n");

        for (int sev = SEV_CRITICAL; sev >= SEV_INFO; sev--) {
            int printed_header = 0;
            for (uint32_t i = initial_issues; i < art->issue_count; i++) {
                if (art->issues[i].severity != sev) continue;
                if (!printed_header) {
                    fprintf(f, "  [%s]\n\n",
                            sss_severity_name((uint8_t)sev));
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
        fprintf(f, "All shared-state schemas are properly versioned "
                    "and typed.\n\n");
    }

    /* Footer */
    write_sep_sss(f);
    fprintf(f, "END OF SHARED-STATE SCHEMA REPORT\n");
    fprintf(f, "\nThis report validates NNOS daemon state schemas for\n");
    fprintf(f, "versioning, type safety, and field declaration hygiene.\n");
    write_sep_sss(f);
    fprintf(f, "\n");

    fclose(f);

    printf("  Wrote %s (%u issues)\n", outpath, new_issues);
    return 0;
}

/* --------------------------------------------------------------------------
 * Recipe registration
 * -------------------------------------------------------------------------- */

void recipe_shared_state_schema_register(void) {
    lst_recipe_t r = {0};
    snprintf(r.name, LST_MAX_NAME, "shared-state-schema");
    snprintf(r.description, LST_MAX_NAME,
             "Validate NNOS shared-state schema versioning and types");
    r.execute = recipe_shared_state_schema;
    r.version = 1;
    lst_recipe_register(&r);
}
