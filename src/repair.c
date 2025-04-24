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


static void rebuild_data_bitmap(vsfs_check *check)
{
    uint8_t *bitmap = block_data(&check->image, VSFS_DATA_BITMAP_BLOCK);
    uint32_t block;

    bitmap_clear_all(bitmap);
    for (block = VSFS_FIRST_DATA_BLOCK; block < VSFS_TOTAL_BLOCKS; block++) {
        if (check->ref_count[block] > 0) {
            bitmap_set(bitmap, block, 1);
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
    rebuild_data_bitmap(check);
    superblock_read(&check->image, &check->superblock);

    return 1;
}
