/*
 * seal.c — Production Immutability
 *
 * Seal output files so they become append-only after production launch.
 * Uses filesystem permissions and checksum verification.
 * On Linux, optionally uses chattr +i for true immutability.
 *
 * Security contract:
 *   - Every caller-supplied path is canonicalized with realpath() before
 *     any filesystem operation. realpath() resolves ".." and follows
 *     symlinks at every level of the directory tree, so the canonical
 *     result can legitimately land outside the directory the caller
 *     composed — no confinement to any base directory is claimed or
 *     enforced here. What is guaranteed: (1) the canonical form is the
 *     path spelling used to locate the resolved parent directory, and
 *     (2) file and marker opens then happen relative to the same parent
 *     directory descriptor with O_NOFOLLOW, rejecting a symlink at the
 *     final resolved component. The seal marker is composed as
 *     "<canonical path>.sealed" and lands beside the resolved file,
 *     wherever that resolution points.
 *   - Permission changes are performed with fchmod() on an O_NOFOLLOW
 *     file descriptor, never via a stat-then-chmod-by-name pair.
 *   - chattr is run via fork/execvp with an argv array — no shell.
 */

#define _XOPEN_SOURCE 700
#define _POSIX_C_SOURCE 200809L

#include "lst.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

/* Simple SHA-256 would go here — for now, use file size + mtime as fingerprint.
 * In production, link against a real SHA-256 (CommonCrypto on macOS, openssl). */

#define SEAL_MARKER_EXT ".sealed"

/* Build "<file_path>.sealed". Returns -1 if it would not fit — no silent
 * truncation. */
static int seal_marker_path(char *dst, size_t maxlen, const char *file_path) {
    size_t flen = strlen(file_path);
    size_t xlen = strlen(SEAL_MARKER_EXT);
    if (flen + xlen + 1 > maxlen) return -1;
    memcpy(dst, file_path, flen);
    memcpy(dst + flen, SEAL_MARKER_EXT, xlen + 1);
    return 0;
}

/* Canonicalize path with realpath() (path must exist). Returns -1 on failure
 * or if the result would not fit — no silent truncation.
 * Note: realpath() follows symlinks in every directory component, so the
 * result may resolve outside the directory the caller composed. That is
 * accepted here — canonicalization, not confinement. */
static int seal_canonicalize(const char *path, char *dst, size_t maxlen) {
    char *rp = realpath(path, NULL);
    if (!rp) return -1;
    size_t len = strlen(rp);
    if (len == 0 || len >= maxlen) {
        free(rp);
        return -1;
    }
    memcpy(dst, rp, len + 1);
    free(rp);
    return 0;
}

/* Open the seal marker without following a hostile final-component symlink. */
static FILE *seal_marker_open(const char *marker, const char *mode) {
    int flags = O_NOFOLLOW | O_CLOEXEC;
    if (mode[0] == 'w')
        flags |= O_WRONLY | O_CREAT | O_TRUNC;
    else
        flags |= O_RDONLY;
    int fd = open(marker, flags, 0644);
    if (fd < 0) return NULL;
    FILE *f = fdopen(fd, mode);
    if (!f) {
        close(fd);
        return NULL;
    }
    return f;
}

/* Best-effort fchmod of path via a fresh O_NOFOLLOW descriptor — same
 * "ignore the result" semantics as the historical chmod-by-name calls. */
static void seal_fchmod_best_effort_at(int dirfd, const char *path, mode_t mode) {
    int fd = openat(dirfd, path, O_RDONLY | O_NOFOLLOW | O_CLOEXEC);
    if (fd < 0) return;
    struct stat st;
    if (fstat(fd, &st) == 0 && S_ISREG(st.st_mode))
        fchmod(fd, mode);
    close(fd);
}

/* Open the canonical parent directory one component at a time so later
 * openat() calls for the data file and its marker are anchored to the same
 * verified directory descriptor. Returns the parent directory fd and copies
 * the final basename to leaf. */
