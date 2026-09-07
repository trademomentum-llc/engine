# AetherOS Telemetry Plane — Makefile
# Adapted from aetherProbe/5-makefile.sh to the telemetry/ 7-layer layout.
# All components compiled from C source; Layer 1 C API is FIXED by contract.

CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11 -D_GNU_SOURCE
LDFLAGS = -lrt -lpthread

PREFIX = /opt/aether
BINDIR = $(PREFIX)/bin
LIBDIR = $(PREFIX)/lib
INCDIR = $(PREFIX)/include

# Layout roots
PROBE_DIR = layer1-instrumentation/aether-probe
COLLECT_DIR = layer2-collection/aether-collect

# Components
PROBE_OBJS = $(PROBE_DIR)/seb.o $(PROBE_DIR)/probe.o
COLLECT_OBJS = $(COLLECT_DIR)/collect.o $(PROBE_DIR)/seb.o
TEST_OBJS = $(PROBE_DIR)/tests/test_seb.o $(PROBE_DIR)/seb.o

# Targets
all: libaether.so aether-collect test-seb

libaether.so: $(PROBE_OBJS)
	$(CC) -shared -fPIC -o $@ $^ $(LDFLAGS)

aether-collect: $(COLLECT_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

test-seb: $(TEST_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -fPIC -c -o $@ $<

clean:
	rm -f $(PROBE_OBJS) $(COLLECT_OBJS) $(TEST_OBJS)
	rm -f libaether.so aether-collect test-seb

install: all
	install -d $(BINDIR) $(LIBDIR) $(INCDIR)
	install -m 755 aether-collect $(BINDIR)/
	install -m 755 libaether.so $(LIBDIR)/
	install -m 644 $(PROBE_DIR)/seb.h $(INCDIR)/
	install -m 644 $(PROBE_DIR)/probe.h $(INCDIR)/

test: test-seb
	./test-seb

.PHONY: all clean install test
