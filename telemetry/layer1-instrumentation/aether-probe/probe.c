/* ============================================================================
 * AetherProbe Implementation
 * ============================================================================
 */

#include "probe.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

/* ----------------------------------------------------------------------------
 * Safe payload builders
 *
 * probe_append() never writes past cap and returns false on truncation, so
 * callers can abort the event instead of publishing a torn/overflowed buffer.
 * probe_append_json_string() emits a quoted, JSON-escaped string.
 * ----------------------------------------------------------------------------
 */

static bool probe_append(char* buf, size_t cap, size_t* used,
                         const char* fmt, ...) {
    if (*used >= cap) {
        return false;
    }
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf + *used, cap - *used, fmt, ap);
    va_end(ap);
    if (n < 0 || (size_t)n >= cap - *used) {
        return false;  /* Truncation or encoding error — abort the event */
    }
    *used += (size_t)n;
    return true;
}

static bool probe_append_json_string(char* buf, size_t cap, size_t* used,
                                     const char* s) {
    if (!probe_append(buf, cap, used, "\"")) {
        return false;
    }
    for (const unsigned char* p = (const unsigned char*)s; *p; p++) {
        bool ok;
        switch (*p) {
            case '"':  ok = probe_append(buf, cap, used, "\\\""); break;
            case '\\': ok = probe_append(buf, cap, used, "\\\\"); break;
            case '\n': ok = probe_append(buf, cap, used, "\\n");  break;
            case '\r': ok = probe_append(buf, cap, used, "\\r");  break;
            case '\t': ok = probe_append(buf, cap, used, "\\t");  break;
            case '\b': ok = probe_append(buf, cap, used, "\\b");  break;
            case '\f': ok = probe_append(buf, cap, used, "\\f");  break;
            default:
                if (*p < 0x20) {
                    /* Other control characters: \u00XX escape */
                    ok = probe_append(buf, cap, used, "\\u%04x", (unsigned)*p);
                } else if (*used + 1 < cap) {
                    buf[(*used)++] = (char)*p;
                    buf[*used] = '\0';
                    ok = true;
                } else {
                    ok = false;
                }
                break;
        }
        if (!ok) {
            return false;
        }
    }
    return probe_append(buf, cap, used, "\"");
}

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
    size_t used = 0;
    size_t cap = sizeof(payload);

    bool ok = probe_append(payload, cap, &used, "{\"t\":\"metric\",\"n\":") &&
              probe_append_json_string(payload, cap, &used, name) &&
              probe_append(payload, cap, &used, ",\"v\":%.6f,\"l\":{", value);

    /* i+1 < label_count guards against a dangling key when label_count is odd */
    for (uint8_t i = 0; ok && labels && i + 1 < label_count && i + 1 < 8; i += 2) {
        ok = probe_append(payload, cap, &used, "%s", i > 0 ? "," : "") &&
             probe_append_json_string(payload, cap, &used, labels[i]) &&
             probe_append(payload, cap, &used, ":") &&
             probe_append_json_string(payload, cap, &used, labels[i+1]);
    }

    ok = ok && probe_append(payload, cap, &used, "}}");
    if (!ok) {
        return false;  /* Payload did not fit — abort the event */
    }

    return seb_publish(probe->ring, SEB_METRIC, payload, (uint16_t)used);
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
    size_t used = 0;

    bool ok = probe_append(payload, sizeof(payload), &used,
                           "{\"t\":\"trace\",\"op\":") &&
              probe_append_json_string(payload, sizeof(payload), &used,
                                       operation) &&
              probe_append(payload, sizeof(payload), &used,
                           ",\"id\":%llu,\"start\":%llu}",
                           (unsigned long long)span_id,
                           (unsigned long long)seb_now());
    if (!ok) {
        return false;
    }

    return seb_publish(probe->ring, SEB_TRACE, payload, (uint16_t)used);
}

bool aether_trace_end(struct aether_probe* probe,
                      uint64_t span_id,
                      const char* status) {
    if (!probe || !probe->enabled) {
        return false;
    }

    char payload[SEB_MAX_PAYLOAD];
    size_t used = 0;

    bool ok = probe_append(payload, sizeof(payload), &used,
                           "{\"t\":\"trace_end\",\"id\":%llu,\"end\":%llu,\"s\":",
                           (unsigned long long)span_id,
                           (unsigned long long)seb_now()) &&
              probe_append_json_string(payload, sizeof(payload), &used,
                                       status ? status : "ok") &&
              probe_append(payload, sizeof(payload), &used, "}");
    if (!ok) {
        return false;
    }

    return seb_publish(probe->ring, SEB_TRACE, payload, (uint16_t)used);
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
    size_t used = 0;
    size_t cap = sizeof(payload);

    bool ok = probe_append(payload, cap, &used, "{\"t\":\"log\",\"lvl\":\"%s\",\"msg\":",
                           level_str) &&
              probe_append_json_string(payload, cap, &used, message);

    /* i+1 < field_count guards against a dangling key when field_count is odd */
    for (uint8_t i = 0; ok && fields && i + 1 < field_count && i + 1 < 8; i += 2) {
        ok = probe_append(payload, cap, &used, ",") &&
             probe_append_json_string(payload, cap, &used, fields[i]) &&
             probe_append(payload, cap, &used, ":") &&
             probe_append_json_string(payload, cap, &used, fields[i+1]);
    }

    ok = ok && probe_append(payload, cap, &used, "}");
    if (!ok) {
        return false;  /* Payload did not fit — abort the event */
    }

    return seb_publish(probe->ring, SEB_LOG, payload, (uint16_t)used);
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
    size_t used = 0;

    bool ok = probe_append(payload, sizeof(payload), &used,
                           "{\"t\":\"alert\",\"sev\":\"%s\",\"cat\":", sev_str) &&
              probe_append_json_string(payload, sizeof(payload), &used, category) &&
              probe_append(payload, sizeof(payload), &used, ",\"desc\":") &&
              probe_append_json_string(payload, sizeof(payload), &used, description) &&
              probe_append(payload, sizeof(payload), &used, ",\"ts\":%llu}",
                           (unsigned long long)seb_now());
    if (!ok) {
        return false;
    }

    return seb_publish(probe->ring, SEB_ALERT, payload, (uint16_t)used);
}

bool aether_event(struct aether_probe* probe,
                  const char* category,
                  const char* action,
                  const char* target) {
    if (!probe || !probe->enabled || !category || !action) {
        return false;
    }

    char payload[SEB_MAX_PAYLOAD];
    size_t used = 0;

    bool ok = probe_append(payload, sizeof(payload), &used,
                           "{\"t\":\"event\",\"cat\":") &&
              probe_append_json_string(payload, sizeof(payload), &used, category) &&
              probe_append(payload, sizeof(payload), &used, ",\"act\":") &&
              probe_append_json_string(payload, sizeof(payload), &used, action) &&
              probe_append(payload, sizeof(payload), &used, ",\"tgt\":") &&
              probe_append_json_string(payload, sizeof(payload), &used,
                                       target ? target : "") &&
              probe_append(payload, sizeof(payload), &used, ",\"ts\":%llu}",
                           (unsigned long long)seb_now());
    if (!ok) {
        return false;
    }

    return seb_publish(probe->ring, SEB_AUDIT, payload, (uint16_t)used);
}
