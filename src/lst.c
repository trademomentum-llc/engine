/*
 * lst.c — LST Builder and Core Operations
 *
 * Builds Lossless Semantic Tree artifacts from project directories.
 * All scanning is deterministic: read files, parse known formats,
 * populate fixed-layout structs. No interpretation, no GC, no boxing.
 */

#include "lst.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>

/* --------------------------------------------------------------------------
 * Internal helpers
 * -------------------------------------------------------------------------- */

FILE *lst_secure_fopen(const char *path, const char *mode) {
    if (!path || !mode) return NULL;
    const char *component = path;
    while ((component = strstr(component, "..")) != NULL) {
        if ((component == path || component[-1] == '/') &&
            (component[2] == '\0' || component[2] == '/'))
            return NULL;
        component += 2;
    }
    int flags;
    if (mode[0] == 'r') flags = O_RDONLY;
    else if (mode[0] == 'a') flags = O_WRONLY | O_CREAT | O_APPEND;
    else if (mode[0] == 'w') flags = O_WRONLY | O_CREAT | O_TRUNC;
    else return NULL;
    int fd = open(path, flags, S_IRUSR | S_IWUSR);
    if (fd < 0) return NULL;
    FILE *f = fdopen(fd, mode);
    if (!f) close(fd);
    return f;
}

/* Safe string copy into fixed buffer */
static void scopy(char *dst, const char *src, size_t maxlen) {
    if (!src) { dst[0] = '\0'; return; }
    size_t len = strlen(src);
    if (len >= maxlen) len = maxlen - 1;
    memcpy(dst, src, len);
    dst[len] = '\0';
}

/* Trim leading/trailing whitespace in place */
static void strim(char *s) {
    char *start = s;
    while (*start && isspace((unsigned char)*start)) start++;
    if (start != s) memmove(s, start, strlen(start) + 1);
    size_t len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1])) s[--len] = '\0';
}

/* Read entire file into malloc'd buffer. Caller frees. Returns NULL on fail. */
static char *read_file(const char *path, size_t *out_len) {
    FILE *f = lst_secure_fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    if (sz < 0 || sz > 50 * 1024 * 1024) { fclose(f); return NULL; } /* 50MB cap */
    fseek(f, 0, SEEK_SET);
    char *buf = malloc((size_t)sz + 1);
    if (!buf) { fclose(f); return NULL; }
    size_t rd = fread(buf, 1, (size_t)sz, f);
    fclose(f);
    buf[rd] = '\0';
    if (out_len) *out_len = rd;
    return buf;
}

