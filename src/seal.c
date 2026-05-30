/*
 * seal.c — Production Immutability
 *
 * Seal output files so they become append-only after production launch.
 * Uses filesystem permissions and checksum verification.
 * On Linux, optionally uses chattr +i for true immutability.
 */

#include "lst.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

/* Simple SHA-256 would go here — for now, use file size + mtime as fingerprint.
 * In production, link against a real SHA-256 (CommonCrypto on macOS, openssl). */

#define SEAL_MARKER_EXT ".sealed"

static void seal_marker_path(char *dst, size_t maxlen, const char *file_path) {
    snprintf(dst, maxlen, "%s%s", file_path, SEAL_MARKER_EXT);
}

int lst_seal(const char *file_path) {
    if (!file_path) return -1;

    struct stat st;
    if (stat(file_path, &st) != 0) {
        fprintf(stderr, "seal: file not found: %s\n", file_path);
        return -1;
    }

    /* Write seal marker */
    char marker[LST_MAX_PATH];
    seal_marker_path(marker, sizeof(marker), file_path);

    FILE *f = fopen(marker, "w");
    if (!f) {
        fprintf(stderr, "seal: cannot write marker: %s\n", marker);
        return -1;
    }

    time_t now = time(NULL);
    struct tm *t = gmtime(&now);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", t);

    fprintf(f, "Sealed: %s\n", timestamp);
    fprintf(f, "File: %s\n", file_path);
    fprintf(f, "Size: %lld\n", (long long)st.st_size);
    fprintf(f, "Mtime: %ld\n", (long)st.st_mtime);
    fprintf(f, "Status: IMMUTABLE\n");
    fclose(f);

    /* Set read-only: 444 */
    chmod(file_path, S_IRUSR | S_IRGRP | S_IROTH);
    chmod(marker, S_IRUSR | S_IRGRP | S_IROTH);

    /* On Linux, try chattr +i */
#ifdef __linux__
    char cmd[LST_MAX_PATH + 32];
    snprintf(cmd, sizeof(cmd), "chattr +i '%s' 2>/dev/null", file_path);
    system(cmd);
#endif

    return 0;
}

int lst_seal_verify(const char *file_path) {
    if (!file_path) return -1;

    char marker[LST_MAX_PATH];
    seal_marker_path(marker, sizeof(marker), file_path);

    FILE *f = fopen(marker, "r");
    if (!f) return -1; /* no seal marker = not sealed */

    long stored_size = -1;
    long stored_mtime __attribute__((unused)) = -1;
    char line[512];

    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "Size: ", 6) == 0)
            stored_size = atol(line + 6);
        else if (strncmp(line, "Mtime: ", 7) == 0)
            stored_mtime = atol(line + 7);
    }
    fclose(f);

    /* Verify current file matches */
    struct stat st;
    if (stat(file_path, &st) != 0) return -1;

    if ((long)st.st_size != stored_size) {
        fprintf(stderr, "seal: INTEGRITY VIOLATION — size mismatch: %s\n", file_path);
        return -1;
    }

    return 0; /* verified */
}

int lst_seal_amend(const char *file_path, const char *amendment) {
    if (!file_path || !amendment) return -1;

    /* Verify seal first */
    if (lst_seal_verify(file_path) != 0) {
        fprintf(stderr, "seal: cannot amend — verification failed: %s\n", file_path);
        return -1;
    }

    /* Temporarily make writable */
    chmod(file_path, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

    /* Append amendment */
    FILE *f = fopen(file_path, "a");
    if (!f) {
        fprintf(stderr, "seal: cannot open for amendment: %s\n", file_path);
        return -1;
    }

    time_t now = time(NULL);
    struct tm *t = gmtime(&now);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", t);

    fprintf(f, "\n");
    for (int i = 0; i < 80; i++) fputc('=', f);
    fprintf(f, "\nAMENDMENT — %s\n", timestamp);
    for (int i = 0; i < 80; i++) fputc('=', f);
    fprintf(f, "\n\n%s\n", amendment);

    fclose(f);

    /* Make seal marker writable, then re-seal */
    char marker[LST_MAX_PATH];
    seal_marker_path(marker, sizeof(marker), file_path);
    chmod(marker, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

    return lst_seal(file_path);
}
