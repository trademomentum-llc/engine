/* ============================================================================
 * AetherProbe Implementation
 * ============================================================================
 */

#include "probe.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

bool aether_probe_init(struct aether_probe* probe,
                       const char* service,
                       const char* version) {
    if (!probe || !service) {
        return false;
    }
    
    probe->ring = seb_create(service);
    if (!probe->ring) {
        return false;
    }
    
    probe->service = service;
    probe->version = version ? version : "unknown";
    probe->start_time = seb_now();
    probe->enabled = true;
    
    return true;
}

void aether_probe_close(struct aether_probe* probe) {
    if (!probe) return;
    seb_close(probe->ring);
    probe->ring = NULL;
    probe->enabled = false;
}

bool aether_metric(struct aether_probe* probe,
                   const char* name,
                   double value,
                   const char** labels,
                   uint8_t label_count) {
    if (!probe || !probe->enabled || !name) {
        return false;
    }
    
    char payload[SEB_MAX_PAYLOAD];
    int n = snprintf(payload, sizeof(payload),
                     "{\"t\":\"metric\",\"n\":\"%s\",\"v\":%.6f,\"l\":{",
                     name, value);
    
    for (uint8_t i = 0; i < label_count && i < 8; i += 2) {
        if (i > 0) n += snprintf(payload + n, sizeof(payload) - n, ",");
        n += snprintf(payload + n, sizeof(payload) - n,
                        "\"%s\":\"%s\"", labels[i], labels[i+1]);
    }
    
    n += snprintf(payload + n, sizeof(payload) - n, "}}");
    
    return seb_publish(probe->ring, SEB_METRIC, payload, (uint16_t)n);
}

bool aether_trace_begin(struct aether_probe* probe,
                        const char* operation,
                        uint64_t* span_id_out) {
    if (!probe || !probe->enabled || !operation) {
        return false;
    }
    
    uint64_t span_id = seb_now(); /* Use timestamp as span ID */
    if (span_id_out) {
        *span_id_out = span_id;
    }
    
    char payload[SEB_MAX_PAYLOAD];
    int n = snprintf(payload, sizeof(payload),
                     "{\"t\":\"trace\",\"op\":\"%s\",\"id\":%llu,\"start\":%llu}",
                     operation, (unsigned long long)span_id,
                     (unsigned long long)seb_now());
    
    return seb_publish(probe->ring, SEB_TRACE, payload, (uint16_t)n);
}

bool aether_trace_end(struct aether_probe* probe,
                      uint64_t span_id,
                      const char* status) {
    if (!probe || !probe->enabled) {
        return false;
    }
    
    char payload[SEB_MAX_PAYLOAD];
    int n = snprintf(payload, sizeof(payload),
                     "{\"t\":\"trace_end\",\"id\":%llu,\"end\":%llu,\"s\":\"%s\"}",
                     (unsigned long long)span_id,
                     (unsigned long long)seb_now(),
                     status ? status : "ok");
    
    return seb_publish(probe->ring, SEB_TRACE, payload, (uint16_t)n);
}

bool aether_log(struct aether_probe* probe,
                uint8_t level,
                const char* message,
                const char** fields,
                uint8_t field_count) {
    if (!probe || !probe->enabled || !message) {
        return false;
    }
    
    const char* level_str = "unknown";
    switch (level) {
        case 1: level_str = "DEBUG"; break;
        case 2: level_str = "INFO"; break;
        case 3: level_str = "WARN"; break;
        case 4: level_str = "ERROR"; break;
        case 5: level_str = "FATAL"; break;
    }
    
    char payload[SEB_MAX_PAYLOAD];
    int n = snprintf(payload, sizeof(payload),
                     "{\"t\":\"log\",\"lvl\":\"%s\",\"msg\":\"%s\"",
                     level_str, message);
    
    for (uint8_t i = 0; i < field_count && i < 8; i += 2) {
        n += snprintf(payload + n, sizeof(payload) - n,
                        ",\"%s\":\"%s\"", fields[i], fields[i+1]);
    }
    
    n += snprintf(payload + n, sizeof(payload) - n, "}");
    
    return seb_publish(probe->ring, SEB_LOG, payload, (uint16_t)n);
}

bool aether_alert(struct aether_probe* probe,
                  uint8_t severity,
                  const char* category,
                  const char* description) {
    if (!probe || !probe->enabled || !category || !description) {
        return false;
    }
    
    const char* sev_str = "unknown";
    switch (severity) {
        case 1: sev_str = "info"; break;
        case 2: sev_str = "warning"; break;
        case 3: sev_str = "critical"; break;
    }
    
    char payload[SEB_MAX_PAYLOAD];
    int n = snprintf(payload, sizeof(payload),
                     "{\"t\":\"alert\",\"sev\":\"%s\",\"cat\":\"%s\",\"desc\":\"%s\",\"ts\":%llu}",
                     sev_str, category, description,
                     (unsigned long long)seb_now());
    
    return seb_publish(probe->ring, SEB_ALERT, payload, (uint16_t)n);
}

bool aether_event(struct aether_probe* probe,
                  const char* category,
                  const char* action,
                  const char* target) {
    if (!probe || !probe->enabled || !category || !action) {
        return false;
    }
    
    char payload[SEB_MAX_PAYLOAD];
    int n = snprintf(payload, sizeof(payload),
                     "{\"t\":\"event\",\"cat\":\"%s\",\"act\":\"%s\",\"tgt\":\"%s\",\"ts\":%llu}",
                     category, action, target ? target : "",
                     (unsigned long long)seb_now());
    
    return seb_publish(probe->ring, SEB_AUDIT, payload, (uint16_t)n);
}
