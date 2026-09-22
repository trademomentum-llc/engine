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
 *   - chattr is run via fork/execv with a fixed absolute path and an
 *     argv array — no PATH search, no shell.
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

#ifdef __linux__
#include <linux/fs.h>
#include <sys/ioctl.h>
#endif

/* Simple SHA-256 would go here — for now, use file size + mtime as fingerprint.
 * In production, link against a real SHA-256 (CommonCrypto on macOS, openssl). */

#define SEAL_MARKER_EXT ".sealed"

static FILE *seal_marker_stream(int fd, const char *mode, int require_regular);

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

/* Open a canonical absolute directory one component at a time so later
 * openat() calls stay anchored to a verified directory descriptor. */
static int seal_open_dir(const char *canon_dir) {
    if (!canon_dir || canon_dir[0] != '/') return -1;
    int dirfd = open("/", O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (dirfd < 0) return -1;

    if (strcmp(canon_dir, "/") == 0)
        return dirfd;

    const char *p = canon_dir + 1;
    while (*p) {
        const char *next = p;
        while (*next && *next != '/') next++;

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
        if (*p == '/') p++;
    }

    return dirfd;
}

/* Resolve a path's parent directory without following its final component.
 * Intermediate directories are canonicalized with realpath(); the final
 * basename is returned in leaf. */
static int seal_open_parent_dir(const char *path, char *leaf, size_t leaf_size) {
    if (!path || !*path) return -1;

    const char *slash = strrchr(path, '/');
    const char *base = slash ? slash + 1 : path;
    size_t leaf_len = strlen(base);
    if (leaf_len == 0 || leaf_len >= leaf_size) return -1;
    memcpy(leaf, base, leaf_len + 1);

    char parent[LST_MAX_PATH];
    if (!slash) {
        memcpy(parent, ".", 2);
    } else if (slash == path) {
        memcpy(parent, "/", 2);
    } else {
        size_t parent_len = (size_t)(slash - path);
        if (parent_len >= sizeof(parent)) return -1;
        memcpy(parent, path, parent_len);
        parent[parent_len] = '\0';
    }

    char canon_parent[LST_MAX_PATH];
    if (seal_canonicalize(parent, canon_parent, sizeof(canon_parent)) != 0)
        return -1;

    return seal_open_dir(canon_parent);
}

static FILE *seal_marker_stream(int fd, const char *mode, int require_regular) {
    if (require_regular) {
        struct stat st;
        if (fstat(fd, &st) != 0 || !S_ISREG(st.st_mode)) {
            close(fd);
            return NULL;
        }

        int flags = fcntl(fd, F_GETFL);
        if (flags < 0 || fcntl(fd, F_SETFL, flags & ~O_NONBLOCK) != 0) {
            close(fd);
            return NULL;
        }
    }

    FILE *f = fdopen(fd, mode);
    if (!f) {
        close(fd);
        return NULL;
    }
    return f;
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
        flags |= O_RDONLY | O_NONBLOCK;

    int fd = openat(dirfd, marker, flags, 0600);
    if (fd < 0) return NULL;
    return seal_marker_stream(fd, mode, mode[0] != 'w');
}

static int seal_verify_marker_against_stat(const char *file_path, const char *canon,
                                           int dirfd, const char *leaf,
                                           const struct stat *st) {
    FILE *f = seal_marker_open_at(dirfd, leaf, "r");
    if (!f) {
        /* Legacy fallback: files sealed through a final-component symlink
         * have their marker at "<original spelling>.sealed", beside the
         * symlink, not at the canonical path's marker. Try the as-passed
         * spelling through the same parent-dirfd + O_NOFOLLOW path before
         * concluding the file is not sealed. */
        char legacy_leaf[LST_MAX_NAME];
        int legacy_dirfd = -1;
        if (strcmp(file_path, canon) != 0)
            legacy_dirfd = seal_open_parent_dir(file_path, legacy_leaf, sizeof(legacy_leaf));
        if (legacy_dirfd >= 0) {
            f = seal_marker_open_at(legacy_dirfd, legacy_leaf, "r");
            close(legacy_dirfd);
        }
    }
    if (!f)
        return -1; /* no seal marker = not sealed */

    long stored_size = -1;
    long stored_mtime = -1;
    unsigned long long stored_dev = 0;
    unsigned long long stored_ino = 0;
    int have_mtime = 0;
    int have_dev = 0;
    int have_ino = 0;
    char line[512];

    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "Size: ", 6) == 0)
            stored_size = atol(line + 6);
        else if (strncmp(line, "Mtime: ", 7) == 0) {
            stored_mtime = atol(line + 7);
            have_mtime = 1;
        } else if (strncmp(line, "Dev: ", 5) == 0) {
            stored_dev = strtoull(line + 5, NULL, 10);
            have_dev = 1;
        } else if (strncmp(line, "Inode: ", 7) == 0) {
            stored_ino = strtoull(line + 7, NULL, 10);
            have_ino = 1;
        }
    }
    fclose(f);

    if ((long)st->st_size != stored_size) {
        fprintf(stderr, "seal: INTEGRITY VIOLATION — size mismatch: %s\n", file_path);
        return -1;
    }

    if (!have_mtime || (long)st->st_mtime != stored_mtime) {
        fprintf(stderr, "seal: INTEGRITY VIOLATION — mtime mismatch: %s\n", file_path);
        return -1;
    }

    if (have_dev != have_ino ||
        (have_dev &&
         ((unsigned long long)st->st_dev != stored_dev ||
          (unsigned long long)st->st_ino != stored_ino))) {
        fprintf(stderr, "seal: INTEGRITY VIOLATION — file identity mismatch: %s\n", file_path);
        return -1;
    }

    return 0;
}

