#define _POSIX_C_SOURCE 200809L
#include "lst.h"
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int main(void) {
    char dir[] = "/tmp/engine-store-test-XXXXXX";
    assert(mkdtemp(dir));
    lst_artifact_t *art = calloc(1, sizeof(*art));
    assert(art);
    memcpy(art->project_name, "sample", 7);
    art->magic = 0x4C535400;
    assert(lst_store_write(art, dir) == 0);
    lst_artifact_t *roundtrip = lst_store_read(dir, "sample");
    assert(roundtrip && roundtrip->magic == art->magic);
    free(roundtrip);

    char path[512];
    assert(snprintf(path, sizeof(path), "%s/sample.lst", dir) > 0);
    assert(unlink(path) == 0);
    assert(mkfifo(path, 0600) == 0);
    assert(lst_store_read(dir, "sample") == NULL);
    /* Write must reject a FIFO leaf promptly instead of blocking. */
    assert(lst_store_write(art, dir) == -1);
    assert(unlink(path) == 0);
    assert(symlink("/etc/passwd", path) == 0);
    assert(lst_store_read(dir, "sample") == NULL);
    assert(unlink(path) == 0);

    /* Self-review (partial failure / concurrent writers): artifacts are
     * published by rename, so a rewrite replaces the whole file and leaves
     * no temporary behind (checked by the final rmdir). */
    assert(lst_store_write(art, dir) == 0);
    lst_artifact_t *changed = calloc(1, sizeof(*changed));
    assert(changed);
    memcpy(changed->project_name, "sample", 7);
    changed->magic = 0x4C535400;
    changed->project_path[0] = 0x5A;
    struct stat before, after;
    assert(stat(path, &before) == 0);
    assert(lst_store_write(changed, dir) == 0);
    assert(stat(path, &after) == 0);
    assert(before.st_ino != after.st_ino);
    lst_artifact_t *replaced = lst_store_read(dir, "sample");
    assert(replaced && replaced->project_path[0] == 0x5A);
    free(replaced);
    free(changed);
    assert(unlink(path) == 0);

    /* A store_dir that is itself a symlink must keep working
     * (PRRT_kwDOSsdSEM6ti3ka). */
    char link[512];
    assert(snprintf(link, sizeof(link), "%s-link", dir) > 0);
    assert(symlink(dir, link) == 0);
    assert(lst_store_write(art, link) == 0);
    lst_artifact_t *via_link = lst_store_read(link, "sample");
    assert(via_link && via_link->magic == art->magic);
    free(via_link);
    assert(unlink(link) == 0);
    assert(unlink(path) == 0);
    /* rmdir fails if any temporary was left behind. */
    assert(rmdir(dir) == 0);
    free(art);
    puts("store regression: pass");
    return 0;
}
