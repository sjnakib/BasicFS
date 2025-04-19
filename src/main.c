#include "vsfs.h"

#include <stdio.h>

static void usage(FILE *stream, const char *program)
{
    fprintf(stream, "Usage: %s <image>\n", program);
    fprintf(stream, "\n");
    fprintf(stream, "Check a VSFS image for consistency.\n");
}

int main(int argc, char **argv)
{
    const char *program = argc > 0 ? argv[0] : "vsfsck";
    vsfs_image image;

    if (argc != 2) {
        usage(stderr, program);
        return 2;
    }

    if (image_load(&image, argv[1], 0) != 0) {
        return 2;
    }

    printf("loaded %zu bytes from %s\n", image.size, argv[1]);
    image_close(&image);
    return 0;
}
