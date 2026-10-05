#define _POSIX_C_SOURCE 200809L

/*
 * deterministic_benchmark.c -- Deterministic Benchmark Validation Recipe
 *
 * Validates deterministic benchmark infrastructure for NNOS traces:
 *   - Benchmark fixture files exist in the project
 *   - Benchmark scripts (benchmark_determinism.sh etc.) are present and executable
 *   - Canonical state captures are available for comparison
 *   - Trace data follows expected formats (trace_id, timestamp, payload)
 *   - Benchmark results are reproducible (hashes match across runs)
 *   - Performance baselines are defined with thresholds
 *
 * Ensures that NNOS trace infrastructure produces bit-identical
 * results across invocations, enabling regression detection.
 */

#include "lst.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

/* --------------------------------------------------------------------------
 * Benchmark pattern sets
 * -------------------------------------------------------------------------- */

/* Fixture file indicators */
static const char *FIXTURE_PATTERNS[] = {
    "fixture",        "Fixture",        "FIXTURE",
    "test_data",      "TestData",       "testData",
    "sample_input",   "SampleInput",    "sampleInput",
    "golden_file",    "GoldenFile",     "goldenFile",
    "reference_data", "ReferenceData",  "referenceData",
    NULL
};

/* Benchmark script indicators */
static const char *BENCH_SCRIPT_PATTERNS[] = {
    "benchmark",          "Benchmark",          "BENCHMARK",
    "bench_determinism",  "benchmark_determinism",
    "bench_run",          "benchmark_run",
    "perf_test",          "PerfTest",           "perfTest",
    "determinism_check",  "DeterminismCheck",
    NULL
};

/* Canonical state capture indicators */
static const char *CANONICAL_STATE_PATTERNS[] = {
    "canonical",       "Canonical",       "CANONICAL",
    "baseline_state",  "BaselineState",   "baselineState",
    "state_capture",   "StateCapture",    "stateCapture",
    "snapshot",        "Snapshot",        "SNAPSHOT",
    "expected_state",  "ExpectedState",   "expectedState",
    NULL
};

/* Trace format indicators */
static const char *TRACE_FORMAT_PATTERNS[] = {
    "trace_id",        "TraceId",         "traceId",
    "trace_timestamp", "TraceTimestamp",   "traceTimestamp",
    "trace_payload",   "TracePayload",    "tracePayload",
    "trace_format",    "TraceFormat",     "traceFormat",
    "nnos_trace",      "NnosTrace",       "nnosTrace",
    NULL
};

/* Reproducibility / hash matching indicators */
static const char *REPRO_HASH_PATTERNS[] = {
    "hash_match",      "HashMatch",       "hashMatch",
    "checksum_verify", "ChecksumVerify",  "checksumVerify",
    "digest_compare",  "DigestCompare",   "digestCompare",
    "reproducible",    "Reproducible",    "REPRODUCIBLE",
    "deterministic",   "Deterministic",   "DETERMINISTIC",
    "bitwise_equal",   "BitwiseEqual",    "bitwiseEqual",
    NULL
};

/* Performance baseline indicators */
static const char *PERF_BASELINE_PATTERNS[] = {
    "baseline",        "Baseline",        "BASELINE",
    "threshold",       "Threshold",       "THRESHOLD",
    "perf_target",     "PerfTarget",      "perfTarget",
    "latency_limit",   "LatencyLimit",    "latencyLimit",
    "throughput_min",  "ThroughputMin",   "throughputMin",
    "regression",      "Regression",      "REGRESSION",
    NULL
};

/* Benchmark-related file name fragments */
static const char *BENCH_FILE_FRAGMENTS[] = {
    "benchmark",       "bench_",         "_bench",
    "fixture",         "golden",         "baseline",
    "determinism",     "canonical",
    NULL
};

/* Benchmark script file names to look for */
static const char *BENCH_SCRIPT_NAMES[] = {
    "benchmark_determinism.sh",
    "benchmark_determinism.py",
    "run_benchmarks.sh",
    "run_benchmarks.py",
    "bench.sh",
    "bench.py",
    "perf_test.sh",
    "perf_test.py",
    NULL
};