static int seal_open_parent_dir(const char *canon, char *leaf, size_t leaf_size) {
    if (!canon || canon[0] != '/') return -1;

    const char *slash = strrchr(canon, '/');
    if (!slash || !slash[1]) return -1;

    size_t leaf_len = strlen(slash + 1);
    if (leaf_len == 0 || leaf_len >= leaf_size) return -1;
    memcpy(leaf, slash + 1, leaf_len + 1);

    int dirfd = open("/", O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (dirfd < 0) return -1;

    const char *p = canon + 1;
    while (p < slash) {
        const char *next = p;
        while (next < slash && *next != '/') next++;

        size_t len = (size_t)(next - p);
        if (len > 0) {
            char component[LST_MAX_NAME];
            if (len >= sizeof(component)) {
                close(dirfd);
                return -1;
            }
            memcpy(component, p, len);
            component[len] = '\0';

            int nextfd = openat(dirfd, component,
                                O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
            close(dirfd);
            if (nextfd < 0) return -1;
            dirfd = nextfd;
        }

        p = next;
        if (p < slash && *p == '/') p++;
    }

    return dirfd;
}

/* Open a data file for seal/verify inspection. O_RDONLY|O_NONBLOCK so a
 * FIFO with no writer succeeds immediately instead of blocking forever
 * (the caller's fstat then rejects it as non-regular). On EACCES — e.g.
 * an owner-write-only (0200) regular file that the historical
 * stat+chmod flow could seal — retry O_WRONLY; the descriptor is only
 * used for fstat/fchmod, never read. O_NONBLOCK on the retry makes a
 * write-only FIFO fail promptly (ENXIO) rather than block awaiting a
 * reader. O_NOFOLLOW rejects a symlink at the final resolved component.
 * Caller must fstat() and reject anything that is not a regular file. */
static int seal_open_data_fd_at(int dirfd, const char *path) {
    int fd = openat(dirfd, path, O_RDONLY | O_NOFOLLOW | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0 && errno == EACCES)
        fd = openat(dirfd, path, O_WRONLY | O_NOFOLLOW | O_NONBLOCK | O_CLOEXEC);
    return fd;
}

static FILE *seal_marker_open_at(int dirfd, const char *leaf, const char *mode) {
    char marker[LST_MAX_PATH];
    if (seal_marker_path(marker, sizeof(marker), leaf) != 0) return NULL;

    int flags = O_NOFOLLOW | O_CLOEXEC;
    if (mode[0] == 'w')
        flags |= O_WRONLY | O_CREAT | O_TRUNC;
    else
        flags |= O_RDONLY;

    int fd = openat(dirfd, marker, flags, 0644);
    if (fd < 0) return NULL;
    FILE *f = fdopen(fd, mode);
    if (!f) {
        close(fd);
        return NULL;
    }
    return f;
}

#ifdef __linux__
/* Best-effort chattr +i without a shell: fork + execvp with an argv array,
 * child stderr redirected to /dev/null, all failures ignored. Preserves the
 * historical "chattr +i '<path>' 2>/dev/null" intent exactly. */
static void seal_chattr_immutable(const char *path) {
    pid_t pid = fork();
    if (pid == 0) {
        int devnull = open("/dev/null", O_WRONLY);
        if (devnull >= 0) {
            dup2(devnull, STDERR_FILENO);
            close(devnull);
        }
        char *const args[] = { "chattr", "+i", (char *)path, NULL };
        execv("/usr/bin/chattr", args);
        _exit(127);
    }
    if (pid > 0) {
        int status;
        while (waitpid(pid, &status, 0) < 0 && errno == EINTR)
            ;
    }
}
#endif

int lst_seal(const char *file_path) {
    if (!file_path) return -1;

    /* Canonicalize: realpath() resolves ".." and follows symlinks in the
     * directory tree — the result may resolve outside the directory the
     * caller composed (same behavior as before; no confinement claimed).
     * Every path below derives from this canonical form, and the marker
     * open uses O_NOFOLLOW to reject a symlink at the final resolved
     * component. */
    char canon[LST_MAX_PATH];
    if (seal_canonicalize(file_path, canon, sizeof(canon)) != 0) {
        fprintf(stderr, "seal: file not found: %s\n", file_path);
        return -1;
    }

    char leaf[LST_MAX_NAME];
    int dirfd = seal_open_parent_dir(canon, leaf, sizeof(leaf));
    if (dirfd < 0) {
        fprintf(stderr, "seal: file not found: %s\n", file_path);
        return -1;
    }

    /* Open once by descriptor: fstat/fchmod act on the same file, closing
     * the stat-then-chmod race window. O_NONBLOCK keeps FIFOs from
     * blocking; the EACCES retry covers owner-write-only regular files. */
    int fd = seal_open_data_fd_at(dirfd, leaf);
    if (fd < 0) {
        fprintf(stderr, "seal: file not found: %s\n", file_path);
        close(dirfd);
        return -1;
    }
    struct stat st;
    if (fstat(fd, &st) != 0 || !S_ISREG(st.st_mode)) {
        fprintf(stderr, "seal: file not found: %s\n", file_path);
        close(fd);
        close(dirfd);
        return -1;
    }

    FILE *f = seal_marker_open_at(dirfd, leaf, "w");
    if (!f) {
        fprintf(stderr, "seal: cannot write marker: %s.sealed\n", canon);
        close(fd);
        close(dirfd);
        return -1;
    }

    time_t now = time(NULL);
    struct tm tm_buf;
    struct tm *t = gmtime_r(&now, &tm_buf);
    char timestamp[64];
    if (t) {
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", t);
    } else {
        snprintf(timestamp, sizeof(timestamp), "1970-01-01T00:00:00Z");
    }
    fprintf(f, "Sealed: %s\n", timestamp);
    fprintf(f, "File: %s\n", file_path);
    fprintf(f, "Size: %lld\n", (long long)st.st_size);
    fprintf(f, "Mtime: %ld\n", (long)st.st_mtime);
    fprintf(f, "Status: IMMUTABLE\n");
    /* Make the marker read-only (444) through the descriptor we already
     * hold — no path re-lookup, and it works even under a
     * read-restricting umask that would deny re-opening the marker. */
    fchmod(fileno(f), S_IRUSR | S_IRGRP | S_IROTH);
    fclose(f);

    /* Set read-only: 444 (fd-based; no path re-lookup) */
    fchmod(fd, S_IRUSR | S_IRGRP | S_IROTH);
    close(fd);
    close(dirfd);

    /* On Linux, try chattr +i */
#ifdef __linux__
    seal_chattr_immutable(canon);
#endif

    return 0;
}

int lst_seal_verify(const char *file_path) {
    if (!file_path) return -1;

    char canon[LST_MAX_PATH];
    if (seal_canonicalize(file_path, canon, sizeof(canon)) != 0)
        return -1;

    char leaf[LST_MAX_NAME];
    int dirfd = seal_open_parent_dir(canon, leaf, sizeof(leaf));
    if (dirfd < 0)
        return -1;

    FILE *f = seal_marker_open_at(dirfd, leaf, "r");
    if (!f) {
        /* Legacy fallback: files sealed through a final-component symlink
         * have their marker at "<original spelling>.sealed", beside the
         * symlink, not at the canonical path's marker. Try the as-passed
         * spelling (same O_NOFOLLOW open) before concluding the file is
         * not sealed. */
        char legacy[LST_MAX_PATH];
        if (strcmp(file_path, canon) != 0 &&
            seal_marker_path(legacy, sizeof(legacy), file_path) == 0)
            f = seal_marker_open(legacy, "r");
    }
    if (!f) {
        close(dirfd);
        return -1; /* no seal marker = not sealed */
    }

    long stored_size = -1;
    long stored_mtime = -1;
    char line[512];

    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "Size: ", 6) == 0)
            stored_size = atol(line + 6);
        else if (strncmp(line, "Mtime: ", 7) == 0)
            stored_mtime = atol(line + 7);
    }
    fclose(f);

    /* Verify current file matches: once the canonical parent directory is
     * open, both the marker and the data file are opened relative to that
     * same descriptor, so a rename after the parent is opened cannot
     * retarget one without the other. Canonicalization itself still happens
     * by pathname before this point. */
    int fd = seal_open_data_fd_at(dirfd, leaf);
    if (fd < 0) {
        close(dirfd);
        return -1;
    }
    struct stat st;
    if (fstat(fd, &st) != 0 || !S_ISREG(st.st_mode)) {
        close(fd);
        close(dirfd);
        return -1;
    }
    close(fd);
    close(dirfd);

    if ((long)st.st_size != stored_size) {
        fprintf(stderr, "seal: INTEGRITY VIOLATION — size mismatch: %s\n", file_path);
        return -1;
    }

    if ((long)st.st_mtime != stored_mtime) {
        fprintf(stderr, "seal: INTEGRITY VIOLATION — mtime mismatch: %s\n", file_path);
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

    char canon[LST_MAX_PATH];
    if (seal_canonicalize(file_path, canon, sizeof(canon)) != 0) {
        fprintf(stderr, "seal: file not found: %s\n", file_path);
        return -1;
    }

    char leaf[LST_MAX_NAME];
    int dirfd = seal_open_parent_dir(canon, leaf, sizeof(leaf));
    if (dirfd < 0) {
        fprintf(stderr, "seal: cannot open for amendment: %s\n", file_path);
        return -1;
    }

    /* Temporarily make writable via descriptor (no chmod-by-name).
     * seal_open_data_fd keeps FIFOs from blocking and covers
     * owner-write-only regular files via its EACCES retry. */
    int fd = seal_open_data_fd_at(dirfd, leaf);
    if (fd < 0) {
        fprintf(stderr, "seal: cannot open for amendment: %s\n", file_path);
        close(dirfd);
        return -1;
    }
    struct stat st;
    if (fstat(fd, &st) != 0 || !S_ISREG(st.st_mode)) {
        close(fd);
        close(dirfd);
        fprintf(stderr, "seal: cannot open for amendment: %s\n", file_path);
        return -1;
    }
    fchmod(fd, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    close(fd);

    /* Append amendment */
    int afd = openat(dirfd, leaf, O_WRONLY | O_APPEND | O_NOFOLLOW | O_CLOEXEC);
    if (afd < 0) {
        fprintf(stderr, "seal: cannot open for amendment: %s\n", file_path);
        close(dirfd);
        return -1;
    }
    FILE *f = fdopen(afd, "a");
    if (!f) {
        close(afd);
        close(dirfd);
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
    if (seal_marker_path(marker, sizeof(marker), leaf) == 0)
        seal_fchmod_best_effort_at(dirfd, marker,
                                   S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    close(dirfd);

    return lst_seal(file_path);
}
