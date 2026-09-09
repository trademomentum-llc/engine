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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>

/* Artifact file extension */
#define LST_EXT ".lst"

/* Build the store path for a given project name.
 * Returns 0 on success, -1 if project_name is unsafe, -2 if store_dir
 * cannot be canonicalized or the result would not fit (no silent
 * truncation). */
static int store_path(char *dst, size_t maxlen, const char *store_dir, const char *project_name) {
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

    /* Canonicalize store_dir: realpath() resolves ".." and follows symlinks
     * at every level of the directory tree, so canon_dir is the store's real
     * location — which can be outside the path spelling the caller passed.
     * dst is composed as <canon_dir>/<safe_name>.lst where safe_name is a
     * single validated component (no separators, no ".."), so the composed
     * path resolves inside canon_dir. Confinement holds relative to the
     * canonical store directory only; open() adds O_NOFOLLOW to reject a
     * symlink at the final resolved component. */
    char *canon_dir = realpath(store_dir, NULL);
    if (!canon_dir) return -2;
    int n = snprintf(dst, maxlen, "%s/%s%s", canon_dir, safe_name, LST_EXT);
    free(canon_dir);
    if (n < 0 || (size_t)n >= maxlen) return -2;
    return 0;
}

int lst_store_write(const lst_artifact_t *art, const char *store_dir) {
    if (!art || !store_dir) return -1;

    /* Ensure store directory exists */
    mkdir(store_dir, 0755);

    char path[LST_MAX_PATH];
    int prc = store_path(path, sizeof(path), store_dir, art->project_name);
    if (prc == -1) {
        fprintf(stderr, "store: unsafe project name: %s\n", art->project_name);
        return -1;
    }
    if (prc != 0) {
        fprintf(stderr, "store: cannot build store path for project: %s\n", art->project_name);
        return -1;
    }

    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC | O_NOFOLLOW | O_CLOEXEC, 0644);
    if (fd < 0) {
        fprintf(stderr, "store: cannot write %s\n", path);
        return -1;
    }
    FILE *f = fdopen(fd, "wb");
    if (!f) {
        close(fd);
        fprintf(stderr, "store: cannot write %s\n", path);
        return -1;
    }

    size_t written = fwrite(art, sizeof(lst_artifact_t), 1, f);
    fclose(f);

    if (written != 1) {
        fprintf(stderr, "store: incomplete write to %s\n", path);
        return -1;
    }

    return 0;
}

lst_artifact_t *lst_store_read(const char *store_dir, const char *project_name) {
    if (!store_dir || !project_name) return NULL;

    char path[LST_MAX_PATH];
    if (store_path(path, sizeof(path), store_dir, project_name) != 0)
        return NULL;

    int fd = open(path, O_RDONLY | O_NOFOLLOW | O_CLOEXEC);
    if (fd < 0) return NULL;
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
    char path[LST_MAX_PATH];
    if (store_path(path, sizeof(path), store_dir, project_name) != 0)
        return 0;

    struct stat st;
    if (stat(path, &st) != 0) return 0; /* doesn't exist */

    /* Current if stored within last hour — recipes can force rebuild */
    time_t now = time(NULL);
    return (now - st.st_mtime) < 3600;
}

int lst_store_list(const char *store_dir, char names[][LST_MAX_NAME], int max) {
    DIR *d = opendir(store_dir);
    if (!d) return 0;

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
