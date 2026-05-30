/*
 * runner.c — Recipe Registration and Execution
 *
 * Recipes are compiled C functions with a fixed signature.
 * They operate on LST artifacts, never on source projects directly.
 * The runner handles registration, lookup, and execution lifecycle.
 */

#include "lst.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* --------------------------------------------------------------------------
 * Recipe registry — static array, no heap allocation for registry itself
 * -------------------------------------------------------------------------- */

static lst_recipe_t g_recipes[LST_MAX_RECIPES];
static int          g_recipe_count = 0;

int lst_recipe_register(const lst_recipe_t *recipe) {
    if (!recipe || !recipe->execute) return -1;
    if (g_recipe_count >= LST_MAX_RECIPES) {
        fprintf(stderr, "runner: recipe registry full (%d)\n", LST_MAX_RECIPES);
        return -1;
    }

    /* Check for duplicate */
    for (int i = 0; i < g_recipe_count; i++) {
        if (strcmp(g_recipes[i].name, recipe->name) == 0) {
            /* Replace existing — allows version upgrades */
            g_recipes[i] = *recipe;
            return 0;
        }
    }

    g_recipes[g_recipe_count++] = *recipe;
    return 0;
}

const lst_recipe_t *lst_recipe_find(const char *name) {
    if (!name) return NULL;
    for (int i = 0; i < g_recipe_count; i++) {
        if (strcmp(g_recipes[i].name, name) == 0)
            return &g_recipes[i];
    }
    return NULL;
}

int lst_recipe_run(const char *recipe_name, const char *project_name,
                   const char *store_dir, const char *output_dir) {
    const lst_recipe_t *recipe = lst_recipe_find(recipe_name);
    if (!recipe) {
        fprintf(stderr, "runner: recipe '%s' not found\n", recipe_name);
        return -1;
    }

    /* Load artifact from store */
    lst_artifact_t *art = lst_store_read(store_dir, project_name);
    if (!art) {
        fprintf(stderr, "runner: no artifact for '%s' in %s\n", project_name, store_dir);
        return -1;
    }

    /* Verify artifact integrity */
    if (art->magic != 0x4C535400) {
        fprintf(stderr, "runner: corrupt artifact for '%s'\n", project_name);
        lst_destroy(art);
        return -1;
    }

    /* Execute recipe */
    int result = recipe->execute(art, output_dir);

    lst_destroy(art);
    return result;
}

int lst_recipe_list(lst_recipe_t *out, int max) {
    int count = (g_recipe_count < max) ? g_recipe_count : max;
    if (count > 0 && out)
        memcpy(out, g_recipes, (size_t)count * sizeof(lst_recipe_t));
    return count;
}
