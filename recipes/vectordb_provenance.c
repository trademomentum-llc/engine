/*
 * vectordb_provenance.c -- VectorDB Provenance Integration Recipe
 *
 * Validates project artifacts against the Qdrant provenance corpus:
 *   - Checks if project concepts have provenance in the corpus
 *   - Validates traceability of key terms to source documents
 *   - Detects orphaned concepts with no provenance chain
 *   - Produces provenance coverage report
 *   - Maps project files to iCloud/corpus source primitives
 *
 * Uses the provenance_corpus collection (384-dim, Cosine)
 * via Qdrant REST API at localhost:6333.
 *
 * This recipe does NOT embed at runtime -- it checks whether
 * the project's key identifiers and concepts appear in the
 * corpus payloads (source_file, content, context_tags).
 */

#include "lst.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <limits.h>

/* --------------------------------------------------------------------------
 * Qdrant REST API interaction (via external curl call)
 *
 * We shell out to curl rather than linking libcurl to keep
 * the engine zero-dependency.  The recipe only runs on the
 * build host where curl is always available.
 * -------------------------------------------------------------------------- */

#define QDRANT_URL "http://127.0.0.1:6333"
#define COLLECTION "provenance_corpus"
#define MAX_QUERY_LEN 512

static int run_curl(char *const argv[], char *output, size_t output_size) {
    int pipefd[2];
    if (pipe(pipefd) != 0) return -1;
    pid_t pid = fork();
    if (pid < 0) { close(pipefd[0]); close(pipefd[1]); return -1; }
    if (pid == 0) {
        int devnull = open("/dev/null", O_WRONLY);
        dup2(pipefd[1], STDOUT_FILENO);
        if (devnull >= 0) dup2(devnull, STDERR_FILENO);
        close(pipefd[0]); close(pipefd[1]);
        if (devnull >= 0) close(devnull);
        execv("/usr/bin/curl", argv);
        _exit(127);
    }
    close(pipefd[1]);
    size_t total = 0;
    while (total + 1 < output_size) {
        ssize_t n = read(pipefd[0], output + total, output_size - total - 1);
        if (n <= 0) break;
        total += (size_t)n;
    }
    output[total] = '\0';
    close(pipefd[0]);
    int status;
    if (waitpid(pid, &status, 0) < 0 || !WIFEXITED(status) ||
        WEXITSTATUS(status) != 0) return -1;
    return 0;
}

static void json_escape(char *dst, size_t size, const char *src) {
    size_t used = 0;
    for (; *src && used + 2 < size; src++) {
        if (*src == '"' || *src == '\\') dst[used++] = '\\';
        dst[used++] = *src;
    }
    dst[used] = '\0';
}

static int qdrant_scroll_count(const char *filter_field, const char *filter_value) {
    /*
     * Query Qdrant for points matching a payload filter.
     * Returns the count of matching points, or -1 on error.
     */
    char escaped_field[MAX_QUERY_LEN * 2], escaped_value[MAX_QUERY_LEN * 2];
    char data[MAX_QUERY_LEN * 4];
    json_escape(escaped_field, sizeof(escaped_field), filter_field);
    json_escape(escaped_value, sizeof(escaped_value), filter_value);
    snprintf(data, sizeof(data),
        "{\"filter\":{\"must\":[{\"key\":\"%s\",\"match\":{\"text\":\"%s\"}}]},"
        "\"limit\":1,\"with_payload\":false}", escaped_field, escaped_value);
    char buf[4096];
    char url[256];
    snprintf(url, sizeof(url), "%s/collections/%s/points/scroll", QDRANT_URL, COLLECTION);
    char *argv[] = {"curl", "-s", "-X", "POST", url, "-H",
                    "Content-Type: application/json", "-d", data, NULL};
    if (run_curl(argv, buf, sizeof(buf)) != 0) return -1;

    /* Parse minimal JSON to find "points":[ ... ] array length */
    char *points = strstr(buf, "\"points\":[");
    if (!points) return 0;

    /* Count objects in the array (each starts with '{') */
    char *p = points + 10;  /* skip "points":[ */
    if (*p == ']') return 0;

    /* We asked limit=1, so if there's anything it's at least 1 match */
    return 1;
}