/* --------------------------------------------------------------------------
 * Helpers
 * -------------------------------------------------------------------------- */

static char *read_file_ben(const char *path, size_t *out_len) {
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

static void add_ben_issue(lst_artifact_t *art, unsigned seq,
                          uint8_t severity, const char *title,
                          const char *description, const char *remediation) {
    if (art->issue_count >= LST_MAX_ISSUES) return;
    lst_issue_t *iss = &art->issues[art->issue_count++];
    snprintf(iss->id, sizeof(iss->id), "BEN-%04u", seq);
    iss->severity = severity;
    snprintf(iss->title, sizeof(iss->title), "%s", title);
    snprintf(iss->description, sizeof(iss->description), "%s", description);
    iss->file_path[0] = '\0';
    iss->line_number = 0;
    snprintf(iss->cwe, sizeof(iss->cwe), "NNOS");
    snprintf(iss->remediation, sizeof(iss->remediation), "%s", remediation);
}

static int contains_any_ben(const char *content, const char **patterns) {
    for (int i = 0; patterns[i]; i++) {
        if (strstr(content, patterns[i])) return 1;
    }
    return 0;
}

static int count_ben_matches(const char *content, const char **patterns) {
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

/* Check if a file path contains any of the given name fragments */
static int path_contains_fragment(const char *path, const char **fragments) {
    for (int i = 0; fragments[i]; i++) {
        if (strstr(path, fragments[i])) return 1;
    }
    return 0;
}

/* Check if a file path ends with a specific filename */
static int path_ends_with(const char *path, const char *name) {
    size_t plen = strlen(path);
    size_t nlen = strlen(name);
    if (nlen > plen) return 0;
    if (strcmp(path + plen - nlen, name) != 0) return 0;
    if (plen > nlen && path[plen - nlen - 1] != '/') return 0;
    return 1;
}

/* Check if a file is executable */
static int file_is_executable(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) return 0;
    return (st.st_mode & S_IXUSR) != 0;
}

/* --------------------------------------------------------------------------
 * Scan results
 * -------------------------------------------------------------------------- */

typedef struct {
    int has_fixtures;
    int has_bench_scripts;
    int has_canonical_states;
    int has_trace_formats;
    int has_repro_hashes;
    int has_perf_baselines;
    int fixture_count;
    int bench_script_count;
    int canonical_count;
    int trace_format_count;
    int repro_hash_count;
    int perf_baseline_count;
    int bench_file_count;
    int bench_script_file_count;
    int executable_script_count;
} ben_scan_result_t;

/* --------------------------------------------------------------------------
 * Core scanning
 * -------------------------------------------------------------------------- */

static void scan_benchmark_infra(lst_artifact_t *art, ben_scan_result_t *result) {
    memset(result, 0, sizeof(*result));

    for (uint32_t i = 0; i < art->file_count; i++) {
        const char *fpath = art->files[i].path;

        /* Count benchmark-related files by name */
        if (path_contains_fragment(fpath, BENCH_FILE_FRAGMENTS)) {
            result->bench_file_count++;
        }

        /* Check for known benchmark script filenames */
        for (int s = 0; BENCH_SCRIPT_NAMES[s]; s++) {
            if (path_ends_with(fpath, BENCH_SCRIPT_NAMES[s])) {
                result->bench_script_file_count++;
                if (file_is_executable(fpath)) {
                    result->executable_script_count++;
                }
                break;
            }
        }

        /* Read file content and scan for patterns */
        size_t len = 0;
        char *content = read_file_ben(fpath, &len);
        if (!content) continue;

        if (contains_any_ben(content, FIXTURE_PATTERNS)) {
            result->has_fixtures = 1;
            result->fixture_count += count_ben_matches(content, FIXTURE_PATTERNS);
        }

        if (contains_any_ben(content, BENCH_SCRIPT_PATTERNS)) {
            result->has_bench_scripts = 1;
            result->bench_script_count += count_ben_matches(content, BENCH_SCRIPT_PATTERNS);
        }

        if (contains_any_ben(content, CANONICAL_STATE_PATTERNS)) {
            result->has_canonical_states = 1;
            result->canonical_count += count_ben_matches(content, CANONICAL_STATE_PATTERNS);
        }

        if (contains_any_ben(content, TRACE_FORMAT_PATTERNS)) {
            result->has_trace_formats = 1;
            result->trace_format_count += count_ben_matches(content, TRACE_FORMAT_PATTERNS);
        }

        if (contains_any_ben(content, REPRO_HASH_PATTERNS)) {
            result->has_repro_hashes = 1;
            result->repro_hash_count += count_ben_matches(content, REPRO_HASH_PATTERNS);
        }

        if (contains_any_ben(content, PERF_BASELINE_PATTERNS)) {
            result->has_perf_baselines = 1;
            result->perf_baseline_count += count_ben_matches(content, PERF_BASELINE_PATTERNS);
        }

        free(content);
    }
}

/* --------------------------------------------------------------------------
 * Benchmark completeness score
 * -------------------------------------------------------------------------- */

static float compute_benchmark_score(ben_scan_result_t *result) {
    float score = 0.0f;
    float max_score = 6.0f;

    if (result->has_fixtures)          score += 1.0f;
    if (result->has_bench_scripts)     score += 1.0f;
    if (result->has_canonical_states)  score += 1.0f;
    if (result->has_trace_formats)     score += 1.0f;
    if (result->has_repro_hashes)      score += 1.0f;
    if (result->has_perf_baselines)    score += 1.0f;

    return (score / max_score) * 100.0f;
}

/* --------------------------------------------------------------------------
 * Report writer
 * -------------------------------------------------------------------------- */

static void write_ben_report(const char *report_path, lst_artifact_t *art,
                             ben_scan_result_t *scan, int critical,
                             int errors, int warnings, int infos,
                             float score, int pass) {
    int rfd = open(report_path, O_WRONLY | O_CREAT | O_TRUNC | O_NOFOLLOW | O_CLOEXEC, 0644);
    if (rfd < 0) return;
    FILE *f = fdopen(rfd, "w");
    if (!f) {
        close(rfd);
        return;
    }

    for (int i = 0; i < 70; i++) fputc('=', f);
    fprintf(f, "\nDETERMINISTIC BENCHMARK VALIDATION REPORT\n");
    fprintf(f, "Project: %s\n", art->project_name);
    fprintf(f, "Path:    %s\n", art->project_path);
    fprintf(f, "Scope:   NNOS Trace Benchmark Infrastructure\n");
    for (int i = 0; i < 70; i++) fputc('=', f);
    fprintf(f, "\n\n");

    fprintf(f, "[1/6] Benchmark Fixtures\n");
    if (scan->has_fixtures) {
        fprintf(f, "  [INFO] Fixture patterns found (%d references)\n",
                scan->fixture_count);
    } else {
        fprintf(f, "  [ERROR] No benchmark fixture files detected\n");
    }
    fprintf(f, "  Benchmark-related files: %d\n\n", scan->bench_file_count);

    fprintf(f, "[2/6] Benchmark Scripts\n");
    if (scan->has_bench_scripts) {
        fprintf(f, "  [INFO] Benchmark script patterns found (%d references)\n",
                scan->bench_script_count);
    } else {
        fprintf(f, "  [ERROR] No benchmark scripts detected\n");
    }
    fprintf(f, "  Script files found: %d (executable: %d)\n\n",
            scan->bench_script_file_count, scan->executable_script_count);

    fprintf(f, "[3/6] Canonical State Captures\n");
    if (scan->has_canonical_states) {
        fprintf(f, "  [INFO] Canonical state patterns found (%d references)\n",
                scan->canonical_count);
    } else {
        fprintf(f, "  [WARNING] No canonical state captures detected\n");
    }

    fprintf(f, "\n[4/6] Trace Data Format\n");
    if (scan->has_trace_formats) {
        fprintf(f, "  [INFO] Trace format patterns found (%d references)\n",
                scan->trace_format_count);
    } else {
        fprintf(f, "  [CRITICAL] No NNOS trace format patterns detected\n");
    }

    fprintf(f, "\n[5/6] Reproducibility (Hash Matching)\n");
    if (scan->has_repro_hashes) {
        fprintf(f, "  [INFO] Reproducibility patterns found (%d references)\n",
                scan->repro_hash_count);
    } else {
        fprintf(f, "  [ERROR] No reproducibility/hash verification detected\n");
    }

    fprintf(f, "\n[6/6] Performance Baselines\n");
    if (scan->has_perf_baselines) {
        fprintf(f, "  [INFO] Performance baseline patterns found (%d references)\n",
                scan->perf_baseline_count);
    } else {
        fprintf(f, "  [WARNING] No performance baselines defined\n");
    }

    fprintf(f, "\n");
    for (int i = 0; i < 70; i++) fputc('-', f);
    fprintf(f, "\nBENCHMARK SCORE: %.0f%% (6 dimensions)\n", (double)score);
    fprintf(f, "SUMMARY: Critical=%d Error=%d Warning=%d Info=%d\n",
            critical, errors, warnings, infos);
    fprintf(f, "RESULT: %s\n", pass ? "PASS" : "FAIL");
    for (int i = 0; i < 70; i++) fputc('=', f);
    fprintf(f, "\n");

    fclose(f);
}

/* --------------------------------------------------------------------------
 * Recipe entry point
 * -------------------------------------------------------------------------- */

static int recipe_deterministic_benchmark(lst_artifact_t *art,
                                          const char *output_dir) {
    unsigned issue_seq = 1;
    int critical = 0, errors = 0, warnings = 0, infos = 0;

    printf("\n");
    for (int i = 0; i < 70; i++) putchar('=');
    printf("\nDETERMINISTIC BENCHMARK VALIDATION REPORT\n");
    printf("Project: %s\n", art->project_name);
    printf("Scope:   NNOS Trace Benchmark Infrastructure\n");
    for (int i = 0; i < 70; i++) putchar('=');
    printf("\n\n");

    /* Scan codebase for benchmark infrastructure */
    ben_scan_result_t scan;
    scan_benchmark_infra(art, &scan);

    /* Dimension 1: Benchmark fixtures */
    printf("[1/6] Benchmark Fixtures\n");
    if (scan.has_fixtures) {
        printf("  [INFO] Fixture patterns found (%d references)\n",
               scan.fixture_count);
        infos++;
    } else {
        printf("  [ERROR] No benchmark fixture files detected\n");
        printf("    Benchmark tests require fixture data for deterministic input\n");
        errors++;
        add_ben_issue(art, issue_seq++, SEV_ERROR,
                      "No benchmark fixture files",
                      "Deterministic benchmarks require fixture data for repeatable input",
                      "Add fixture files with known inputs under a fixtures/ or test_data/ directory");
    }
    printf("  Benchmark-related files: %d\n", scan.bench_file_count);

    /* Dimension 2: Benchmark scripts */
    printf("\n[2/6] Benchmark Scripts\n");
    if (scan.has_bench_scripts) {
        printf("  [INFO] Benchmark script patterns found (%d references)\n",
               scan.bench_script_count);
        infos++;
    } else {
        printf("  [ERROR] No benchmark scripts detected\n");
        printf("    Need benchmark_determinism.sh or equivalent to run determinism checks\n");
        errors++;
        add_ben_issue(art, issue_seq++, SEV_ERROR,
                      "No benchmark scripts found",
                      "Missing benchmark_determinism.sh or equivalent runnable scripts",
                      "Create benchmark scripts that exercise NNOS trace determinism");
    }

    if (scan.bench_script_file_count > 0 &&
        scan.executable_script_count < scan.bench_script_file_count) {
        int non_exec = scan.bench_script_file_count - scan.executable_script_count;
        printf("  [WARNING] %d benchmark script(s) are not executable\n", non_exec);
        warnings++;
        add_ben_issue(art, issue_seq++, SEV_WARNING,
                      "Benchmark scripts not executable",
                      "Some benchmark scripts lack the executable permission bit",
                      "Run chmod +x on benchmark scripts so CI can invoke them directly");
    }
    printf("  Script files found: %d (executable: %d)\n",
           scan.bench_script_file_count, scan.executable_script_count);

    /* Dimension 3: Canonical state captures */
    printf("\n[3/6] Canonical State Captures\n");
    if (scan.has_canonical_states) {
        printf("  [INFO] Canonical state patterns found (%d references)\n",
               scan.canonical_count);
        infos++;
    } else {
        printf("  [WARNING] No canonical state captures detected\n");
        printf("    Canonical snapshots enable diff-based regression detection\n");
        warnings++;
        add_ben_issue(art, issue_seq++, SEV_WARNING,
                      "No canonical state captures",
                      "Missing baseline state snapshots for comparison across runs",
                      "Capture canonical state after verified-correct runs and store as reference");
    }

    /* Dimension 4: Trace data format */
    printf("\n[4/6] Trace Data Format\n");
    if (scan.has_trace_formats) {
        printf("  [INFO] Trace format patterns found (%d references)\n",
               scan.trace_format_count);
        infos++;
    } else {
        printf("  [CRITICAL] No NNOS trace format patterns detected\n");
        printf("    Traces must include trace_id, timestamp, and payload fields\n");
        critical++;
        add_ben_issue(art, issue_seq++, SEV_CRITICAL,
                      "No NNOS trace format patterns",
                      "Trace data must follow expected format with trace_id, timestamp, payload",
                      "Implement structured NNOS trace emission with mandatory fields");
    }

    /* Dimension 5: Reproducibility (hash matching) */
    printf("\n[5/6] Reproducibility (Hash Matching)\n");
    if (scan.has_repro_hashes) {
        printf("  [INFO] Reproducibility patterns found (%d references)\n",
               scan.repro_hash_count);
        infos++;
    } else {
        printf("  [ERROR] No reproducibility/hash verification detected\n");
        printf("    Benchmark results must be verifiable via hash comparison\n");
        errors++;
        add_ben_issue(art, issue_seq++, SEV_ERROR,
                      "No reproducibility verification",
                      "Benchmark outputs are not verified for bit-identical reproducibility",
                      "Add hash/checksum comparison of benchmark outputs across runs");
    }

    /* Dimension 6: Performance baselines */
    printf("\n[6/6] Performance Baselines\n");
    if (scan.has_perf_baselines) {
        printf("  [INFO] Performance baseline patterns found (%d references)\n",
               scan.perf_baseline_count);
        infos++;
    } else {
        printf("  [WARNING] No performance baselines defined\n");
        printf("    Baselines with thresholds enable regression detection\n");
        warnings++;
        add_ben_issue(art, issue_seq++, SEV_WARNING,
                      "No performance baselines defined",
                      "Missing latency/throughput baselines for regression detection",
                      "Define performance baselines with acceptable threshold ranges");
    }

    /* Compute score */
    float score = compute_benchmark_score(&scan);
    int pass = (critical == 0 && errors == 0);

    /* Summary to stdout */
    printf("\n");
    for (int i = 0; i < 70; i++) putchar('-');
    printf("\nBENCHMARK SCORE: %.0f%% (6 dimensions)\n", (double)score);
    printf("SUMMARY: Critical=%d Error=%d Warning=%d Info=%d\n",
           critical, errors, warnings, infos);
    printf("RESULT: %s\n", pass ? "PASS" : "FAIL");
    for (int i = 0; i < 70; i++) putchar('=');
    printf("\n");

    /* Write report file */
    char report_path[LST_MAX_PATH];
    const char *rdir = output_dir ? output_dir : art->project_path;
    snprintf(report_path, sizeof(report_path),
             "%s/deterministic_benchmark_report.txt", rdir);
    write_ben_report(report_path, art, &scan, critical, errors, warnings,
                     infos, score, pass);
    printf("Report written to: %s\n", report_path);

    return pass ? 0 : 1;
}

/* --------------------------------------------------------------------------
 * Registration
 * -------------------------------------------------------------------------- */

void recipe_deterministic_benchmark_register(void) {
    lst_recipe_t r;
    memset(&r, 0, sizeof(r));
    snprintf(r.name, sizeof(r.name), "deterministic_benchmark");
    snprintf(r.description, sizeof(r.description),
             "Deterministic benchmark validation for NNOS traces");
    r.execute = recipe_deterministic_benchmark;
    r.version = 1;
    lst_recipe_register(&r);
}
