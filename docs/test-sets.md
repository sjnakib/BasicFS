# Test images

The `tests/images/` directory contains the fourteen images supplied with the assignment. The counts below are the findings reported by `./vsfsck` before any repair.

| Image | Findings | Main problems |
| --- | --- | --- |
| `set-01.img` | 2 | Inode 0 is valid but not marked; inode 0 points at block 80 |
| `set-02.img` | 5 | Inode 10 is not marked; inode bits 28-31 are stale |
| `set-03.img` | 2 | Inode bit 13 is stale; data block 13 is marked but unreferenced |
| `set-04.img` | 2 | Bad magic `0x3d4d`; first data block is 160 |
| `set-05.img` | 2 | Bad magic `0xd39d`; inode table block is 160 |
| `set-06.img` | 2 | Inode table block is 160; inode count is 96 |
| `set-07.img` | 2 | Total block count is 80; inode count is 96 |
| `set-08.img` | 8 | Inode bits 8, 9, and 35 are stale; data bits 8, 11, 24, 25, and 26 are unreferenced |
| `set-09.img` | 5 | Data blocks 40 and 41 are unreferenced; bits 64-66 are outside the data area |
| `set-10.img` | 4 | Inode bits 65, 68, 69, and 74 are stale |
| `set-11.img` | 12 | All eight superblock fields are wrong; data bitmap bits 1, 2, 4, and 6 are outside the data area |
| `set-12.img` | 3 | Inode bits 72, 73, and 74 are stale |
| `set-13.img` | 4 | Data blocks 8, 9, 13, and 14 are unreferenced |
| `set-14.img` | 16 | Inode bits 25, 26, 28, and 30 are stale; twelve bits are outside the 0-79 inode range |

Two images contain unused inode slots with garbage fields: inode 16 in Set 9 and inode 32 in Set 13. Both have zero links, so the checker treats them as free and does not report the garbage.

## Generated fixtures

The provided images do not contain a clean file system or a duplicate block reference. The test runner creates both in a temporary directory:

- a clean image with one inode and one data block
- a duplicate image where two inodes point at block 8
- a bad-block image with a pointer to block 80
- single, double, and triple indirect chains
- a short image and an image with trailing bytes

The repair tests work on copies. The files under `tests/images/` are never modified.
