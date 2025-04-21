#include "vsfs.h"

#include <stdint.h>
#include <string.h>

void inode_read(const vsfs_image *image, uint32_t index, vsfs_inode *inode)
{
    size_t offset = (size_t)VSFS_INODE_TABLE_BLOCK * VSFS_BLOCK_SIZE +
                    (size_t)index * VSFS_INODE_SIZE;

    memcpy(inode, image->data + offset, sizeof(*inode));
}

void inode_write(vsfs_image *image, uint32_t index, const vsfs_inode *inode)
{
    size_t offset = (size_t)VSFS_INODE_TABLE_BLOCK * VSFS_BLOCK_SIZE +
                    (size_t)index * VSFS_INODE_SIZE;

    memcpy(image->data + offset, inode, sizeof(*inode));
}

int inode_is_valid(const vsfs_inode *inode)
{
    return inode->links > 0 && inode->dtime == 0;
}

int inode_pointer_is_valid(uint32_t block)
{
    return block >= VSFS_FIRST_DATA_BLOCK && block < VSFS_TOTAL_BLOCKS;
}

static void add_block_ref(vsfs_check *check, uint32_t block, uint32_t inode)
{
    if (block == 0) {
        return;
    }

    if (!inode_pointer_is_valid(block)) {
        issue_add(&check->issues, VSFS_ISSUE_BAD_BLOCK,
                  "inode %u references block %u, outside the data range "
                  "%u-%u",
                  inode, block, (unsigned)VSFS_FIRST_DATA_BLOCK,
                  (unsigned)(VSFS_TOTAL_BLOCKS - 1));
        return;
    }

    if (check->ref_count[block] == 0) {
        check->first_inode[block] = (int16_t)inode;
    }
    if (check->ref_count[block] < UINT16_MAX) {
        check->ref_count[block]++;
    }
}

void inode_scan_refs(vsfs_check *check, uint32_t index,
                     const vsfs_inode *inode)
{
    uint32_t slot;

    for (slot = 0; slot < VSFS_DIRECT_POINTERS; slot++) {
        add_block_ref(check, inode->direct[slot], index);
    }
}
