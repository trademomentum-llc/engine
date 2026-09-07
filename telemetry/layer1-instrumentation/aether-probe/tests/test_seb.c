/* ============================================================================
 * test_seb — SEB ring buffer verification suite (Layer 1 contract test)
 *
 * Cases per SPEC.md "Layer 1 API":
 *   1. create ring "test", publish N=1000 mixed-type events, consume all,
 *      assert count + ordering + dropped==0
 *   2. overflow: publish beyond 64KB capacity without consuming,
 *      assert dropped>0
 *
 * Prints PASS/FAIL per case; exit status non-zero on any failure.
 * ============================================================================
 */

#include "../seb.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define TEST_RING     "test"
#define TEST_RING_OVF "test-overflow"
#define N_EVENTS      1000

static int failures = 0;

static void report(const char* name, bool ok) {
    printf("%s: %s\n", name, ok ? "PASS" : "FAIL");
    if (!ok) {
        failures++;
    }
}

/* Deterministic unlink so reruns never attach to a stale ring. */
static void cleanup_ring(const char* name) {
    char path[256];
    snprintf(path, sizeof(path), "/dev/shm/seb_%s", name);
    unlink(path);
}

/* Case 1: N=1000 mixed-type events, FIFO order, zero drops. */
static void test_publish_consume_1000(void) {
    bool ok = true;
    cleanup_ring(TEST_RING);

    struct seb_ring* ring = seb_create(TEST_RING);
    if (!ring) {
        printf("test_publish_consume_1000: FAIL (seb_create returned NULL"
               " — /dev/shm unavailable?)\n");
        failures++;
        return;
    }

    /* Publish 1000 events, types cycling 1..5 (SEB_METRIC..SEB_AUDIT),
     * payload embeds the sequence number so order can be verified. */
    for (int i = 0; i < N_EVENTS; i++) {
        uint8_t type = (uint8_t)((i % 5) + 1);
        char payload[64];
        int len = snprintf(payload, sizeof(payload),
                           "{\"seq\":%d,\"type\":%u}", i, (unsigned)type);
        if (!seb_publish(ring, type, payload, (uint16_t)len)) {
            printf("  publish failed at i=%d (unexpected; ring too small?)\n", i);
            ok = false;
            break;
        }
    }

    if (ring->dropped != 0) {
        printf("  dropped=%llu during fill (expected 0)\n",
               (unsigned long long)ring->dropped);
        ok = false;
    }

    /* Consume all; assert count, FIFO order, mixed types, monotonic ts. */
    struct seb_event ev;
    uint64_t prev_ts = 0;
    int consumed = 0;
    while (seb_consume(ring, &ev)) {
        uint8_t expect_type = (uint8_t)((consumed % 5) + 1);
        int seq = -1;
        if (ev.magic != SEB_MAGIC) {
            printf("  bad magic at event %d\n", consumed);
            ok = false;
            break;
        }
        if (ev.type != expect_type) {
            printf("  type mismatch at event %d: got %u want %u\n",
                   consumed, (unsigned)ev.type, (unsigned)expect_type);
            ok = false;
        }
        char text[SEB_MAX_PAYLOAD + 1];
        memcpy(text, ev.payload, ev.len);
        text[ev.len] = '\0';
        if (sscanf(text, "{\"seq\":%d", &seq) != 1 || seq != consumed) {
            printf("  order mismatch at event %d: seq=%d payload=%.*s\n",
                   consumed, seq, ev.len, ev.payload);
            ok = false;
        }
        if (ev.timestamp < prev_ts) {
            printf("  non-monotonic timestamp at event %d\n", consumed);
            ok = false;
        }
        prev_ts = ev.timestamp;
        consumed++;
    }

    if (consumed != N_EVENTS) {
        printf("  consumed=%d, expected %d\n", consumed, N_EVENTS);
        ok = false;
    }
    if (ring->dropped != 0) {
        printf("  dropped=%llu after drain (expected 0)\n",
               (unsigned long long)ring->dropped);
        ok = false;
    }

    report("test_publish_consume_1000", ok);
    seb_close(ring);
    cleanup_ring(TEST_RING);
}

/* Case 2: overflow — publishing without consuming must drop, not corrupt. */
static void test_overflow_drops(void) {
    bool ok = true;
    cleanup_ring(TEST_RING_OVF);

    struct seb_ring* ring = seb_create(TEST_RING_OVF);
    if (!ring) {
        printf("test_overflow_drops: FAIL (seb_create returned NULL"
               " — /dev/shm unavailable?)\n");
        failures++;
        return;
    }

    /* Max-size events: 16B header + 1024B payload = 1040B each.
     * 64KB ring holds 63; publishing 200 must drop ~137. */
    char payload[SEB_MAX_PAYLOAD];
    memset(payload, 'A', sizeof(payload));

    int published = 0;
    for (int i = 0; i < 200; i++) {
        if (seb_publish(ring, SEB_LOG, payload, SEB_MAX_PAYLOAD)) {
            published++;
        }
    }

    if (published >= 200) {
        printf("  all 200 max-size events fit — impossible in 64KB ring\n");
        ok = false;
    }
    if (ring->dropped == 0) {
        printf("  dropped==0 after overflow (expected >0)\n");
        ok = false;
    }
    if ((uint64_t)published + ring->dropped != 200) {
        printf("  accounting mismatch: published=%d dropped=%llu total=%d\n",
               published, (unsigned long long)ring->dropped, 200);
        ok = false;
    }

    /* Ring must remain coherent: every stored event consumes cleanly. */
    struct seb_event ev;
    int drained = 0;
    while (seb_consume(ring, &ev)) {
        if (ev.magic != SEB_MAGIC || ev.len != SEB_MAX_PAYLOAD) {
            printf("  corrupt event after overflow at drain %d\n", drained);
            ok = false;
            break;
        }
        drained++;
    }
    if (drained != published) {
        printf("  drained=%d != published=%d after overflow\n",
               drained, published);
        ok = false;
    }

    report("test_overflow_drops", ok);
    seb_close(ring);
    cleanup_ring(TEST_RING_OVF);
}

int main(void) {
    printf("=== SEB ring verification (SPEC Layer 1) ===\n");
    test_publish_consume_1000();
    test_overflow_drops();
    printf("=== %s (%d failure%s) ===\n",
           failures == 0 ? "ALL PASS" : "FAILURES",
           failures, failures == 1 ? "" : "s");
    return failures == 0 ? 0 : 1;
}
