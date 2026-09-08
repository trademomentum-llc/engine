/* ============================================================================
 * AetherProbe — Telemetry Instrumentation SDK
 * Embeddable C library for emitting metrics, traces, logs, alerts
 * Zero-allocation fast path
 * ============================================================================
 */

#ifndef PROBE_H
#define PROBE_H

#include "seb.h"
#include <stdint.h>
#include <stdbool.h>

/* Probe handle */
struct aether_probe {
    struct seb_ring* ring;
    const char* service;
    const char* version;
    uint64_t start_time;
    bool enabled;
};

/* Initialize probe for a service */
bool aether_probe_init(struct aether_probe* probe, 
                       const char* service, 
                       const char* version);

void aether_probe_close(struct aether_probe* probe);

/* Metric: numeric measurement with labels */
bool aether_metric(struct aether_probe* probe,
                   const char* name,
                   double value,
                   const char** labels,      /* label pairs: key, value, key, value... */
                   uint8_t label_count);

/* Trace: start a span */
bool aether_trace_begin(struct aether_probe* probe,
                        const char* operation,
                        uint64_t* span_id_out);

bool aether_trace_end(struct aether_probe* probe,
                      uint64_t span_id,
                      const char* status);    /* "ok", "error", "timeout" */

/* Log: structured log entry */
bool aether_log(struct aether_probe* probe,
                uint8_t level,              /* 1=debug, 2=info, 3=warn, 4=error, 5=fatal */
                const char* message,
                const char** fields,
                uint8_t field_count);

/* Alert: security or integrity event */
bool aether_alert(struct aether_probe* probe,
                  uint8_t severity,         /* 1=info, 2=warning, 3=critical */
                  const char* category,     /* "drift", "auth", "resource", "integrity" */
                  const char* description);

/* Event: generic telemetry event */
bool aether_event(struct aether_probe* probe,
                  const char* category,
                  const char* action,
                  const char* target);

#endif /* PROBE_H */
