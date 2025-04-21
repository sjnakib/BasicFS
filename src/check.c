#include "vsfs.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void issues_init(vsfs_issues *issues)
{
    issues->items = NULL;
    issues->count = 0;
    issues->capacity = 0;
}

void issues_free(vsfs_issues *issues)
{
    free(issues->items);
    issues->items = NULL;
    issues->count = 0;
    issues->capacity = 0;
}

static void issue_reserve(vsfs_issues *issues)
{
    size_t capacity;
    vsfs_issue *items;

    if (issues->count < issues->capacity) {
        return;
    }

    capacity = issues->capacity == 0 ? 16 : issues->capacity * 2;
    items = realloc(issues->items, capacity * sizeof(*items));
    if (items == NULL) {
        fprintf(stderr, "vsfsck: out of memory\n");
        exit(EXIT_FAILURE);
    }
    issues->items = items;
    issues->capacity = capacity;
}

void issue_add(vsfs_issues *issues, vsfs_issue_type type, const char *format,
               ...)
{
    va_list args;
    vsfs_issue *issue;

    issue_reserve(issues);
    issue = &issues->items[issues->count];
    issue->type = type;

    va_start(args, format);
    vsnprintf(issue->message, sizeof(issue->message), format, args);
    va_end(args);

    issues->count++;
}

void check_init(vsfs_check *check, vsfs_image *image)
{
    memset(check, 0, sizeof(*check));
    check->image = *image;
    image->data = NULL;
    image->size = 0;

    issues_init(&check->issues);
    superblock_read(&check->image, &check->superblock);
}

void check_free(vsfs_check *check)
{
    image_close(&check->image);
    issues_free(&check->issues);
}


static void check_inode_bitmap(vsfs_check *check)
{
    const uint8_t *bitmap =
        const_block_data(&check->image, VSFS_INODE_BITMAP_BLOCK);
    uint32_t index;

    for (index = 0; index < VSFS_INODE_COUNT; index++) {
        vsfs_inode inode;
        int valid;
        int marked;

        inode_read(&check->image, index, &inode);
        valid = inode_is_valid(&inode);
        marked = bitmap_get(bitmap, index);

        if (valid && !marked) {
            issue_add(&check->issues, VSFS_ISSUE_INODE_BITMAP,
                      "inode %u is valid but not marked in the inode bitmap",
                      index);
        } else if (!valid && marked) {
            issue_add(&check->issues, VSFS_ISSUE_INODE_BITMAP,
                      "inode %u is marked in the inode bitmap but is not "
                      "valid",
                      index);
        }
    }

    for (index = VSFS_INODE_COUNT; index < VSFS_BLOCK_SIZE * 8u; index++) {
        if (bitmap_get(bitmap, index)) {
            issue_add(&check->issues, VSFS_ISSUE_INODE_BITMAP,
                      "inode bitmap bit %u is outside the valid range 0-%u",
                      index, (unsigned)(VSFS_INODE_COUNT - 1));
        }
    }
}

void check_run(vsfs_check *check)
{
    superblock_check(&check->superblock, &check->issues);
    check_inode_bitmap(check);
}
