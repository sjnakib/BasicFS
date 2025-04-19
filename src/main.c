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

    if (argc != 2) {
        usage(stderr, program);
        return 2;
    }

    printf("vsfsck: would check %s\n", argv[1]);
    return 0;
}