static int qdrant_count_collection(void) {
    char buf[4096];
    char url[256];
    snprintf(url, sizeof(url), "%s/collections/%s", QDRANT_URL, COLLECTION);
    char *argv[] = {"curl", "-s", url, NULL};
    if (run_curl(argv, buf, sizeof(buf)) != 0) return -1;

    /* Find points_count in response */
    char *pc = strstr(buf, "\"points_count\":");
    if (!pc) return -1;
    char *end;
    long count = strtol(pc + 15, &end, 10);
    if (end == pc + 15 || count < 0 || count > INT_MAX) return -1;
    return (int)count;
}

/* --------------------------------------------------------------------------
 * Key concept extraction from project files
 * -------------------------------------------------------------------------- */

/* Extract identifiers that might be provenance-bearing */
static int extract_key_terms(
    lst_artifact_t *art,
    char terms[][MAX_QUERY_LEN],
    int max_terms
) {
    int count = 0;

    /* Use project name itself */
    if (art->project_name[0] && count < max_terms) {
        snprintf(terms[count], MAX_QUERY_LEN, "%s", art->project_name);
        count++;
    }

    /* Scan file names for meaningful identifiers */
    for (uint32_t i = 0; i < art->file_count && count < max_terms; i++) {
        const char *path = art->files[i].path;
        const char *base = strrchr(path, '/');
        base = base ? base + 1 : path;

        /* Skip generic names */
        if (strcmp(base, "index.js") == 0) continue;
        if (strcmp(base, "index.ts") == 0) continue;
        if (strcmp(base, "__init__.py") == 0) continue;
        if (strcmp(base, "main.py") == 0) continue;
        if (strcmp(base, "main.c") == 0) continue;
        if (strcmp(base, "README.md") == 0) continue;
        if (strcmp(base, "package.json") == 0) continue;

        /* Use meaningful filenames as search terms */
        char clean[MAX_QUERY_LEN];
        int ci = 0;
        for (int j = 0; base[j] && base[j] != '.' && ci < MAX_QUERY_LEN - 1; j++) {
            if (base[j] == '_' || base[j] == '-')
                clean[ci++] = ' ';
            else
                clean[ci++] = base[j];
        }
        clean[ci] = '\0';

        if (ci > 4) {  /* skip very short names */
            snprintf(terms[count], MAX_QUERY_LEN, "%s", clean);
            count++;
        }
    }

    /* Also check dependency names */
    for (uint32_t i = 0; i < art->dep_count && count < max_terms; i++) {
        if (art->deps[i].name[0] && strlen(art->deps[i].name) > 3) {
            snprintf(terms[count], MAX_QUERY_LEN, "%s", art->deps[i].name);
            count++;
        }
    }

    return count;
}

/* --------------------------------------------------------------------------
 * Recipe entry point
 * -------------------------------------------------------------------------- */

