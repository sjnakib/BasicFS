#include "vsfs.h"

#include <string.h>

static int claim_block(vsfs_check *check, uint32_t block)
{
    if (!inode_pointer_is_valid(block) || check->claimed[block]) {
        return 0;
    }
    check->claimed[block] = 1;
    return 1;
}

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
        if (check->claimed[block]) {
            bitmap_set(bitmap, block, 1);
        }
    }
}

static void repair_indirect(vsfs_check *check, uint32_t *pointer, int depth)
{
    uint32_t block = *pointer;
    uint32_t *entries;
    uint32_t index;

    if (block == 0) {
        return;
    }

    if (!claim_block(check, block)) {
        *pointer = 0;
        return;
    }

    entries = (uint32_t *)block_data(&check->image, block);
    for (index = 0; index < VSFS_POINTERS_PER_BLOCK; index++) {
        uint32_t child = entries[index];

        if (child == 0) {
            continue;
        }
        if (depth == 1) {
            if (!claim_block(check, child)) {
                entries[index] = 0;
            }
        } else {
            repair_indirect(check, &entries[index], depth - 1);
        }
    }
}

static void repair_inodes(vsfs_check *check)
{
    uint32_t index;

    memset(check->claimed, 0, sizeof(check->claimed));

    for (index = 0; index < VSFS_INODE_COUNT; index++) {
        vsfs_inode inode;
        uint32_t slot;

        inode_read(&check->image, index, &inode);
        if (!inode_is_valid(&inode)) {
            continue;
        }

        for (slot = 0; slot < VSFS_DIRECT_POINTERS; slot++) {
            if (inode.direct[slot] != 0 &&
                !claim_block(check, inode.direct[slot])) {
                inode.direct[slot] = 0;
            }
        }

        repair_indirect(check, &inode.single_indirect, 1);
        repair_indirect(check, &inode.double_indirect, 2);
        repair_indirect(check, &inode.triple_indirect, 3);

        inode_write(&check->image, index, &inode);
    }
}

int repair_image(vsfs_check *check)
{
    if (check->issues.count == 0) {
        return 0;
    }

    superblock_repair(&check->image);
    repair_inodes(check);
    rebuild_inode_bitmap(check);
    rebuild_data_bitmap(check);
    superblock_read(&check->image, &check->superblock);

    return 1;
}
