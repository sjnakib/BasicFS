# Design

BasicFS checks one VSFS image at a time. The image is a flat 256 KB file, so the whole disk fits in memory and every check can use direct block offsets.

## On-disk layout

| Block | Use |
| --- | --- |
| 0 | Superblock |
| 1 | Inode bitmap |
| 2 | Data bitmap |
| 3-7 | Inode table |
| 8-63 | Data blocks |

The block size is 4096 bytes. The image has 64 blocks, 80 inodes, and 56 data blocks.

### Superblock

The superblock starts with a little-endian magic value, `0xd34d`, followed by two padding bytes. The remaining fields are 32-bit values:

- block size
- total block count
- inode bitmap block
- data bitmap block
- inode table block
- first data block
- inode size
- inode count

The rest of the block is reserved. The checker validates the eight numeric fields and reports each mismatch separately.

### Inode

Each inode is 256 bytes. The first ten fields are `mode`, `uid`, `gid`, `size`, `atime`, `ctime`, `mtime`, `dtime`, `links`, and `blocks`. The next fifteen fields are block pointers: twelve direct pointers, one single indirect pointer, one double indirect pointer, and one triple indirect pointer. The final 156 bytes are reserved.

An inode is treated as allocated when `links > 0` and `dtime == 0`. The other metadata fields are not validated because the assignment only defines consistency rules for allocation, bitmaps, and block ownership.

## Check flow

1. Read the superblock and compare every numeric field with the fixed VSFS values.
2. Compare each inode with its bit in the inode bitmap. Also check bits beyond the 80 inode slots.
3. Scan every allocated inode for block references:
   - direct pointers
   - single indirect entries
   - double indirect entries
   - triple indirect entries
4. Record each data block reference, its first owner, and the total reference count.
5. Compare the reference set with the data bitmap. A marked block must be referenced, and a referenced block must be marked.
6. Report duplicate references and block numbers outside the data range.

Indirect blocks are data blocks too, so the indirect block itself is counted as a reference. Its entries are then read as 32-bit block numbers.

## Damaged images

The checker uses the fixed layout above even when the superblock is damaged. This keeps the inode and bitmap checks meaningful for images with a bad magic value or a broken block pointer. The superblock findings still tell the user which fields need repair.