static int recipe_vectordb_provenance(lst_artifact_t *art, const char *output_dir) {
    (void)output_dir;

    printf("\n");
    for (int i = 0; i < 70; i++) putchar('=');
    printf("\nVECTORDB PROVENANCE INTEGRATION REPORT\n");
    printf("Project: %s\n", art->project_name);
    printf("Corpus: %s @ %s\n", COLLECTION, QDRANT_URL);
    for (int i = 0; i < 70; i++) putchar('=');
    printf("\n\n");

    int critical = 0, error = 0, warning = 0, info = 0;

    /* Check corpus availability */
    printf("[1/4] Corpus Status\n");
    int corpus_size = qdrant_count_collection();
    if (corpus_size < 0) {
        printf("  [CRITICAL] Cannot connect to Qdrant at %s\n", QDRANT_URL);
        printf("    Ensure Qdrant is running with provenance_corpus collection\n");
        critical++;

        if (art->issue_count < LST_MAX_ISSUES) {
            lst_issue_t *iss = &art->issues[art->issue_count++];
            snprintf(iss->id, sizeof(iss->id), "VDB-001");
            iss->severity = SEV_CRITICAL;
            snprintf(iss->title, sizeof(iss->title),
                     "VectorDB corpus not reachable");
            snprintf(iss->description, sizeof(iss->description),
                     "Qdrant at %s is not responding", QDRANT_URL);
            snprintf(iss->remediation, sizeof(iss->remediation),
                     "Start Qdrant and ensure provenance_corpus collection exists");
        }

        printf("\n");
        for (int i = 0; i < 70; i++) putchar('-');
        printf("\nRESULT: FAIL (corpus unreachable)\n");
        for (int i = 0; i < 70; i++) putchar('=');
        printf("\n");
        return 1;
    }

    printf("  [INFO] Corpus online: %d points indexed\n", corpus_size);
    info++;

    if (corpus_size < 1000) {
        printf("  [WARNING] Corpus is small (%d points) -- results may be incomplete\n",
               corpus_size);
        warning++;
    }

    /* Extract key terms from project */
    printf("\n[2/4] Key Term Extraction\n");

    char terms[200][MAX_QUERY_LEN];
    int term_count = extract_key_terms(art, terms, 200);
    printf("  Extracted %d key terms from project\n", term_count);

    /* Query corpus for each term */
    printf("\n[3/4] Provenance Coverage Check\n");
    int found = 0;
    int not_found = 0;
    int query_errors = 0;

    /* Cap queries to first 50 terms to keep runtime reasonable */
    int check_count = term_count < 50 ? term_count : 50;

    for (int i = 0; i < check_count; i++) {
        int result = qdrant_scroll_count("content", terms[i]);
        if (result > 0) {
            found++;
        } else if (result == 0) {
            not_found++;
            if (not_found <= 10) {
                printf("  [WARNING] No provenance: '%s'\n", terms[i]);
            }
        } else {
            query_errors++;
        }
    }

    float coverage = 0.0f;
    if (check_count > 0) {
        coverage = (float)found / (float)check_count * 100.0f;
    }

    printf("  Checked: %d terms\n", check_count);
    printf("  Found in corpus: %d (%.0f%%)\n", found, (double)coverage);
    printf("  Not found: %d\n", not_found);
    if (query_errors > 0)
        printf("  Query errors: %d\n", query_errors);

    if (coverage < 20.0f) {
        printf("  [ERROR] Very low provenance coverage (%.0f%%)\n", (double)coverage);
        printf("    Most project concepts have no trace in the corpus\n");
        error++;
    } else if (coverage < 50.0f) {
        printf("  [WARNING] Low provenance coverage (%.0f%%)\n", (double)coverage);
        warning++;
    } else {
        printf("  [INFO] Provenance coverage adequate (%.0f%%)\n", (double)coverage);
        info++;
    }

    /* Check source file provenance */
    printf("\n[4/4] Source File Traceability\n");
    int files_with_provenance = 0;
    int files_checked = art->file_count < 20 ? art->file_count : 20;

    for (int i = 0; i < files_checked; i++) {
        const char *base = strrchr(art->files[i].path, '/');
        base = base ? base + 1 : art->files[i].path;

        int result = qdrant_scroll_count("source_file", base);
        if (result > 0) {
            files_with_provenance++;
        }
    }

    if (files_checked > 0) {
        float file_coverage = (float)files_with_provenance / (float)files_checked * 100.0f;
        printf("  Files checked: %d\n", files_checked);
        printf("  Files with corpus provenance: %d (%.0f%%)\n",
               files_with_provenance, (double)file_coverage);
    }

    /* Summary */
    printf("\n");
    for (int i = 0; i < 70; i++) putchar('-');
    printf("\nPROVENANCE COVERAGE: %.0f%% (%d/%d terms)\n",
           (double)coverage, found, check_count);
    printf("CORPUS SIZE: %d points\n", corpus_size);
    printf("SUMMARY: Critical=%d Error=%d Warning=%d Info=%d\n",
           critical, error, warning, info);

    int pass = (critical == 0);
    printf("RESULT: %s\n", pass ? "PASS" : "FAIL");
    for (int i = 0; i < 70; i++) putchar('=');
    printf("\n");

    return pass ? 0 : 1;
}

void recipe_vectordb_provenance_register(void) {
    lst_recipe_t r;
    memset(&r, 0, sizeof(r));
    snprintf(r.name, sizeof(r.name), "vectordb_provenance");
    snprintf(r.description, sizeof(r.description),
             "Validate project provenance against Qdrant vector corpus");
    r.execute = recipe_vectordb_provenance;
    r.version = 1;
    lst_recipe_register(&r);
}
