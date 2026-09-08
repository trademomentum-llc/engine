/* ============================================================================
 * test_probe — AetherProbe payload-construction verification (Layer 1)
 *
 * Cases (security remediation findings P1/P2):
 *   T3a. odd label_count: must not read past the labels array, must not
 *        crash, and must emit well-formed JSON (dangling key dropped).
 *   T3b. oversized input: 900-char metric name + labels must return false
 *        (event aborted on truncation), not crash or overflow the stack.
 *   T4.  JSON escaping: a log message containing quotes/backslashes/newlines
 *        must be emitted with \" \\ \n escapes so downstream JSON stays
 *        well-formed.
 *
 * Prints PASS/FAIL per case; exit status non-zero on any failure.
 * ============================================================================
 */

#include "../probe.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define PROBE_RING_T3 "test-probe-t3"
#define PROBE_RING_T4 "test-probe-t4"

static int failures = 0;

static void report(const char* name, bool ok) {
    printf("%s: %s\n", name, ok ? "PASS" : "FAIL");
    if (!ok) {
        failures++;
    }
}

static void cleanup_ring(const char* name) {
    char path[256];
    snprintf(path, sizeof(path), "/dev/shm/seb_%s", name);
    unlink(path);
}

/* Minimal well-formedness check: braces balanced, brackets balanced, and an
 * even number of unescaped quotes (i.e. no string is left unterminated and
 * no raw quote splits a token). Not a full JSON parser — a structural guard. */
static bool json_shape_ok(const char* s, size_t len) {
    int braces = 0;
    int quotes = 0;
    bool esc = false;
    for (size_t i = 0; i < len; i++) {
        char c = s[i];
        if (esc) {
            esc = false;
            continue;
        }
        if (c == '\\') {
            esc = true;
        } else if (c == '"') {
            quotes++;
        } else if (c == '{') {
            braces++;
        } else if (c == '}') {
            braces--;
            if (braces < 0) {
                return false;
            }
        }
    }
    return braces == 0 && !esc && (quotes % 2) == 0;
}

/* Drain one event of the expected type into a NUL-terminated text buffer. */
static bool consume_one(struct aether_probe* probe, uint8_t expect_type,
                        char* text, size_t text_cap, size_t* text_len) {
    struct seb_event ev;
    if (!seb_consume(probe->ring, &ev)) {
        return false;
    }
    if (ev.magic != SEB_MAGIC || ev.type != expect_type ||
        ev.len > SEB_MAX_PAYLOAD || ev.len >= text_cap) {
        return false;
    }
    memcpy(text, ev.payload, ev.len);
    text[ev.len] = '\0';
    *text_len = ev.len;
    return true;
}

/* T3a: odd label_count drops the dangling key and emits well-formed JSON. */
static void test_odd_label_count(void) {
    bool ok = true;
    cleanup_ring(PROBE_RING_T3);

    struct aether_probe probe;
    if (!aether_probe_init(&probe, PROBE_RING_T3, "0.0.0-test")) {
        printf("test_odd_label_count: FAIL (probe init failed)\n");
        failures++;
        return;
    }

    /* 3 entries = 1 full pair + 1 dangling key (no value after it). */
    const char* labels[3] = { "host", "node-1", "dangling_key" };
    if (!aether_metric(&probe, "cpu_load", 0.75, labels, 3)) {
        printf("  aether_metric with odd label_count returned false\n");
        ok = false;
    }

    char text[SEB_MAX_PAYLOAD + 1];
    size_t len = 0;
    if (!consume_one(&probe, SEB_METRIC, text, sizeof(text), &len)) {
        printf("  failed to consume metric event\n");
        ok = false;
    } else {
        if (!json_shape_ok(text, len)) {
            printf("  malformed JSON: %s\n", text);
            ok = false;
        }
        if (!strstr(text, "\"host\":\"node-1\"")) {
            printf("  complete label pair missing: %s\n", text);
            ok = false;
        }
        if (strstr(text, "dangling_key")) {
            printf("  dangling key was not dropped: %s\n", text);
            ok = false;
        }
    }

    report("test_odd_label_count", ok);
    aether_probe_close(&probe);
    cleanup_ring(PROBE_RING_T3);
}

