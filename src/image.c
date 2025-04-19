#include "vsfs.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int image_load(vsfs_image *image, const char *path, int writable)
{
    FILE *file;
    long size;
    uint8_t *data;

    image->data = NULL;
    image->size = 0;

    file = fopen(path, writable ? "r+b" : "rb");
    if (file == NULL) {
        fprintf(stderr, "vsfsck: cannot open %s: %s\n", path, strerror(errno));
        return -1;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fprintf(stderr, "vsfsck: cannot seek %s: %s\n", path, strerror(errno));
        fclose(file);
        return -1;
    }

    size = ftell(file);
    if (size < 0) {
        fprintf(stderr, "vsfsck: cannot measure %s: %s\n", path,
                strerror(errno));
        fclose(file);
        return -1;
    }

    data = malloc(VSFS_IMAGE_SIZE);
    if (data == NULL) {
        fprintf(stderr, "vsfsck: out of memory\n");
        fclose(file);
        return -1;
    }

    if (fseek(file, 0, SEEK_SET) != 0) {
        fprintf(stderr, "vsfsck: cannot rewind %s: %s\n", path,
                strerror(errno));
        free(data);
        fclose(file);
        return -1;
    }

    if (fread(data, 1, VSFS_IMAGE_SIZE, file) != VSFS_IMAGE_SIZE) {
        fprintf(stderr, "vsfsck: cannot read %s: %s\n", path,
                ferror(file) ? strerror(errno) : "short read");
        free(data);
        fclose(file);
        return -1;
    }

    fclose(file);
    image->data = data;
    image->size = VSFS_IMAGE_SIZE;
    return 0;
}

int image_save(const vsfs_image *image, const char *path)
{
    FILE *file;
    size_t written;

    file = fopen(path, "r+b");
    if (file == NULL) {
        fprintf(stderr, "vsfsck: cannot write %s: %s\n", path,
                strerror(errno));
        return -1;
    }

    written = fwrite(image->data, 1, image->size, file);
    if (written != image->size) {
        fprintf(stderr, "vsfsck: cannot write %s: %s\n", path,
                strerror(errno));
        fclose(file);
        return -1;
    }

    if (fclose(file) != 0) {
        fprintf(stderr, "vsfsck: cannot close %s: %s\n", path,
                strerror(errno));
        return -1;
    }

    return 0;
}

void image_close(vsfs_image *image)
{
    free(image->data);
    image->data = NULL;
    image->size = 0;
}
