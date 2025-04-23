#include "vsfs.h"

#include <stdio.h>
#include <string.h>

void superblock_read(const vsfs_image *image, vsfs_superblock *superblock)
{
    memcpy(superblock, image->data, sizeof(*superblock));
}

void superblock_check(const vsfs_superblock *superblock, vsfs_issues *issues)
{
    if (superblock->magic != VSFS_MAGIC) {
        issue_add(issues, VSFS_ISSUE_SUPERBLOCK,
                  "superblock magic is 0x%04x, expected 0xd34d",
                  superblock->magic);
    }
    if (superblock->block_size != VSFS_BLOCK_SIZE) {
        issue_add(issues, VSFS_ISSUE_SUPERBLOCK,
                  "superblock block size is %u, expected %u",
                  superblock->block_size, (unsigned)VSFS_BLOCK_SIZE);
    }
    if (superblock->total_blocks != VSFS_TOTAL_BLOCKS) {
        issue_add(issues, VSFS_ISSUE_SUPERBLOCK,
                  "superblock total blocks is %u, expected %u",
                  superblock->total_blocks, (unsigned)VSFS_TOTAL_BLOCKS);
    }
    if (superblock->inode_bitmap_block != VSFS_INODE_BITMAP_BLOCK) {
        issue_add(issues, VSFS_ISSUE_SUPERBLOCK,
                  "superblock inode bitmap block is %u, expected %u",
                  superblock->inode_bitmap_block,
                  (unsigned)VSFS_INODE_BITMAP_BLOCK);
    }
    if (superblock->data_bitmap_block != VSFS_DATA_BITMAP_BLOCK) {
        issue_add(issues, VSFS_ISSUE_SUPERBLOCK,
                  "superblock data bitmap block is %u, expected %u",
                  superblock->data_bitmap_block,
                  (unsigned)VSFS_DATA_BITMAP_BLOCK);
    }
    if (superblock->inode_table_block != VSFS_INODE_TABLE_BLOCK) {
        issue_add(issues, VSFS_ISSUE_SUPERBLOCK,
                  "superblock inode table block is %u, expected %u",
                  superblock->inode_table_block,
                  (unsigned)VSFS_INODE_TABLE_BLOCK);
    }
    if (superblock->first_data_block != VSFS_FIRST_DATA_BLOCK) {
        issue_add(issues, VSFS_ISSUE_SUPERBLOCK,
                  "superblock first data block is %u, expected %u",
                  superblock->first_data_block,
                  (unsigned)VSFS_FIRST_DATA_BLOCK);
    }
    if (superblock->inode_size != VSFS_INODE_SIZE) {
        issue_add(issues, VSFS_ISSUE_SUPERBLOCK,
                  "superblock inode size is %u, expected %u",
                  superblock->inode_size, (unsigned)VSFS_INODE_SIZE);
    }
    if (superblock->inode_count != VSFS_INODE_COUNT) {
        issue_add(issues, VSFS_ISSUE_SUPERBLOCK,
                  "superblock inode count is %u, expected %u",
                  superblock->inode_count, (unsigned)VSFS_INODE_COUNT);
    }
}

void superblock_repair(vsfs_image *image)
{
    vsfs_superblock *superblock = (vsfs_superblock *)image->data;

    superblock->magic = VSFS_MAGIC;
    superblock->padding = 0;
    superblock->block_size = VSFS_BLOCK_SIZE;
    superblock->total_blocks = VSFS_TOTAL_BLOCKS;
    superblock->inode_bitmap_block = VSFS_INODE_BITMAP_BLOCK;
    superblock->data_bitmap_block = VSFS_DATA_BITMAP_BLOCK;
    superblock->inode_table_block = VSFS_INODE_TABLE_BLOCK;
    superblock->first_data_block = VSFS_FIRST_DATA_BLOCK;
    superblock->inode_size = VSFS_INODE_SIZE;
    superblock->inode_count = VSFS_INODE_COUNT;
}
