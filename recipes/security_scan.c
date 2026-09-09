#define _POSIX_C_SOURCE 200809L

/*
 * security_scan.c — Security Vulnerability Scanner Recipe
 *
 * Deterministic pattern matching against source files in an LST artifact.
 * Converts the logic from frameworks/security/scanner.py and
 * frameworks/code_check/__init__.py into compiled C.
 *
 * Checks:
 *   - SQL injection (CWE-89)
 *   - Command injection (CWE-78)
 *   - Hardcoded credentials (CWE-798)
 *   - Weak cryptography (CWE-327)
 *   - Bare except / eval/exec (CWE-703 / CWE-95)
 *   - Neural safety violations (BCI-specific)
 *   - Complexity analysis
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

static char *read_file_sec(const char *path, size_t *out_len) {
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

/* Case-insensitive substring search */
static const char *strcasestr_local(const char *haystack, const char *needle) {
    if (!*needle) return haystack;
    size_t nlen = strlen(needle);
    for (; *haystack; haystack++) {
        if (strncasecmp(haystack, needle, nlen) == 0)
            return haystack;
    }
    return NULL;
}

/* Add an issue to the artifact */
static void add_issue(lst_artifact_t *art, uint8_t severity,
                      const char *title, const char *desc,
                      const char *file_path, uint32_t line,
                      const char *cwe, const char *remediation) {
    if (art->issue_count >= LST_MAX_ISSUES) return;
    lst_issue_t *issue = &art->issues[art->issue_count];
    memset(issue, 0, sizeof(lst_issue_t));

    snprintf(issue->id, sizeof(issue->id), "SEC-%04u", art->issue_count + 1);
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
 * Pattern checks — line-by-line scanning
 * -------------------------------------------------------------------------- */

/* Check for SQL injection: execute() with %s or format() */
static void check_sql_injection(lst_artifact_t *art, const char *fpath,
                                 const char *line, int lineno) {
    if (!strstr(line, "execute(")) return;
    if (strstr(line, "%s") || strstr(line, "format(") || strstr(line, "f\"")) {
        add_issue(art, SEV_ERROR,
            "Potential SQL Injection",
            "String formatting used with SQL execute()",
            fpath, (uint32_t)lineno, "CWE-89",
            "Use parameterized queries instead");
    }
}

/* Check for command injection: os.system(), subprocess.call() with strings */
static void check_command_injection(lst_artifact_t *art, const char *fpath,
                                     const char *line, int lineno) {
    if (strstr(line, "os.system(") || strstr(line, "subprocess.call(") ||
        strstr(line, "subprocess.run(") || strstr(line, "subprocess.Popen(")) {
        if (strchr(line, '"') || strchr(line, '\'')) {
            add_issue(art, SEV_CRITICAL,
                "Potential Command Injection",
                "Unsanitized input to system command",
                fpath, (uint32_t)lineno, "CWE-78",
                "Sanitize input; use subprocess with array arguments");
        }
    }
    /* C-level: system() calls with string concat */
    if (strstr(line, "system(") && strstr(line, "snprintf")) {
        add_issue(art, SEV_ERROR,
            "Potential Command Injection (C)",
            "system() call with formatted string",
            fpath, (uint32_t)lineno, "CWE-78",
            "Avoid system(); use execve() with explicit arguments");
    }
}

/* Check for hardcoded credentials */
static void check_hardcoded_creds(lst_artifact_t *art, const char *fpath,
                                   const char *line, int lineno) {
    const char *keywords[] = {"password", "api_key", "api-key", "apikey",
                               "secret", "token", "private_key", NULL};
    /* Must have assignment and a string literal */
    if (!strchr(line, '=')) return;
    if (!strchr(line, '"') && !strchr(line, '\'')) return;

    /* Skip comments */
    const char *stripped = line;
    while (*stripped == ' ' || *stripped == '\t') stripped++;
    if (*stripped == '#' || *stripped == '/' || *stripped == '*') return;

    for (int k = 0; keywords[k]; k++) {
        if (strcasestr_local(line, keywords[k])) {
            /* Exclude env lookups and config reads */
            if (strstr(line, "os.environ") || strstr(line, "getenv") ||
                strstr(line, "os.getenv") || strstr(line, "config.get") ||
                strstr(line, "env(") || strstr(line, "ENV[")) return;
            add_issue(art, SEV_CRITICAL,
                "Hardcoded Credentials",
                "Credentials should not be hardcoded in source",
                fpath, (uint32_t)lineno, "CWE-798",
                "Use environment variables or a secure vault");
            return; /* one per line */
        }
    }
}

/* Check for weak cryptography: MD5, SHA1 */
static void check_weak_crypto(lst_artifact_t *art, const char *fpath,
                               const char *line, int lineno) {
    /* Skip comments */
    const char *stripped = line;
    while (*stripped == ' ' || *stripped == '\t') stripped++;
    if (*stripped == '#' || *stripped == '/' || *stripped == '*') return;

    if (strcasestr_local(line, "md5") && !strcasestr_local(line, "sha256") &&
        !strcasestr_local(line, "sha-256")) {
        add_issue(art, SEV_WARNING,
            "Weak Cryptographic Algorithm",
            "MD5 is cryptographically broken",
            fpath, (uint32_t)lineno, "CWE-327",
            "Use SHA-256 or SHA-3 for hashing");
    }
    if (strcasestr_local(line, "sha1") && !strcasestr_local(line, "sha1sum") &&
        !strcasestr_local(line, "sha256")) {
        add_issue(art, SEV_WARNING,
            "Weak Cryptographic Algorithm",
            "SHA-1 is cryptographically weak",
            fpath, (uint32_t)lineno, "CWE-327",
            "Use SHA-256 or SHA-3 for hashing");
    }
}

/* Check for bare except (Python) */
static void check_bare_except(lst_artifact_t *art, const char *fpath,
                               const char *line, int lineno) {
    /* Match "except:" but not "except SomeError:" */
    const char *p = strstr(line, "except");
    if (!p) return;
    p += 6;
    while (*p == ' ' || *p == '\t') p++;
    if (*p == ':') {
        add_issue(art, SEV_WARNING,
            "Bare Except Clause",
            "Bare except catches all exceptions including SystemExit",
            fpath, (uint32_t)lineno, "CWE-703",
            "Specify the exception type to catch");
    }
}

/* Check for eval/exec usage */
static void check_eval_exec(lst_artifact_t *art, const char *fpath,
                              const char *line, int lineno) {
    /* Skip comments */
    const char *stripped = line;
    while (*stripped == ' ' || *stripped == '\t') stripped++;
    if (*stripped == '#' || *stripped == '/' || *stripped == '*') return;

    if (strstr(line, "eval(") || strstr(line, "exec(")) {
        /* Not inside a comment or docstring */
        add_issue(art, SEV_ERROR,
            "Dangerous eval/exec Usage",
            "eval() and exec() can execute arbitrary code",
            fpath, (uint32_t)lineno, "CWE-95",
            "Avoid eval/exec; use safe alternatives");
    }
}

/* Check for neural safety (BCI-specific) */
static void check_neural_safety(lst_artifact_t *art, const char *fpath,
                                 const char *line, int lineno) {
    if (strstr(line, "stimulate") || strstr(line, "charge_density")) {
        if (!strcasestr_local(line, "safety") && !strcasestr_local(line, "check") &&
            !strcasestr_local(line, "limit") && !strcasestr_local(line, "bound")) {
            add_issue(art, SEV_CRITICAL,
                "Missing Neural Safety Check",
                "Neural stimulation without safety validation",
                fpath, (uint32_t)lineno, NULL,
                "Add safety bounds check before stimulation");
        }
    }
}

/* Simple complexity: count decision points per line */
static void check_complexity(lst_artifact_t *art, const char *fpath,
                              const char *line, int lineno) {
    int points = 0;
    const char *keywords[] = {" if ", " elif ", " for ", " while ", " and ", " or ",
                               "&&", "||", "? ", NULL};
    for (int k = 0; keywords[k]; k++) {
        const char *p = line;
        while ((p = strstr(p, keywords[k])) != NULL) {
            points++;
            p += strlen(keywords[k]);
        }
    }
    if (points > 5) {
        char desc[LST_MAX_NAME];
        snprintf(desc, sizeof(desc), "Line has %d decision points — consider splitting", points);
        add_issue(art, SEV_WARNING,
            "High Complexity",
            desc,
            fpath, (uint32_t)lineno, NULL,
            "Refactor into smaller functions");
    }
}

/* --------------------------------------------------------------------------
 * File scanner
 * -------------------------------------------------------------------------- */

/* Skip list for directories */
static const char *SKIP_DIRS_SEC[] = {
    "node_modules", "vendor", ".git", "__pycache__", "dist", "build",
    ".venv", "venv", ".next", ".nuxt", "target", ".cache", "coverage",
    ".tox", "env", NULL
};

static int should_skip_sec(const char *name) {
    if (name[0] == '.') return 1;
    for (int i = 0; SKIP_DIRS_SEC[i]; i++)
        if (strcmp(name, SKIP_DIRS_SEC[i]) == 0) return 1;
    return 0;
}

/* Scannable extensions */
static int is_scannable(const char *name) {
    const char *ext = strrchr(name, '.');
    if (!ext) return 0;
    const char *exts[] = {".py", ".js", ".ts", ".jsx", ".tsx", ".c", ".cpp", ".h",
                          ".php", ".rb", ".go", ".rs", ".java", ".kt", ".swift",
                          ".sh", ".bash", ".sql", NULL};
    for (int i = 0; exts[i]; i++)
        if (strcasecmp(ext, exts[i]) == 0) return 1;
    return 0;
}

static void scan_file_security(lst_artifact_t *art, const char *fpath) {
    size_t len = 0;
    char *content = read_file_sec(fpath, &len);
    if (!content) return;

    int lineno = 1;
    char *line_start = content;

    while (*line_start) {
        /* Find end of line */
        char *eol = strchr(line_start, '\n');
        if (eol) *eol = '\0';

        /* Run all checks */
        check_sql_injection(art, fpath, line_start, lineno);
        check_command_injection(art, fpath, line_start, lineno);
        check_hardcoded_creds(art, fpath, line_start, lineno);
        check_weak_crypto(art, fpath, line_start, lineno);
        check_bare_except(art, fpath, line_start, lineno);
        check_eval_exec(art, fpath, line_start, lineno);
        check_neural_safety(art, fpath, line_start, lineno);
        check_complexity(art, fpath, line_start, lineno);

        if (!eol) break;
        line_start = eol + 1;
        lineno++;
    }

    free(content);
}

static void scan_dir_security(lst_artifact_t *art, const char *dir, int depth) {
    if (depth > 8) return;

    DIR *d = opendir(dir);
    if (!d) return;

    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        if (ent->d_name[0] == '.') continue;
        if (should_skip_sec(ent->d_name)) continue;

        char child[LST_MAX_PATH];
        snprintf(child, sizeof(child), "%s/%s", dir, ent->d_name);

        struct stat st;
        if (stat(child, &st) != 0) continue;

        if (S_ISDIR(st.st_mode)) {
            scan_dir_security(art, child, depth + 1);
        } else if (S_ISREG(st.st_mode) && is_scannable(ent->d_name)) {
            scan_file_security(art, child);
        }
    }
    closedir(d);
}

