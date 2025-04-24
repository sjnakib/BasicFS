#include "vsfs.h"


static void rebuild_inode_bitmap(vsfs_check *check)
{
    uint8_t *bitmap = block_data(&check->image, VSFS_INODE_BITMAP_BLOCK);
    uint32_t index;

    bitmap_clear_all(bitmap);
    for (index = 0; index < VSFS_INODE_COUNT; index++) {
        vsfs_inode inode;

        inode_read(&check->image, index, &inode);
        if (inode_is_valid(&inode)) {
            bitmap_set(bitmap, index, 1);
        }
    }
}

int repair_image(vsfs_check *check)
{
    if (check->issues.count == 0) {
        return 0;
    }

    superblock_repair(&check->image);
    rebuild_inode_bitmap(check);
    superblock_read(&check->image, &check->superblock);

    return 1;
}
