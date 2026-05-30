/*
 * store.c — LST Artifact Repository
 *
 * Serialize/deserialize artifacts to disk as raw binary files.
 * No format negotiation, no versioning overhead — the struct IS the format.
 * Store once, read by any recipe, any number of times.
 */

#include "lst.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>

/* Artifact file extension */
#define LST_EXT ".lst"

/* Build the store path for a given project name */
static void store_path(char *dst, size_t maxlen, const char *store_dir, const char *project_name) {
    /* Sanitize project name: replace / with _ */
    char safe_name[LST_MAX_NAME];
    size_t i;
    for (i = 0; project_name[i] && i < LST_MAX_NAME - 1; i++) {
        safe_name[i] = (project_name[i] == '/') ? '_' : project_name[i];
    }
    safe_name[i] = '\0';
    snprintf(dst, maxlen, "%s/%s%s", store_dir, safe_name, LST_EXT);
}

int lst_store_write(const lst_artifact_t *art, const char *store_dir) {
    if (!art || !store_dir) return -1;

    /* Ensure store directory exists */
    mkdir(store_dir, 0755);

    char path[LST_MAX_PATH];
    store_path(path, sizeof(path), store_dir, art->project_name);

    FILE *f = fopen(path, "wb");
    if (!f) {
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
    store_path(path, sizeof(path), store_dir, project_name);

    FILE *f = fopen(path, "rb");
    if (!f) return NULL;

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
    store_path(path, sizeof(path), store_dir, project_name);

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
