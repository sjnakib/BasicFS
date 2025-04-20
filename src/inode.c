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
