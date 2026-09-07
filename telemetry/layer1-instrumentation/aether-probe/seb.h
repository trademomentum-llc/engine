/* ============================================================================
 * SEB — Sovereign Event Bus
 * Shared memory ring buffer for inter-process telemetry communication
 * Single-producer, single-consumer, lock-free
 * ============================================================================
 */

#ifndef SEB_H
#define SEB_H

#include <stdint.h>
#include <stdbool.h>

#define SEB_PAGE_SIZE     4096
#define SEB_RING_PAGES    16
#define SEB_RING_SIZE     (SEB_PAGE_SIZE * SEB_RING_PAGES)  /* 64KB */
#define SEB_MAX_PAYLOAD   1024
#define SEB_MAGIC         0x53454221  /* "SEB!" */

/* Event types */
enum seb_type {
    SEB_METRIC = 1,   /* Time-series measurement */
    SEB_TRACE  = 2,   /* Distributed trace span */
    SEB_LOG    = 3,   /* Structured log entry */
    SEB_ALERT  = 4,   /* Security/integrity alert */
    SEB_AUDIT  = 5,   /* Audit trail entry */
};

/* Event header — 16 bytes, cache-line aligned */
struct seb_event {
    uint32_t magic;      /* SEB_MAGIC */
    uint8_t  type;       /* seb_type */
    uint8_t  flags;      /* Reserved */
    uint16_t len;        /* Payload length */
    uint64_t timestamp;  /* Nanoseconds since boot */
    uint8_t  payload[SEB_MAX_PAYLOAD];
} __attribute__((packed));

/* Ring buffer control block — 64 bytes */
struct seb_ring {
    uint32_t magic;      /* SEB_MAGIC */
    uint32_t version;    /* Protocol version = 1 */
    uint64_t size;       /* Ring size in bytes */
    volatile uint64_t head;  /* Producer write position */
    volatile uint64_t tail;  /* Consumer read position */
    uint64_t dropped;    /* Events dropped (overflow) */
    uint64_t padding[5]; /* Pad to 64 bytes */
} __attribute__((aligned(64)));

/* API */
struct seb_ring* seb_create(const char* name);
struct seb_ring* seb_open(const char* name);
void seb_close(struct seb_ring* ring);

bool seb_publish(struct seb_ring* ring, uint8_t type, 
                 const void* payload, uint16_t len);
bool seb_consume(struct seb_ring* ring, struct seb_event* out);
bool seb_peek(struct seb_ring* ring, struct seb_event* out);

uint64_t seb_now(void);  /* Monotonic timestamp in nanoseconds */

#endif /* SEB_H */
