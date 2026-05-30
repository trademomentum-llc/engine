/*
 * main.c — Engine CLI
 *
 * Usage:
 *   engine build <project_path>              Build LST artifact
 *   engine run <recipe> <project_path>       Build + run recipe
 *   engine batch <root_dir> <recipe>         Build + run across all projects
 *   engine list                              List registered recipes
 *   engine store list                        List stored artifacts
 *   engine seal <file_path>                  Seal a file for production
 *   engine verify <file_path>                Verify seal integrity
 *   engine amend <file_path> <text>          Amend a sealed file
 */

#include "lst.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

/* Default store location */
#define DEFAULT_STORE "/Users/nexus1/Projects/engine/store"

/* Recipe registration functions — add new ones here */
extern void recipe_license_attribution_register(void);
extern void recipe_security_scan_register(void);
extern void recipe_code_check_register(void);
extern void recipe_pqc_validation_register(void);
extern void recipe_lsa_compliance_register(void);
extern void recipe_neurodiv_safety_register(void);
extern void recipe_morphogenetic_healing_register(void);
extern void recipe_agentxfoundry_threat_intel_register(void);
extern void recipe_tmg_compliance_register(void);
extern void recipe_agent_lifecycle_register(void);
extern void recipe_risk_assessment_register(void);
extern void recipe_traceability_audit_register(void);
extern void recipe_compliance_guardrails_register(void);
extern void recipe_vectordb_provenance_register(void);

static void register_all_recipes(void) {
    recipe_license_attribution_register();
    recipe_security_scan_register();
    recipe_code_check_register();
    recipe_pqc_validation_register();
    recipe_lsa_compliance_register();
    recipe_neurodiv_safety_register();
    recipe_morphogenetic_healing_register();
    recipe_agentxfoundry_threat_intel_register();
    recipe_tmg_compliance_register();
    recipe_agent_lifecycle_register();
    recipe_risk_assessment_register();
    recipe_traceability_audit_register();
    recipe_compliance_guardrails_register();
    recipe_vectordb_provenance_register();
}

/* --------------------------------------------------------------------------
 * Skip directories during batch traversal
 * -------------------------------------------------------------------------- */

static const char *SKIP_DIRS[] = {
    "node_modules", "vendor", ".git", ".next", ".nuxt", "__pycache__",
    "dist", "build", ".venv", "venv", "env", ".tox", "target",
    ".cache", ".npm", ".yarn", "coverage", ".terraform", "engine",
    NULL
};

static int should_skip(const char *name) {
    if (name[0] == '.') return 1;
    for (int i = 0; SKIP_DIRS[i]; i++)
        if (strcmp(name, SKIP_DIRS[i]) == 0) return 1;
    return 0;
}

/* Check if a directory has third-party deps */
static int has_deps(const char *path) {
    char check[LST_MAX_PATH];

    snprintf(check, sizeof(check), "%s/vendor/autoload.php", path);
    struct stat st;
    if (stat(check, &st) == 0) return 1;

    snprintf(check, sizeof(check), "%s/node_modules", path);
    if (stat(check, &st) == 0 && S_ISDIR(st.st_mode)) return 1;

    snprintf(check, sizeof(check), "%s/Cargo.lock", path);
    if (stat(check, &st) == 0) return 1;

    snprintf(check, sizeof(check), "%s/go.mod", path);
    if (stat(check, &st) == 0) return 1;

    snprintf(check, sizeof(check), "%s/Gemfile.lock", path);
    if (stat(check, &st) == 0) return 1;

    return 0;
}

/* --------------------------------------------------------------------------
 * Commands
 * -------------------------------------------------------------------------- */

static int cmd_build(const char *project_path) {
    printf("Building LST: %s\n", project_path);

    lst_artifact_t *art = lst_create(project_path);
    if (!art) {
        fprintf(stderr, "Failed to create artifact\n");
        return 1;
    }

    lst_build(art);
    printf("  Dependencies: %u\n", art->dep_count);
    printf("  Files: %u\n", art->file_count);
    printf("  Issues: %u\n", art->issue_count);

    int rc = lst_store_write(art, DEFAULT_STORE);
    if (rc == 0)
        printf("  Stored: %s/%s.lst\n", DEFAULT_STORE, art->project_name);
    else
        fprintf(stderr, "  Failed to store artifact\n");

    lst_destroy(art);
    return rc;
}

static int cmd_run(const char *recipe_name, const char *project_path) {
    printf("Building LST: %s\n", project_path);

    lst_artifact_t *art = lst_create(project_path);
    if (!art) return 1;

    lst_build(art);
    lst_store_write(art, DEFAULT_STORE);

    printf("Running recipe: %s\n", recipe_name);
    const lst_recipe_t *recipe = lst_recipe_find(recipe_name);
    if (!recipe) {
        fprintf(stderr, "Recipe not found: %s\n", recipe_name);
        lst_destroy(art);
        return 1;
    }

    int rc = recipe->execute(art, NULL);
    lst_destroy(art);
    return rc;
}