/* --------------------------------------------------------------------------
 * Report writer
 * -------------------------------------------------------------------------- */

static const char *severity_name(uint8_t sev) {
    switch (sev) {
        case SEV_INFO:     return "INFO";
        case SEV_WARNING:  return "WARNING";
        case SEV_ERROR:    return "ERROR";
        case SEV_CRITICAL: return "CRITICAL";
        default:           return "NONE";
    }
}

static void write_sep_sec(FILE *f) {
    for (int i = 0; i < 80; i++) fputc('=', f);
    fputc('\n', f);
}

static void write_line_sec(FILE *f) {
    for (int i = 0; i < 40; i++) fputc('-', f);
    fputc('\n', f);
}

/* Compute risk score: 0-100 */
static float compute_risk_score(const lst_artifact_t *art) {
    if (art->issue_count == 0) return 0.0f;

    float total = 0;
    for (uint32_t i = 0; i < art->issue_count; i++) {
        switch (art->issues[i].severity) {
            case SEV_INFO:     total += 5;   break;
            case SEV_WARNING:  total += 15;  break;
            case SEV_ERROR:    total += 35;  break;
            case SEV_CRITICAL: total += 70;  break;
            default: break;
        }
    }
    float score = total / (float)art->issue_count;
    return score > 100.0f ? 100.0f : score;
}

