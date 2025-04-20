#include "vsfs.h"

#include <stdio.h>
#include <string.h>

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
    vsfs_check check;
    int result;

    if (argc != 2) {
        usage(stderr, program);
        return 2;
    }

    if (image_load(&image, argv[1], 0) != 0) {
        return 2;
    }

    check_init(&check, &image);
    check_run(&check);

    printf("VSFS check: %s\n\n", argv[1]);
    report_print(&check);
    result = check.issues.count == 0 ? 0 : 1;

    check_free(&check);
    return result;
}
