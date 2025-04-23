#include "vsfs.h"

int repair_image(vsfs_check *check)
{
    if (check->issues.count == 0) {
        return 0;
    }

    superblock_repair(&check->image);
    superblock_read(&check->image, &check->superblock);

    return 1;
}
