#include "vsfs.h"

#include <string.h>

int bitmap_get(const uint8_t *bitmap, uint32_t index)
{
    return (bitmap[index / 8u] >> (index % 8u)) & 1u;
}

void bitmap_set(uint8_t *bitmap, uint32_t index, int value)
{
    uint8_t mask = (uint8_t)(1u << (index % 8u));

    if (value != 0) {
        bitmap[index / 8u] |= mask;
    } else {
        bitmap[index / 8u] &= (uint8_t)~mask;
    }
}

void bitmap_clear_all(uint8_t *bitmap)
{
    memset(bitmap, 0, VSFS_BLOCK_SIZE);
}

uint8_t *block_data(vsfs_image *image, uint32_t block)
{
    if (block >= VSFS_TOTAL_BLOCKS) {
        return NULL;
    }
    return image->data + (size_t)block * VSFS_BLOCK_SIZE;
}

const uint8_t *const_block_data(const vsfs_image *image, uint32_t block)
{
    if (block >= VSFS_TOTAL_BLOCKS) {
        return NULL;
    }
    return image->data + (size_t)block * VSFS_BLOCK_SIZE;
}
