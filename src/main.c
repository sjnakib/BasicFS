#include "vsfs.h"

#include <stdio.h>
#include <string.h>

static void usage(FILE *stream, const char *program)
{
    fprintf(stream, "Usage: %s [--fix] <image>\n", program);
    fprintf(stream, "\n");
    fprintf(stream, "Check a VSFS image for consistency.\n");
    fprintf(stream, "\n");
    fprintf(stream, "Options:\n");
    fprintf(stream, "  -f, --fix   repair the image in place\n");
    fprintf(stream, "  -h, --help  show this help\n");
}

static int load_and_check(vsfs_check *check, const char *path, int writable)
{
    vsfs_image image;

    if (image_load(&image, path, writable) != 0) {
        return -1;
    }

    check_init(check, &image);
    check_run(check);
    return 0;
}

int main(int argc, char **argv)
{
    const char *path = NULL;
    const char *program = argc > 0 ? argv[0] : "vsfsck";
    int fix = 0;
    int index;
    int result;
    vsfs_check check;

    for (index = 1; index < argc; index++) {
        if (strcmp(argv[index], "-h") == 0 ||
            strcmp(argv[index], "--help") == 0) {
            usage(stdout, program);
            return 0;
        } else if (strcmp(argv[index], "-f") == 0 ||
                   strcmp(argv[index], "--fix") == 0) {
            fix = 1;
        } else if (argv[index][0] == '-') {
            fprintf(stderr, "vsfsck: unknown option %s\n", argv[index]);
            usage(stderr, program);
            return 2;
        } else if (path == NULL) {
            path = argv[index];
        } else {
            fprintf(stderr, "vsfsck: only one image can be checked at a time\n");
            usage(stderr, program);
            return 2;
        }
    }

    if (path == NULL) {
        usage(stderr, program);
        return 2;
    }

    if (load_and_check(&check, path, fix) != 0) {
        return 2;
    }

    printf("VSFS check: %s\n\n", path);
    report_print(&check);

    if (fix && check.issues.count > 0) {
        if (repair_image(&check) == 0 || image_save(&check.image, path) != 0) {
            check_free(&check);
            return 2;
        }

        check_free(&check);
        if (load_and_check(&check, path, 0) != 0) {
            return 2;
        }

        printf("\nAfter repair: %s\n\n", path);
        report_print(&check);
        result = check.issues.count == 0 ? 0 : 1;
    } else if (fix) {
        printf("No repairs needed.\n");
        result = 0;
    } else {
        result = check.issues.count == 0 ? 0 : 1;
    }

    check_free(&check);
    return result;
}