#ifdef __linux__
/* Best-effort chattr flag toggle without a shell: fork + execv with a fixed
 * absolute path (no PATH search) and an argv array; child stderr redirected
 * to /dev/null, all failures ignored. Preserves the historical
 * "chattr +/-i '<path>' 2>/dev/null" behavior without PATH lookup. */
static void seal_chattr_flag(const char *path, const char *flag) {
    pid_t pid = fork();
    if (pid == 0) {
        int devnull = open("/dev/null", O_WRONLY);
        if (devnull >= 0) {
            dup2(devnull, STDERR_FILENO);
            close(devnull);
        }
        char *const args[] = { "chattr", (char *)flag, (char *)path, NULL };
        execv("/usr/bin/chattr", args);
        _exit(127);
    }
    if (pid > 0) {
        int status;
        while (waitpid(pid, &status, 0) < 0 && errno == EINTR)
            ;
    }
}

static void seal_chattr_immutable(const char *path) {
    seal_chattr_flag(path, "+i");
}

static void seal_chattr_mutable(const char *path) {
    seal_chattr_flag(path, "-i");
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
    fprintf(f, "Dev: %llu\n", (unsigned long long)st.st_dev);
    fprintf(f, "Inode: %llu\n", (unsigned long long)st.st_ino);
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
    /* Verify current file against the marker's recorded identity and
     * fingerprint. Canonicalization itself still happens by pathname before
     * this point. */
    int rc = seal_verify_marker_against_stat(file_path, canon, dirfd, leaf, &st);
    close(fd);
    close(dirfd);
    return rc;
}

int lst_seal_amend(const char *file_path, const char *amendment) {
    if (!file_path || !amendment) return -1;

    char canon[LST_MAX_PATH];
    if (seal_canonicalize(file_path, canon, sizeof(canon)) != 0) {
        fprintf(stderr, "seal: file not found: %s\n", file_path);
        return -1;
    }

    char leaf[LST_MAX_NAME];
    int dirfd = seal_open_parent_dir(canon, leaf, sizeof(leaf));
    if (dirfd < 0) {
        fprintf(stderr, "seal: cannot amend: %s\n", file_path);
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
        fprintf(stderr, "seal: cannot amend: %s\n", file_path);
        return -1;
    }
    if (seal_verify_marker_against_stat(file_path, canon, dirfd, leaf, &st) != 0) {
        close(fd);
        close(dirfd);
        fprintf(stderr, "seal: cannot amend — verification failed: %s\n", file_path);
        return -1;
    }

    mode_t original_mode = st.st_mode & 07777;
    int afd = -1;
    FILE *f = NULL;
    int made_writable = 0;
    int restore_immutable = 0;

#ifdef __linux__
    unsigned int attr_flags = 0;
    if (ioctl(fd, FS_IOC_GETFLAGS, &attr_flags) == 0 &&
        (attr_flags & FS_IMMUTABLE_FL))
        restore_immutable = 1;
    seal_chattr_mutable(canon);
#endif

    if (fchmod(fd, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) != 0)
        goto amend_fail;
    made_writable = 1;

    /* Append amendment */
    afd = openat(dirfd, leaf, O_WRONLY | O_APPEND | O_NOFOLLOW | O_CLOEXEC);
    if (afd < 0) {
        goto amend_fail;
    }
    struct stat ast;
    if (fstat(afd, &ast) != 0 || !S_ISREG(ast.st_mode) ||
        ast.st_dev != st.st_dev || ast.st_ino != st.st_ino) {
        goto amend_fail;
    }
    f = fdopen(afd, "a");
    if (!f) {
        goto amend_fail;
    }
    afd = -1;

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

    if (fclose(f) != 0) {
        f = NULL;
        goto amend_fail;
    }
    f = NULL;
    close(fd);

    /* Make seal marker writable, then re-seal */
    char marker[LST_MAX_PATH];
    if (seal_marker_path(marker, sizeof(marker), leaf) == 0)
        seal_fchmod_best_effort_at(dirfd, marker,
                                   S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    close(dirfd);

    return lst_seal(file_path);

amend_fail:
    if (f)
        fclose(f);
    else if (afd >= 0)
        close(afd);
    if (made_writable)
        fchmod(fd, original_mode);
#ifdef __linux__
    if (restore_immutable)
        seal_chattr_immutable(canon);
#endif
    close(fd);
    close(dirfd);
    fprintf(stderr, "seal: cannot open for amendment: %s\n", file_path);
    return -1;
}
