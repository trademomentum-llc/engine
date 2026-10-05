/*
 * store.c — LST Artifact Repository
 *
 * Serialize/deserialize artifacts to disk as raw binary files.
 * No format negotiation, no versioning overhead — the struct IS the format.
 * Store once, read by any recipe, any number of times.
 */

#define _XOPEN_SOURCE 700
#define _POSIX_C_SOURCE 200809L

#include "lst.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>

/* Artifact file extension */
#define LST_EXT ".lst"

static int open_store_dir_fd(const char *store_dir, int create_if_missing, int require_private) {
    if (create_if_missing &&
        mkdir(store_dir, 0700) != 0 && errno != EEXIST) {
        fprintf(stderr, "store: cannot create store directory: %s\n", store_dir);
        return -1;
    }

    int dirfd = open(store_dir, O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
    if (dirfd < 0) {
        fprintf(stderr, "store: cannot access store directory: %s\n", store_dir);
        return -1;
    }

    struct stat st;
    if (fstat(dirfd, &st) != 0 || !S_ISDIR(st.st_mode)) {
        fprintf(stderr, "store: cannot access store directory: %s\n", store_dir);
        close(dirfd);
        return -1;
    }

    if (require_private && (st.st_mode & 07777) != 0700) {
        if (fchmod(dirfd, 0700) != 0) {
            fprintf(stderr, "store: store directory must be private (0700): %s\n", store_dir);
            close(dirfd);
            return -1;
        }
    }

    return dirfd;
}

/* Build the store leaf name for a given project name.
 * Returns 0 on success, -1 if project_name is unsafe, -3 if the composed
 * filename would not fit (no silent truncation). */
static int store_path(char *dst, size_t maxlen, const char *project_name) {
    /* Reject traversal and hostile names outright */
    if (!project_name || !project_name[0]) return -1;
    if (project_name[0] == '.') return -1;              /* ".", "..", hidden   */
    if (strstr(project_name, "..")) return -1;          /* parent traversal    */
    if (strchr(project_name, '\\')) return -1;          /* alternate separator */
    if (strlen(project_name) >= LST_MAX_NAME) return -1;
    for (const unsigned char *p = (const unsigned char *)project_name; *p; p++)
        if (*p < 0x20 || *p == 0x7f) return -1;         /* control chars       */

    /* Sanitize project name: replace / with _ (unchanged for legitimate names) */
    char safe_name[LST_MAX_NAME];
    size_t i;
    for (i = 0; project_name[i]; i++) {
        safe_name[i] = (project_name[i] == '/') ? '_' : project_name[i];
    }
    safe_name[i] = '\0';

    int n = snprintf(dst, maxlen, "%s%s", safe_name, LST_EXT);
    if (n < 0 || (size_t)n >= maxlen) return -3;
    return 0;
}

int lst_store_write(const lst_artifact_t *art, const char *store_dir) {
    if (!art || !store_dir) return -1;

    /* Ensure store directory exists and is private even if it predates the
     * 0700 hardening. */
    int dirfd = open_store_dir_fd(store_dir, 1, 1);
    if (dirfd < 0)
        return -1;

    char path[LST_MAX_PATH];
    int prc = store_path(path, sizeof(path), art->project_name);
    if (prc == -1) {
        fprintf(stderr, "store: unsafe project name: %s\n", art->project_name);
        close(dirfd);
        return -1;
    }
    if (prc != 0) {
        fprintf(stderr, "store: composed path too long under store directory: %s\n", store_dir);
        close(dirfd);
        return -1;
    }

    /* O_NONBLOCK: an existing leaf replaced by a FIFO fails with ENXIO
     * instead of blocking for a reader; anything else non-regular is
     * rejected by the fstat check before it is wrapped in a FILE *. */
    int fd = openat(dirfd, path,
                    O_WRONLY | O_CREAT | O_TRUNC | O_NOFOLLOW | O_NONBLOCK | O_CLOEXEC, 0600);
    close(dirfd);
    if (fd < 0) {
        fprintf(stderr, "store: cannot write %s/%s\n", store_dir, path);
        return -1;
    }
    struct stat leaf_st;
    int fl = -1;
    if (fstat(fd, &leaf_st) != 0 || !S_ISREG(leaf_st.st_mode) ||
        (fl = fcntl(fd, F_GETFL)) < 0 || fcntl(fd, F_SETFL, fl & ~O_NONBLOCK) != 0) {
        close(fd);
        fprintf(stderr, "store: refusing non-regular store file %s/%s\n", store_dir, path);
        return -1;
    }
    FILE *f = fdopen(fd, "wb");
    if (!f) {
        close(fd);
        fprintf(stderr, "store: cannot write %s/%s\n", store_dir, path);
        return -1;
    }

    size_t written = fwrite(art, sizeof(lst_artifact_t), 1, f);
    int close_rc = fclose(f);

    if (written != 1 || close_rc != 0) {
        fprintf(stderr, "store: incomplete write to %s/%s\n", store_dir, path);
        return -1;
    }

    return 0;
}

lst_artifact_t *lst_store_read(const char *store_dir, const char *project_name) {
    if (!store_dir || !project_name) return NULL;

    int dirfd = open_store_dir_fd(store_dir, 0, 0);
    if (dirfd < 0)
        return NULL;

    char path[LST_MAX_PATH];
    if (store_path(path, sizeof(path), project_name) != 0) {
        close(dirfd);
        return NULL;
    }

    int fd = openat(dirfd, path, O_RDONLY | O_NONBLOCK | O_NOFOLLOW | O_CLOEXEC);
    close(dirfd);
    if (fd < 0) return NULL;
    struct stat st;
    if (fstat(fd, &st) != 0 || !S_ISREG(st.st_mode) ||
        st.st_size != (off_t)sizeof(lst_artifact_t)) {
        close(fd);
        return NULL;
    }
    FILE *f = fdopen(fd, "rb");
    if (!f) { close(fd); return NULL; }

    lst_artifact_t *art = calloc(1, sizeof(lst_artifact_t));
    if (!art) { fclose(f); return NULL; }

    size_t rd = fread(art, sizeof(lst_artifact_t), 1, f);
    fclose(f);

    if (rd != 1 || art->magic != 0x4C535400) {
        free(art);
        return NULL;
    }

    return art;
}

int lst_store_is_current(const char *store_dir, const char *project_name) {
    int dirfd = open_store_dir_fd(store_dir, 0, 0);
    if (dirfd < 0)
        return 0;

    char path[LST_MAX_PATH];
    if (store_path(path, sizeof(path), project_name) != 0) {
        close(dirfd);
        return 0;
    }

    struct stat st;
    if (fstatat(dirfd, path, &st, AT_SYMLINK_NOFOLLOW) != 0 || !S_ISREG(st.st_mode)) {
        close(dirfd);
        return 0; /* doesn't exist or is not a regular file */
    }
    close(dirfd);

    /* Current if stored within last hour — recipes can force rebuild */
    time_t now = time(NULL);
    return (now - st.st_mtime) < 3600;
}

int lst_store_list(const char *store_dir, char names[][LST_MAX_NAME], int max) {
    int dirfd = open_store_dir_fd(store_dir, 0, 0);
    if (dirfd < 0) return 0;

    DIR *d = fdopendir(dirfd);
    if (!d) {
        close(dirfd);
        return 0;
    }

    int count = 0;
    struct dirent *ent;
    while ((ent = readdir(d)) != NULL && count < max) {
        size_t len = strlen(ent->d_name);
        size_t ext_len = strlen(LST_EXT);
        if (len > ext_len && strcmp(ent->d_name + len - ext_len, LST_EXT) == 0) {
            /* Strip extension for name */
            size_t name_len = len - ext_len;
            if (name_len >= LST_MAX_NAME) name_len = LST_MAX_NAME - 1;
            memcpy(names[count], ent->d_name, name_len);
            names[count][name_len] = '\0';
            count++;
        }
    }
    closedir(d);
    return count;
}
