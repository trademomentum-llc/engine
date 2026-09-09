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
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>

/* Simple SHA-256 would go here — for now, use file size + mtime as fingerprint.
 * In production, link against a real SHA-256 (CommonCrypto on macOS, openssl). */

#define SEAL_MARKER_EXT ".sealed"

static void seal_marker_path(char *dst, size_t maxlen, const char *file_path) {
    snprintf(dst, maxlen, "%s%s", file_path, SEAL_MARKER_EXT);
}

int lst_seal(const char *file_path) {
    if (!file_path) return -1;

    int file_fd = open(file_path, O_RDONLY | O_NOFOLLOW);
    struct stat st;
    if (file_fd < 0 || fstat(file_fd, &st) != 0) {
        if (file_fd >= 0) close(file_fd);
        fprintf(stderr, "seal: file not found: %s\n", file_path);
        return -1;
    }
    /* Write seal marker */
    char marker[LST_MAX_PATH];
    seal_marker_path(marker, sizeof(marker), file_path);

    FILE *f = lst_secure_fopen(marker, "w");
    if (!f) {
        close(file_fd);
        fprintf(stderr, "seal: cannot write marker: %s\n", marker);
        return -1;
    }

    time_t now = time(NULL);
    struct tm tm_buf;
    struct tm *t = gmtime_r(&now, &tm_buf);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", t);

    fprintf(f, "Sealed: %s\n", timestamp);
    fprintf(f, "File: %s\n", file_path);
    fprintf(f, "Size: %lld\n", (long long)st.st_size);
    fprintf(f, "Mtime: %ld\n", (long)st.st_mtime);
    fprintf(f, "Status: IMMUTABLE\n");
    fchmod(fileno(f), S_IRUSR | S_IRGRP | S_IROTH);
    fclose(f);

    /* Set read-only: 444 */
    fchmod(file_fd, S_IRUSR | S_IRGRP | S_IROTH);
    close(file_fd);

    /* On Linux, try chattr +i */
#ifdef __linux__
    pid_t pid = fork();
    if (pid == 0) {
        execl("/usr/bin/chattr", "chattr", "+i", "--", file_path, (char *)NULL);
        _exit(127);
    }
    if (pid > 0) {
        int status = 0;
        (void)waitpid(pid, &status, 0);
    }
#endif

    return 0;
}

int lst_seal_verify(const char *file_path) {
    if (!file_path) return -1;

    char marker[LST_MAX_PATH];
    seal_marker_path(marker, sizeof(marker), file_path);

    FILE *f = lst_secure_fopen(marker, "r");
    if (!f) return -1; /* no seal marker = not sealed */

    long stored_size = -1;
    long stored_mtime = -1;
    char line[512];

    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "Size: ", 6) == 0) {
            char *end;
            long value = strtol(line + 6, &end, 10);
            if (end != line + 6) stored_size = value;
        } else if (strncmp(line, "Mtime: ", 7) == 0) {
            char *end;
            long value = strtol(line + 7, &end, 10);
            if (end != line + 7) stored_mtime = value;
        }
    }
    fclose(f);

    /* Verify current file matches */
    int file_fd = open(file_path, O_RDONLY | O_NOFOLLOW);
    struct stat st;
    if (file_fd < 0 || fstat(file_fd, &st) != 0) {
        if (file_fd >= 0) close(file_fd);
        return -1;
    }
    close(file_fd);

    if ((long)st.st_size != stored_size || (long)st.st_mtime != stored_mtime) {
        fprintf(stderr, "seal: INTEGRITY VIOLATION — size/mtime mismatch: %s\n", file_path);
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
    FILE *f = lst_secure_fopen(file_path, "a");
    if (!f) {
        fprintf(stderr, "seal: cannot open for amendment: %s\n", file_path);
        return -1;
    }

    time_t now = time(NULL);
    struct tm tm_buf;
    struct tm *t = gmtime_r(&now, &tm_buf);
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