/* T3b: oversized input must abort (return false), never crash/overflow. */
static void test_oversized_metric_aborts(void) {
    bool ok = true;
    cleanup_ring(PROBE_RING_T3);

    struct aether_probe probe;
    if (!aether_probe_init(&probe, PROBE_RING_T3, "0.0.0-test")) {
        printf("test_oversized_metric_aborts: FAIL (probe init failed)\n");
        failures++;
        return;
    }

    char big_name[901];
    memset(big_name, 'n', 900);
    big_name[900] = '\0';

    /* 900-char name plus long label values: far beyond SEB_MAX_PAYLOAD. */
    char big_val[201];
    memset(big_val, 'v', 200);
    big_val[200] = '\0';
    const char* big_labels[4] = { "host", big_val, "region", big_val };
    uint64_t head_before = probe.ring->head;

    if (aether_metric(&probe, big_name, 1.0, big_labels, 4)) {
        printf("  900-char name + labels should not fit in %u-byte payload\n",
               (unsigned)SEB_MAX_PAYLOAD);
        ok = false;
    }
    if (probe.ring->head != head_before) {
        printf("  aborted event advanced ring head\n");
        ok = false;
    }

    /* Probe must still be functional after the aborted event. */
    const char* labels[4] = { "host", "node-1", "region", "eu-west" };
    if (!aether_metric(&probe, "ok_metric", 1.0, labels, 4)) {
        printf("  probe broken after oversized input\n");
        ok = false;
    } else {
        char text[SEB_MAX_PAYLOAD + 1];
        size_t len = 0;
        if (!consume_one(&probe, SEB_METRIC, text, sizeof(text), &len) ||
            !json_shape_ok(text, len)) {
            printf("  post-abort metric malformed: %s\n", text);
            ok = false;
        }
    }

    report("test_oversized_metric_aborts", ok);
    aether_probe_close(&probe);
    cleanup_ring(PROBE_RING_T3);
}

/* T4: quotes/backslashes/newlines in a log message must be JSON-escaped. */
static void test_log_escaping(void) {
    bool ok = true;
    cleanup_ring(PROBE_RING_T4);

    struct aether_probe probe;
    if (!aether_probe_init(&probe, PROBE_RING_T4, "0.0.0-test")) {
        printf("test_log_escaping: FAIL (probe init failed)\n");
        failures++;
        return;
    }

    const char* fields[2] = { "path", "C:\\tmp\\x" };
    if (!aether_log(&probe, 2, "say \"hi\"\nbye", fields, 2)) {
        printf("  aether_log returned false\n");
        ok = false;
    }

    char text[SEB_MAX_PAYLOAD + 1];
    size_t len = 0;
    if (!consume_one(&probe, SEB_LOG, text, sizeof(text), &len)) {
        printf("  failed to consume log event\n");
        ok = false;
    } else {
        if (!json_shape_ok(text, len)) {
            printf("  malformed JSON: %s\n", text);
            ok = false;
        }
        if (!strstr(text, "\\\"hi\\\"")) {
            printf("  quotes not escaped (want \\\"hi\\\"): %s\n", text);
            ok = false;
        }
        if (!strstr(text, "\\n")) {
            printf("  newline not escaped (want \\n): %s\n", text);
            ok = false;
        }
        if (!strstr(text, "C:\\\\tmp\\\\x")) {
            printf("  backslashes not escaped (want C:\\\\tmp\\\\x): %s\n", text);
            ok = false;
        }
        /* No raw control characters may survive into the payload. */
        for (size_t i = 0; i < len; i++) {
            if ((unsigned char)text[i] < 0x20) {
                printf("  raw control char 0x%02x at offset %zu\n",
                       (unsigned char)text[i], i);
                ok = false;
                break;
            }
        }
    }

    report("test_log_escaping", ok);
    aether_probe_close(&probe);
    cleanup_ring(PROBE_RING_T4);
}

int main(void) {
    printf("=== AetherProbe payload verification (findings P1/P2) ===\n");
    test_odd_label_count();
    test_oversized_metric_aborts();
    test_log_escaping();
    printf("=== %s (%d failure%s) ===\n",
           failures == 0 ? "ALL PASS" : "FAILURES",
           failures, failures == 1 ? "" : "s");
    return failures == 0 ? 0 : 1;
}
