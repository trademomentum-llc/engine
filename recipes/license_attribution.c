/*
 * license_attribution.c — License Attribution Recipe
 *
 * Operates on a stored LST artifact to generate a THIRD_PARTY_LICENSES
 * file. Pure transformation: reads artifact data, writes output file.
 * Never touches the original project source.
 */

#include "lst.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

/* License type to string */
static const char *license_name(uint8_t lic) {
    switch ((license_t)lic) {
        case LIC_MIT:         return "MIT";
        case LIC_BSD_2:       return "BSD-2-Clause";
        case LIC_BSD_3:       return "BSD-3-Clause";
        case LIC_APACHE_2:    return "Apache-2.0";
        case LIC_GPL_2:       return "GPL-2.0";
        case LIC_GPL_3:       return "GPL-3.0";
        case LIC_LGPL_2_1:    return "LGPL-2.1";
        case LIC_LGPL_3:      return "LGPL-3.0";
        case LIC_ISC:         return "ISC";
        case LIC_MPL_2:       return "MPL-2.0";
        case LIC_UNLICENSE:   return "Unlicense";
        case LIC_CC0:         return "CC0-1.0";
        case LIC_WTFPL:       return "WTFPL";
        case LIC_ARTISTIC_2:  return "Artistic-2.0";
        case LIC_ZLIB:        return "Zlib";
        case LIC_PROPRIETARY: return "Proprietary";
        case LIC_DUAL:        return "Multi-License";
        default:              return "UNKNOWN";
    }
}

/* Package manager to string */
static const char *manager_name(uint8_t mgr) {
    switch ((pkg_manager_t)mgr) {
        case PKG_COMPOSER:  return "Composer (PHP)";
        case PKG_NPM:       return "npm (Node.js)";
        case PKG_PIP:       return "pip (Python)";
        case PKG_CARGO:     return "Cargo (Rust)";
        case PKG_GO:        return "Go Modules";
        case PKG_RUBY:      return "Bundler (Ruby)";
        case PKG_MAVEN:     return "Maven (Java)";
        case PKG_GRADLE:    return "Gradle (Java)";
        case PKG_NUGET:     return "NuGet (.NET)";
        case PKG_SWIFT:     return "Swift PM";
        case PKG_COCOAPODS: return "CocoaPods (iOS)";
        default:            return "Unknown";
    }
}

static void write_sep(FILE *f) {
    for (int i = 0; i < 80; i++) fputc('=', f);
    fputc('\n', f);
}

static void write_line(FILE *f) {
    for (int i = 0; i < 40; i++) fputc('-', f);
    fputc('\n', f);
}

/* Compare deps by license then name — for qsort */
static int dep_compare(const void *a, const void *b) {
    const lst_dep_t *da = (const lst_dep_t *)a;
    const lst_dep_t *db = (const lst_dep_t *)b;
    if (da->license != db->license) return (int)da->license - (int)db->license;
    return strcasecmp(da->name, db->name);
}

/* --------------------------------------------------------------------------
 * Recipe entry point
 * -------------------------------------------------------------------------- */

static int recipe_license_attribution(lst_artifact_t *art, const char *output_dir) {
    if (!art || art->dep_count == 0) return 0;

    /* Build output path */
    char outpath[LST_MAX_PATH];
    if (output_dir)
        snprintf(outpath, sizeof(outpath), "%s/THIRD_PARTY_LICENSES", output_dir);
    else
        snprintf(outpath, sizeof(outpath), "%s/THIRD_PARTY_LICENSES", art->project_path);

    /* Ensure output directory exists */
    if (output_dir) mkdir(output_dir, 0755);

    FILE *f = fopen(outpath, "w");
    if (!f) {
        fprintf(stderr, "license-attribution: cannot write %s\n", outpath);
        return -1;
    }

    /* Sort deps for deterministic output */
    qsort(art->deps, art->dep_count, sizeof(lst_dep_t), dep_compare);

    /* Header */
    write_sep(f);
    fprintf(f, "THIRD-PARTY SOFTWARE LICENSES\n");
    fprintf(f, "Project: %s\n", art->project_name);
    fprintf(f, "Path: %s\n", art->project_path);

    time_t now = time(NULL);
    struct tm *t = gmtime(&now);
    char ts[64];
    strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S UTC", t);
    fprintf(f, "Generated: %s\n", ts);
    fprintf(f, "Total Dependencies: %u\n", art->dep_count);
    write_sep(f);
    fprintf(f, "\n");

    /* License summary */
    fprintf(f, "LICENSE SUMMARY\n");
    write_line(f);
    int counts[LIC_COUNT] = {0};
    for (uint32_t i = 0; i < art->dep_count; i++)
        counts[art->deps[i].license]++;
    for (int i = 0; i < LIC_COUNT; i++) {
        if (counts[i] > 0)
            fprintf(f, "  %-35s %4d\n", license_name((uint8_t)i), counts[i]);
    }
    fprintf(f, "\n");

    /* Per-manager sections */
    for (int mgr = 0; mgr < PKG_COUNT; mgr++) {
        if (!art->has_manager[mgr]) continue;

        /* Count deps for this manager */
        int mgr_count = 0;
        for (uint32_t i = 0; i < art->dep_count; i++)
            if (art->deps[i].pkg_manager == mgr) mgr_count++;
        if (mgr_count == 0) continue;

        write_sep(f);
        fprintf(f, "  %s — %d packages\n", manager_name((uint8_t)mgr), mgr_count);
        write_sep(f);
        fprintf(f, "\n");

        uint8_t current_license = 255;
        for (uint32_t i = 0; i < art->dep_count; i++) {
            const lst_dep_t *dep = &art->deps[i];
            if (dep->pkg_manager != mgr) continue;

            if (dep->license != current_license) {
                current_license = dep->license;
                const char *lname = (dep->license == LIC_DUAL && dep->license_str[0])
                    ? dep->license_str : license_name(dep->license);
                fprintf(f, "  [%s]\n\n", lname);
            }

            fprintf(f, "    %s\n", dep->name);

            /* Authors */
            if (dep->author_count > 0) {
                fprintf(f, "      Authors: ");
                for (int a = 0; a < dep->author_count; a++) {
                    if (a > 0) fprintf(f, ", ");
                    fprintf(f, "%s", dep->authors[a]);
                }
                fprintf(f, "\n");
            }

            if (dep->copyright[0])
                fprintf(f, "      %s\n", dep->copyright);

            fprintf(f, "\n");
        }
    }

    /* Footer */
    write_sep(f);
    fprintf(f, "END OF THIRD-PARTY LICENSES\n");
    fprintf(f, "\nThis file catalogs the intellectual property of every third-party\n");
    fprintf(f, "author whose software is distributed with this project. Once sealed\n");
    fprintf(f, "for production, this file is immutable and may only be amended.\n");
    write_sep(f);
    fprintf(f, "\n");

    fclose(f);

    printf("  Wrote %s (%u dependencies)\n", outpath, art->dep_count);
    return 0;
}

/* --------------------------------------------------------------------------
 * Recipe registration — called from main
 * -------------------------------------------------------------------------- */

void recipe_license_attribution_register(void) {
    lst_recipe_t r = {0};
    snprintf(r.name, LST_MAX_NAME, "license-attribution");
    snprintf(r.description, LST_MAX_NAME, "Generate THIRD_PARTY_LICENSES from dependency data");
    r.execute = recipe_license_attribution;
    r.version = 1;
    lst_recipe_register(&r);
}