/* --------------------------------------------------------------------------
 * Recipe entry point
 * -------------------------------------------------------------------------- */

static int recipe_security_scan(lst_artifact_t *art, const char *output_dir) {
    if (!art) return -1;

    /* Scan project files for vulnerabilities */
    uint32_t initial_issues = art->issue_count;
    scan_dir_security(art, art->project_path, 0);
    uint32_t new_issues = art->issue_count - initial_issues;

    /* Compute risk score */
    float risk = compute_risk_score(art);
    art->risk_score = risk;

    /* Set security level */
    if (risk >= 70.0f) art->security_level = SEC_CRITICAL;
    else if (risk >= 40.0f) art->security_level = SEC_PRIVATE;
    else if (risk >= 10.0f) art->security_level = SEC_ISOLATED;
    else art->security_level = SEC_SECURE;

    /* Build output path */
    char outpath[LST_MAX_PATH];
    if (output_dir)
        snprintf(outpath, sizeof(outpath), "%s/SECURITY_REPORT", output_dir);
    else
        snprintf(outpath, sizeof(outpath), "%s/SECURITY_REPORT", art->project_path);

    if (output_dir) mkdir(output_dir, 0755);

    int rfd = open(outpath, O_WRONLY | O_CREAT | O_TRUNC | O_NOFOLLOW | O_CLOEXEC, 0644);
    if (rfd < 0) {
        fprintf(stderr, "security-scan: cannot write %s\n", outpath);
        return -1;
    }
    FILE *f = fdopen(rfd, "w");
    if (!f) {
        close(rfd);
        fprintf(stderr, "security-scan: cannot write %s\n", outpath);
        return -1;
    }

    /* Header */
    write_sep_sec(f);
    fprintf(f, "SECURITY VULNERABILITY REPORT\n");
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
    fprintf(f, "Risk Score: %.1f / 100\n", risk);
    fprintf(f, "Security Level: %s\n",
        art->security_level == SEC_SECURE   ? "SECURE" :
        art->security_level == SEC_ISOLATED ? "ELEVATED" :
        art->security_level == SEC_PRIVATE  ? "HIGH" : "CRITICAL");
    fprintf(f, "Vulnerabilities Found: %u\n", new_issues);
    write_sep_sec(f);
    fprintf(f, "\n");

    /* Severity summary */
    fprintf(f, "SEVERITY SUMMARY\n");
    write_line_sec(f);
    int counts[5] = {0};
    for (uint32_t i = initial_issues; i < art->issue_count; i++)
        counts[art->issues[i].severity]++;
    if (counts[SEV_CRITICAL]) fprintf(f, "  CRITICAL:  %d\n", counts[SEV_CRITICAL]);
    if (counts[SEV_ERROR])    fprintf(f, "  ERROR:     %d\n", counts[SEV_ERROR]);
    if (counts[SEV_WARNING])  fprintf(f, "  WARNING:   %d\n", counts[SEV_WARNING]);
    if (counts[SEV_INFO])     fprintf(f, "  INFO:      %d\n", counts[SEV_INFO]);
    fprintf(f, "\n");

    /* Group by CWE */
    if (new_issues > 0) {
        fprintf(f, "FINDINGS\n");
        write_sep_sec(f);
        fprintf(f, "\n");

        /* Critical first, then error, warning, info */
        for (int sev = SEV_CRITICAL; sev >= SEV_INFO; sev--) {
            int printed_header = 0;
            for (uint32_t i = initial_issues; i < art->issue_count; i++) {
                if (art->issues[i].severity != sev) continue;
                if (!printed_header) {
                    fprintf(f, "  [%s]\n\n", severity_name((uint8_t)sev));
                    printed_header = 1;
                }
                const lst_issue_t *issue = &art->issues[i];
                fprintf(f, "    %s: %s\n", issue->id, issue->title);
                if (issue->file_path[0])
                    fprintf(f, "      File: %s:%u\n", issue->file_path, issue->line_number);
                if (issue->cwe[0])
                    fprintf(f, "      CWE: %s\n", issue->cwe);
                fprintf(f, "      %s\n", issue->description);
                if (issue->remediation[0])
                    fprintf(f, "      Fix: %s\n", issue->remediation);
                fprintf(f, "\n");
            }
        }
    } else {
        fprintf(f, "No vulnerabilities found.\n\n");
    }

    /* Footer */
    write_sep_sec(f);
    fprintf(f, "END OF SECURITY REPORT\n");
    fprintf(f, "\nThis report is generated by deterministic pattern matching.\n");
    fprintf(f, "Manual review is still recommended for complex security issues.\n");
    write_sep_sec(f);
    fprintf(f, "\n");

    fclose(f);

    printf("  Wrote %s (%u issues, risk=%.1f)\n", outpath, new_issues, risk);
    return 0;
}

/* --------------------------------------------------------------------------
 * Recipe registration
 * -------------------------------------------------------------------------- */

void recipe_security_scan_register(void) {
    lst_recipe_t r = {0};
    snprintf(r.name, LST_MAX_NAME, "security-scan");
    snprintf(r.description, LST_MAX_NAME, "Scan source files for security vulnerabilities");
    r.execute = recipe_security_scan;
    r.version = 1;
    lst_recipe_register(&r);
}
