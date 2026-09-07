/* ============================================================================
 * AetherCollect — Telemetry Collection & Routing
 * Reads from SEB rings, processes, routes to storage layers
 * ============================================================================
 */

#include "../../layer1-instrumentation/aether-probe/seb.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

#define MAX_RINGS 16
#define POLL_INTERVAL_US 10000  /* 10ms */

static volatile int running = 1;

static void sig_handler(int sig) {
    (void)sig;
    running = 0;
}

struct collector {
    struct seb_ring* rings[MAX_RINGS];
    int ring_count;
    const char* storage_path;
};

static bool collector_init(struct collector* c, const char* storage) {
    memset(c, 0, sizeof(*c));
    c->storage_path = storage ? storage : "/var/lib/aether";
    return true;
}

static bool collector_add_ring(struct collector* c, const char* name) {
    if (c->ring_count >= MAX_RINGS) {
        return false;
    }
    struct seb_ring* ring = seb_open(name);
    if (!ring) {
        return false;
    }
    c->rings[c->ring_count++] = ring;
    return true;
}

static void collector_close(struct collector* c) {
    for (int i = 0; i < c->ring_count; i++) {
        seb_close(c->rings[i]);
    }
    c->ring_count = 0;
}

static void process_event(struct seb_event* ev) {
    /* Route based on event type */
    switch (ev->type) {
        case SEB_METRIC:
            /* Write to metrics store */
            break;
        case SEB_TRACE:
            /* Write to trace store */
            break;
        case SEB_LOG:
            /* Write to log store */
            break;
        case SEB_ALERT:
            /* Write to alert store, trigger notifications */
            break;
        case SEB_AUDIT:
            /* Write to audit trail */
            break;
        default:
            break;
    }
    
    /* Print for now (development mode) */
    printf("[%llu] type=%d len=%d %.*s\n",
           (unsigned long long)ev->timestamp,
           ev->type, ev->len, ev->len, ev->payload);
}

static void collector_poll(struct collector* c) {
    struct seb_event ev;
    
    for (int i = 0; i < c->ring_count; i++) {
        while (seb_consume(c->rings[i], &ev)) {
            process_event(&ev);
        }
    }
}

int main(int argc, char** argv) {
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);
    
    struct collector c;
    if (!collector_init(&c, NULL)) {
        fprintf(stderr, "Failed to initialize collector\n");
        return 1;
    }
    
    /* Add rings from command line */
    for (int i = 1; i < argc; i++) {
        if (!collector_add_ring(&c, argv[i])) {
            fprintf(stderr, "Failed to open ring: %s\n", argv[i]);
        } else {
            printf("Monitoring ring: %s\n", argv[i]);
        }
    }
    
    if (c.ring_count == 0) {
        fprintf(stderr, "Usage: %s <ring_name> [ring_name...]\n", argv[0]);
        return 1;
    }
    
    printf("AetherCollect started. Press Ctrl+C to stop.\n");
    
    while (running) {
        collector_poll(&c);
        usleep(POLL_INTERVAL_US);
    }
    
    printf("\nShutting down...\n");
    collector_close(&c);
    
    return 0;
}