/* Check if file exists */
static int file_exists(const char *path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

/* Check if directory exists */
static int dir_exists(const char *path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

/* Build a path: base/a/b into dst */
static void path_join(char *dst, size_t maxlen, const char *base, const char *a, const char *b) {
    if (b)
        snprintf(dst, maxlen, "%s/%s/%s", base, a, b);
    else
        snprintf(dst, maxlen, "%s/%s", base, a);
}

/* --------------------------------------------------------------------------
 * Minimal JSON value extractor
 *
 * Not a full parser — extracts string values for known keys from
 * simple JSON objects. Handles the flat structure of composer.json
 * and package.json. Deterministic, no allocations beyond the input buffer.
 * -------------------------------------------------------------------------- */

/* Find value for "key" in JSON string. Writes into dst. Returns 1 if found. */
static int json_get_string(const char *json, const char *key, char *dst, size_t maxlen) {
    dst[0] = '\0';
    char pattern[LST_MAX_NAME + 8];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);

    const char *p = strstr(json, pattern);
    if (!p) return 0;
    p += strlen(pattern);

    /* Skip whitespace and colon */
    while (*p && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r' || *p == ':')) p++;
    if (*p != '"') return 0;
    p++; /* skip opening quote */

    size_t i = 0;
    while (*p && *p != '"' && i < maxlen - 1) {
        if (*p == '\\' && *(p + 1)) { p++; } /* skip escape */
        dst[i++] = *p++;
    }
    dst[i] = '\0';
    return 1;
}

/* Extract license string — handles both "license": "MIT" and "license": ["MIT", "BSD"] */
static int json_get_license(const char *json, char *dst, size_t maxlen) {
    dst[0] = '\0';
    const char *p = strstr(json, "\"license\"");
    if (!p) return 0;
    p += 9; /* strlen("\"license\"") */
    while (*p && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r' || *p == ':')) p++;

    if (*p == '"') {
        /* Simple string */
        p++;
        size_t i = 0;
        while (*p && *p != '"' && i < maxlen - 1) dst[i++] = *p++;
        dst[i] = '\0';
        return 1;
    }
    if (*p == '[') {
        /* Array — join with " OR " */
        p++;
        size_t off = 0;
        int first = 1;
        while (*p && *p != ']') {
            if (*p == '"') {
                if (!first && off + 4 < maxlen) {
                    memcpy(dst + off, " OR ", 4); off += 4;
                }
                first = 0;
                p++;
                while (*p && *p != '"' && off < maxlen - 1) dst[off++] = *p++;
                if (*p == '"') p++;
            } else {
                p++;
            }
        }
        dst[off] = '\0';
        return 1;
    }
    return 0;
}

/* Map license string to license_t enum */
static license_t license_classify(const char *s) {
    if (!s || !*s) return LIC_UNKNOWN;

    /* Uppercase comparison buffer */
    char upper[LST_MAX_LICENSE];
    size_t i;
    for (i = 0; s[i] && i < sizeof(upper) - 1; i++)
        upper[i] = (char)toupper((unsigned char)s[i]);
    upper[i] = '\0';

    if (strstr(upper, "MIT"))                          return LIC_MIT;
    if (strstr(upper, "APACHE") && strstr(upper, "2")) return LIC_APACHE_2;
    if (strstr(upper, "LGPL") && strstr(upper, "3"))   return LIC_LGPL_3;
    if (strstr(upper, "LGPL") && strstr(upper, "2"))   return LIC_LGPL_2_1;
    if (strstr(upper, "GPL") && strstr(upper, "3"))    return LIC_GPL_3;
    if (strstr(upper, "GPL") && strstr(upper, "2"))    return LIC_GPL_2;
    if (strstr(upper, "BSD-3") || strstr(upper, "BSD 3")) return LIC_BSD_3;
    if (strstr(upper, "BSD-2") || strstr(upper, "BSD 2")) return LIC_BSD_2;
    if (strstr(upper, "BSD"))                          return LIC_BSD_3; /* default BSD */
    if (strstr(upper, "ISC"))                          return LIC_ISC;
    if (strstr(upper, "MPL"))                          return LIC_MPL_2;
    if (strstr(upper, "UNLICENSE"))                    return LIC_UNLICENSE;
    if (strstr(upper, "CC0"))                          return LIC_CC0;
    if (strstr(upper, "WTFPL"))                        return LIC_WTFPL;
    if (strstr(upper, "ARTISTIC"))                     return LIC_ARTISTIC_2;
    if (strstr(upper, "ZLIB"))                         return LIC_ZLIB;
    if (strstr(upper, "OR"))                           return LIC_DUAL;
    if (strstr(upper, "PROPRIETARY"))                  return LIC_PROPRIETARY;

    return LIC_UNKNOWN;
}

/* Extract authors from JSON "authors" array (composer.json style) */
static int json_get_authors_composer(const char *json, lst_dep_t *dep) {
    const char *p = strstr(json, "\"authors\"");
    if (!p) return 0;
    p = strchr(p, '[');
    if (!p) return 0;
    p++;

    dep->author_count = 0;
    while (*p && *p != ']' && dep->author_count < LST_MAX_AUTHORS) {
        const char *name_key = strstr(p, "\"name\"");
        if (!name_key || name_key > strchr(p, ']')) break;
        name_key += 6;
        while (*name_key && (*name_key == ' ' || *name_key == ':' || *name_key == '\t')) name_key++;
        if (*name_key == '"') {
            name_key++;
            size_t i = 0;
            while (*name_key && *name_key != '"' && i < LST_MAX_AUTHOR_NAME - 1)
                dep->authors[dep->author_count][i++] = *name_key++;
            dep->authors[dep->author_count][i] = '\0';
            dep->author_count++;
        }
        /* Advance past this object */
        const char *next = strchr(name_key, '}');
        if (!next) break;
        p = next + 1;
    }
    return dep->author_count;
}

/* Extract author from package.json "author" field (string or object) */
static int json_get_author_npm(const char *json, lst_dep_t *dep) {
    char author_str[LST_MAX_AUTHOR_NAME];
    dep->author_count = 0;

    /* Try "author": "Name <email>" */
    if (json_get_string(json, "author", author_str, sizeof(author_str))) {
        /* Strip email/url parts */
        char *lt = strchr(author_str, '<');
        if (lt) *lt = '\0';
        char *paren = strchr(author_str, '(');
        if (paren) *paren = '\0';
        strim(author_str);
        if (author_str[0]) {
            scopy(dep->authors[0], author_str, LST_MAX_AUTHOR_NAME);
            dep->author_count = 1;
        }
    }

    /* Also check "author": { "name": "..." } */
    if (dep->author_count == 0) {
        const char *p = strstr(json, "\"author\"");
        if (p) {
            p += 8;
            while (*p && (*p == ' ' || *p == ':' || *p == '\t' || *p == '\n')) p++;
            if (*p == '{') {
                char name[LST_MAX_AUTHOR_NAME];
                if (json_get_string(p, "name", name, sizeof(name))) {
                    scopy(dep->authors[0], name, LST_MAX_AUTHOR_NAME);
                    dep->author_count = 1;
                }
            }
        }
    }

    return dep->author_count;
}

/* Find and extract copyright line from LICENSE file in a directory */
static void find_copyright(const char *dir, char *dst, size_t maxlen) {
    dst[0] = '\0';
    const char *names[] = {
        "LICENSE", "LICENSE.md", "LICENSE.txt", "LICENCE", "license.md",
        "COPYING", "LICENSE-MIT", "LICENSE.LGPL", NULL
    };
    char path[LST_MAX_PATH];
    for (int i = 0; names[i]; i++) {
        path_join(path, sizeof(path), dir, names[i], NULL);
        size_t len = 0;
        char *buf = read_file(path, &len);
        if (!buf) continue;

        /* Scan for "Copyright" line */
        char *line = buf;
        while (*line) {
            char *eol = strchr(line, '\n');
            if (eol) *eol = '\0';
            /* Case-insensitive startswith "copyright" */
            if (strncasecmp(line, "copyright", 9) == 0) {
                scopy(dst, line, maxlen);
                free(buf);
                return;
            }
            if (!eol) break;
            line = eol + 1;
        }
        free(buf);
    }
}

/* --------------------------------------------------------------------------
 * LST Create / Destroy
 * -------------------------------------------------------------------------- */

lst_artifact_t *lst_create(const char *project_path) {
    lst_artifact_t *art = calloc(1, sizeof(lst_artifact_t));
    if (!art) return NULL;

    art->magic = 0x4C535400; /* "LST\0" */
    art->version = 1;
    art->built_at = time(NULL);

    scopy(art->project_path, project_path, LST_MAX_PATH);

    /* Extract project name from path */
    const char *name = strrchr(project_path, '/');
    scopy(art->project_name, name ? name + 1 : project_path, LST_MAX_NAME);

    return art;
}

void lst_destroy(lst_artifact_t *art) {
    free(art);
}

/* --------------------------------------------------------------------------
 * Dependency Scanners
 * -------------------------------------------------------------------------- */

int lst_scan_deps_composer(lst_artifact_t *art) {
    char vendor_dir[LST_MAX_PATH];
    path_join(vendor_dir, sizeof(vendor_dir), art->project_path, "vendor", NULL);
    if (!dir_exists(vendor_dir)) return 0;

    art->has_manager[PKG_COMPOSER] = 1;
    DIR *orgs = opendir(vendor_dir);
    if (!orgs) return -1;

    struct dirent *org_ent;
    while ((org_ent = readdir(orgs)) != NULL) {
        if (org_ent->d_name[0] == '.') continue;
        if (strcmp(org_ent->d_name, "bin") == 0 || strcmp(org_ent->d_name, "composer") == 0) continue;

        char org_path[LST_MAX_PATH];
        path_join(org_path, sizeof(org_path), vendor_dir, org_ent->d_name, NULL);
        if (!dir_exists(org_path)) continue;

        DIR *pkgs = opendir(org_path);
        if (!pkgs) continue;

        struct dirent *pkg_ent;
        while ((pkg_ent = readdir(pkgs)) != NULL) {
            if (pkg_ent->d_name[0] == '.') continue;
            if (art->dep_count >= LST_MAX_DEPS) break;

            char cj_path[LST_MAX_PATH];
            path_join(cj_path, sizeof(cj_path), org_path, pkg_ent->d_name, "composer.json");
            if (!file_exists(cj_path)) continue;

            size_t len = 0;
            char *json = read_file(cj_path, &len);
            if (!json) continue;

            lst_dep_t *dep = &art->deps[art->dep_count];
            memset(dep, 0, sizeof(lst_dep_t));
            dep->pkg_manager = PKG_COMPOSER;

            json_get_string(json, "name", dep->name, LST_MAX_NAME);

            char lic_str[LST_MAX_LICENSE];
            if (json_get_license(json, lic_str, LST_MAX_LICENSE)) {
                dep->license = license_classify(lic_str);
                scopy(dep->license_str, lic_str, LST_MAX_LICENSE);
            }

            json_get_authors_composer(json, dep);
            scopy(dep->source_path, cj_path, LST_MAX_PATH);

            /* Copyright from LICENSE file */
            char pkg_dir[LST_MAX_PATH];
            path_join(pkg_dir, sizeof(pkg_dir), org_path, pkg_ent->d_name, NULL);
            find_copyright(pkg_dir, dep->copyright, LST_MAX_NAME);

            art->dep_count++;
            free(json);
        }
        closedir(pkgs);
    }
    closedir(orgs);
    return (int)art->dep_count;
}

int lst_scan_deps_npm(lst_artifact_t *art) {
    char nm_dir[LST_MAX_PATH];
    path_join(nm_dir, sizeof(nm_dir), art->project_path, "node_modules", NULL);
    if (!dir_exists(nm_dir)) return 0;

    art->has_manager[PKG_NPM] = 1;
    DIR *d = opendir(nm_dir);
    if (!d) return -1;

    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        if (ent->d_name[0] == '.') continue;
        if (art->dep_count >= LST_MAX_DEPS) break;

        char pkg_path[LST_MAX_PATH];

        if (ent->d_name[0] == '@') {
            /* Scoped package: @scope/name */
            char scope_path[LST_MAX_PATH];
            path_join(scope_path, sizeof(scope_path), nm_dir, ent->d_name, NULL);
            DIR *scope = opendir(scope_path);
            if (!scope) continue;

            struct dirent *sub;
            while ((sub = readdir(scope)) != NULL) {
                if (sub->d_name[0] == '.') continue;
                if (art->dep_count >= LST_MAX_DEPS) break;

                char pj_path[LST_MAX_PATH];
                path_join(pj_path, sizeof(pj_path), scope_path, sub->d_name, "package.json");
                if (!file_exists(pj_path)) continue;

                size_t len = 0;
                char *json = read_file(pj_path, &len);
                if (!json) continue;

                lst_dep_t *dep = &art->deps[art->dep_count];
                memset(dep, 0, sizeof(lst_dep_t));
                dep->pkg_manager = PKG_NPM;

                json_get_string(json, "name", dep->name, LST_MAX_NAME);

                char lic_str[LST_MAX_LICENSE];
                if (json_get_license(json, lic_str, LST_MAX_LICENSE)) {
                    dep->license = license_classify(lic_str);
                    scopy(dep->license_str, lic_str, LST_MAX_LICENSE);
                }

                json_get_author_npm(json, dep);
                scopy(dep->source_path, pj_path, LST_MAX_PATH);

                char sub_dir[LST_MAX_PATH];
                path_join(sub_dir, sizeof(sub_dir), scope_path, sub->d_name, NULL);
                find_copyright(sub_dir, dep->copyright, LST_MAX_NAME);

                art->dep_count++;
                free(json);
            }
            closedir(scope);
        } else {
            path_join(pkg_path, sizeof(pkg_path), nm_dir, ent->d_name, "package.json");
            if (!file_exists(pkg_path)) continue;

            size_t len = 0;
            char *json = read_file(pkg_path, &len);
            if (!json) continue;

            lst_dep_t *dep = &art->deps[art->dep_count];
            memset(dep, 0, sizeof(lst_dep_t));
            dep->pkg_manager = PKG_NPM;

            json_get_string(json, "name", dep->name, LST_MAX_NAME);

            char lic_str[LST_MAX_LICENSE];
            if (json_get_license(json, lic_str, LST_MAX_LICENSE)) {
                dep->license = license_classify(lic_str);
                scopy(dep->license_str, lic_str, LST_MAX_LICENSE);
            }

            json_get_author_npm(json, dep);
            scopy(dep->source_path, pkg_path, LST_MAX_PATH);

            char ent_dir[LST_MAX_PATH];
            path_join(ent_dir, sizeof(ent_dir), nm_dir, ent->d_name, NULL);
            find_copyright(ent_dir, dep->copyright, LST_MAX_NAME);

            art->dep_count++;
            free(json);
        }
    }
    closedir(d);
    return (int)art->dep_count;
}

/* Stub scanners — same pattern, extend as needed */
int lst_scan_deps_pip(lst_artifact_t *art)   { (void)art; return 0; }
int lst_scan_deps_cargo(lst_artifact_t *art) { (void)art; return 0; }
int lst_scan_deps_go(lst_artifact_t *art)    { (void)art; return 0; }
int lst_scan_files(lst_artifact_t *art)      { (void)art; return 0; }

/* --------------------------------------------------------------------------
 * LST Build — run all scanners
 * -------------------------------------------------------------------------- */

int lst_build(lst_artifact_t *art) {
    if (!art) return -1;

    lst_scan_deps_composer(art);
    lst_scan_deps_npm(art);
    lst_scan_deps_pip(art);
    lst_scan_deps_cargo(art);
    lst_scan_deps_go(art);
    lst_scan_files(art);

    /* Set health based on what we found */
    art->health = (art->dep_count > 0) ? HEALTH_HEALTHY : HEALTH_UNKNOWN;

    return 0;
}