static void batch_walk(const char *dir, const char *recipe_name, int depth,
                       int *found, int *success, int *failed) {
    if (depth > 6) return;

    if (has_deps(dir)) {
        (*found)++;
        printf("\n>>> %s\n", dir);

        lst_artifact_t *art = lst_create(dir);
        if (art) {
            lst_build(art);
            lst_store_write(art, DEFAULT_STORE);

            const lst_recipe_t *recipe = lst_recipe_find(recipe_name);
            if (recipe && recipe->execute(art, NULL) == 0)
                (*success)++;
            else
                (*failed)++;

            lst_destroy(art);
        } else {
            (*failed)++;
        }
    }

    DIR *d = opendir(dir);
    if (!d) return;

    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        if (should_skip(ent->d_name)) continue;

        char child[LST_MAX_PATH];
        snprintf(child, sizeof(child), "%s/%s", dir, ent->d_name);

        struct stat st;
        if (stat(child, &st) == 0 && S_ISDIR(st.st_mode))
            batch_walk(child, recipe_name, depth + 1, found, success, failed);
    }
    closedir(d);
}

static int cmd_batch(const char *root_dir, const char *recipe_name) {
    printf("Batch: %s (recipe: %s)\n", root_dir, recipe_name);

    const lst_recipe_t *recipe = lst_recipe_find(recipe_name);
    if (!recipe) {
        fprintf(stderr, "Recipe not found: %s\n", recipe_name);
        return 1;
    }

    int found = 0, success = 0, failed = 0;
    batch_walk(root_dir, recipe_name, 0, &found, &success, &failed);

    printf("\n");
    for (int i = 0; i < 70; i++) putchar('=');
    printf("\nBATCH COMPLETE\n");
    printf("  Projects found:  %d\n", found);
    printf("  Succeeded:       %d\n", success);
    printf("  Failed:          %d\n", failed);
    for (int i = 0; i < 70; i++) putchar('=');
    printf("\n");

    return (failed > 0) ? 1 : 0;
}

static int cmd_list_recipes(void) {
    lst_recipe_t recipes[LST_MAX_RECIPES];
    int count = lst_recipe_list(recipes, LST_MAX_RECIPES);

    printf("Registered recipes: %d\n", count);
    for (int i = 0; i < count; i++)
        printf("  %-30s v%u  %s\n", recipes[i].name, recipes[i].version, recipes[i].description);

    return 0;
}

static int cmd_store_list(void) {
    char names[256][LST_MAX_NAME];
    int count = lst_store_list(DEFAULT_STORE, names, 256);

    printf("Stored artifacts: %d\n", count);
    for (int i = 0; i < count; i++)
        printf("  %s\n", names[i]);

    return 0;
}

/* --------------------------------------------------------------------------
 * Main
 * -------------------------------------------------------------------------- */

static void usage(const char *prog) {
    fprintf(stderr,
        "Usage:\n"
        "  %s build <project_path>           Build and store LST artifact\n"
        "  %s run <recipe> <project_path>    Build + run recipe on project\n"
        "  %s batch <root_dir> <recipe>      Run recipe across all projects\n"
        "  %s list                           List registered recipes\n"
        "  %s store list                     List stored artifacts\n"
        "  %s seal <file_path>               Seal file for production\n"
        "  %s verify <file_path>             Verify seal integrity\n"
        "  %s amend <file_path> <text>       Amend a sealed file\n",
        prog, prog, prog, prog, prog, prog, prog, prog);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        usage(argv[0]);
        return 1;
    }

    register_all_recipes();

    const char *cmd = argv[1];

    if (strcmp(cmd, "build") == 0 && argc >= 3)
        return cmd_build(argv[2]);

    if (strcmp(cmd, "run") == 0 && argc >= 4)
        return cmd_run(argv[2], argv[3]);

    if (strcmp(cmd, "batch") == 0 && argc >= 4)
        return cmd_batch(argv[2], argv[3]);

    if (strcmp(cmd, "list") == 0)
        return cmd_list_recipes();

    if (strcmp(cmd, "store") == 0 && argc >= 3 && strcmp(argv[2], "list") == 0)
        return cmd_store_list();

    if (strcmp(cmd, "seal") == 0 && argc >= 3)
        return lst_seal(argv[2]);

    if (strcmp(cmd, "verify") == 0 && argc >= 3)
        return lst_seal_verify(argv[2]);

    if (strcmp(cmd, "amend") == 0 && argc >= 4)
        return lst_seal_amend(argv[2], argv[3]);

    usage(argv[0]);
    return 1;
}
