/* ============================================================================
 * SEB — Sovereign Event Bus Implementation
 * Lock-free single-producer single-consumer ring buffer
 * ============================================================================
 */

#include "seb.h"
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>

static int seb_fd(const char* name, int flags) {
    char path[256];
    snprintf(path, sizeof(path), "/dev/shm/seb_%s", name);
    return open(path, flags, 0644);
}

struct seb_ring* seb_create(const char* name) {
    int fd = seb_fd(name, O_RDWR | O_CREAT | O_EXCL);
    if (fd < 0) {
        if (errno == EEXIST) {
            return seb_open(name);
        }
        return NULL;
    }
    
    size_t total = sizeof(struct seb_ring) + SEB_RING_SIZE;
    if (ftruncate(fd, total) < 0) {
        close(fd);
        return NULL;
    }
    
    void* mem = mmap(NULL, total, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    
    if (mem == MAP_FAILED) {
        return NULL;
    }
    
    struct seb_ring* ring = mem;
    ring->magic = SEB_MAGIC;
    ring->version = 1;
    ring->size = SEB_RING_SIZE;
    ring->head = 0;
    ring->tail = 0;
    ring->dropped = 0;
    
    return ring;
}

struct seb_ring* seb_open(const char* name) {
    int fd = seb_fd(name, O_RDWR);
    if (fd < 0) {
        return NULL;
    }
    
    size_t total = sizeof(struct seb_ring) + SEB_RING_SIZE;
    void* mem = mmap(NULL, total, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    
    if (mem == MAP_FAILED) {
        return NULL;
    }
    
    struct seb_ring* ring = mem;
    if (ring->magic != SEB_MAGIC) {
        munmap(mem, total);
        return NULL;
    }
    
    return ring;
}

void seb_close(struct seb_ring* ring) {
    if (!ring) return;
    size_t total = sizeof(struct seb_ring) + SEB_RING_SIZE;
    munmap(ring, total);
}

bool seb_publish(struct seb_ring* ring, uint8_t type,
                 const void* payload, uint16_t len) {
    if (!ring || len > SEB_MAX_PAYLOAD) {
        return false;
    }
    
    uint64_t head = ring->head;
    uint64_t tail = ring->tail;
    uint64_t available = ring->size - (head - tail);
    
    uint16_t event_size = sizeof(struct seb_event) - SEB_MAX_PAYLOAD + len;
    if (available < event_size) {
        ring->dropped++;
        return false;
    }
    
    uint8_t* data = (uint8_t*)(ring + 1);
    uint64_t offset = head % ring->size;
    
    struct seb_event ev;
    ev.magic = SEB_MAGIC;
    ev.type = type;
    ev.flags = 0;
    ev.len = len;
    ev.timestamp = seb_now();
    memcpy(ev.payload, payload, len);
    
    if (offset + event_size <= ring->size) {
        memcpy(data + offset, &ev, event_size);
    } else {
        uint64_t first = ring->size - offset;
        memcpy(data + offset, &ev, first);
        memcpy(data, (uint8_t*)&ev + first, event_size - first);
    }
    
    __sync_synchronize();
    ring->head = head + event_size;
    
    return true;
}

bool seb_consume(struct seb_ring* ring, struct seb_event* out) {
    if (!ring || !out) {
        return false;
    }
    
    uint64_t head = ring->head;
    uint64_t tail = ring->tail;
    
    if (head == tail) {
        return false;  /* Empty */
    }
    
    uint8_t* data = (uint8_t*)(ring + 1);
    uint64_t offset = tail % ring->size;
    
    struct seb_event header;
    if (offset + sizeof(header) <= ring->size) {
        memcpy(&header, data + offset, sizeof(header));
    } else {
        uint64_t first = ring->size - offset;
        memcpy(&header, data + offset, first);
        memcpy((uint8_t*)&header + first, data, sizeof(header) - first);
    }
    
    if (header.magic != SEB_MAGIC) {
        ring->tail = head;  /* Corruption — reset */
        return false;
    }
    
    uint16_t event_size = sizeof(struct seb_event) - SEB_MAX_PAYLOAD + header.len;
    
    if (offset + event_size <= ring->size) {
        memcpy(out, data + offset, event_size);
    } else {
        uint64_t first = ring->size - offset;
        memcpy(out, data + offset, first);
        memcpy((uint8_t*)out + first, data, event_size - first);
    }
    
    __sync_synchronize();
    ring->tail = tail + event_size;
    
    return true;
}

bool seb_peek(struct seb_ring* ring, struct seb_event* out) {
    if (!ring || !out) {
        return false;
    }
    
    uint64_t head = ring->head;
    uint64_t tail = ring->tail;
    
    if (head == tail) {
        return false;
    }
    
    uint8_t* data = (uint8_t*)(ring + 1);
    uint64_t offset = tail % ring->size;
    
    struct seb_event header;
    if (offset + sizeof(header) <= ring->size) {
        memcpy(&header, data + offset, sizeof(header));
    } else {
        uint64_t first = ring->size - offset;
        memcpy(&header, data + offset, first);
        memcpy((uint8_t*)&header + first, data, sizeof(header) - first);
    }
    
    uint16_t event_size = sizeof(struct seb_event) - SEB_MAX_PAYLOAD + header.len;
    
    if (offset + event_size <= ring->size) {
        memcpy(out, data + offset, event_size);
    } else {
        uint64_t first = ring->size - offset;
        memcpy(out, data + offset, first);
        memcpy((uint8_t*)out + first, data, event_size - first);
    }
    
    return true;
}

uint64_t seb_now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}
