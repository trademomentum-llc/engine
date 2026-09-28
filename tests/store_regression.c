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
    assert(unlink(path) == 0);
    assert(symlink("/etc/passwd", path) == 0);
    assert(lst_store_read(dir, "sample") == NULL);
    assert(unlink(path) == 0);
    assert(rmdir(dir) == 0);
    free(art);
    puts("store regression: pass");
    return 0;
}
