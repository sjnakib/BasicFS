#ifndef VSFS_H
#define VSFS_H

#include <stddef.h>
#include <stdint.h>

#define VSFS_MAGIC 0xD34Du
#define VSFS_BLOCK_SIZE 4096u
#define VSFS_TOTAL_BLOCKS 64u
#define VSFS_INODE_BITMAP_BLOCK 1u
#define VSFS_DATA_BITMAP_BLOCK 2u
#define VSFS_INODE_TABLE_BLOCK 3u
#define VSFS_FIRST_DATA_BLOCK 8u
#define VSFS_INODE_SIZE 256u
#define VSFS_INODE_COUNT 80u
#define VSFS_DIRECT_POINTERS 12u
#define VSFS_POINTERS_PER_BLOCK (VSFS_BLOCK_SIZE / (uint32_t)sizeof(uint32_t))
#define VSFS_IMAGE_SIZE (VSFS_BLOCK_SIZE * VSFS_TOTAL_BLOCKS)
#define VSFS_SUPERBLOCK_RESERVED 4060u
#define VSFS_INODE_RESERVED 156u

typedef struct {
    uint16_t magic;
    uint16_t padding;
    uint32_t block_size;
    uint32_t total_blocks;
    uint32_t inode_bitmap_block;
    uint32_t data_bitmap_block;
    uint32_t inode_table_block;
    uint32_t first_data_block;
    uint32_t inode_size;
    uint32_t inode_count;
    uint8_t reserved[VSFS_SUPERBLOCK_RESERVED];
} vsfs_superblock;

typedef struct {
    uint32_t mode;
    uint32_t uid;
    uint32_t gid;
    uint32_t size;
    uint32_t atime;
    uint32_t ctime;
    uint32_t mtime;
    uint32_t dtime;
    uint32_t links;
    uint32_t blocks;
    uint32_t direct[VSFS_DIRECT_POINTERS];
    uint32_t single_indirect;
    uint32_t double_indirect;
    uint32_t triple_indirect;
    uint8_t reserved[VSFS_INODE_RESERVED];
} vsfs_inode;

_Static_assert(sizeof(vsfs_superblock) == VSFS_BLOCK_SIZE,
               "superblock must fit in one block");
_Static_assert(sizeof(vsfs_inode) == VSFS_INODE_SIZE,
               "inode must be 256 bytes");

typedef enum {
    VSFS_ISSUE_SUPERBLOCK = 0,
    VSFS_ISSUE_INODE_BITMAP,
    VSFS_ISSUE_DATA_BITMAP,
    VSFS_ISSUE_DUPLICATE,
    VSFS_ISSUE_BAD_BLOCK,
    VSFS_ISSUE_COUNT
} vsfs_issue_type;

typedef struct {
    vsfs_issue_type type;
    char message[192];
} vsfs_issue;

typedef struct {
    vsfs_issue *items;
    size_t count;
    size_t capacity;
} vsfs_issues;

typedef struct {
    uint8_t *data;
    size_t size;
} vsfs_image;

typedef struct {
    vsfs_image image;
    vsfs_superblock superblock;
    vsfs_issues issues;
    uint16_t ref_count[VSFS_TOTAL_BLOCKS];
    int16_t first_inode[VSFS_TOTAL_BLOCKS];
    uint8_t claimed[VSFS_TOTAL_BLOCKS];
} vsfs_check;

/* image.c */
int image_load(vsfs_image *image, const char *path, int writable);
int image_save(const vsfs_image *image, const char *path);
void image_close(vsfs_image *image);

/* superblock.c */
void superblock_read(const vsfs_image *image, vsfs_superblock *superblock);
void superblock_check(const vsfs_superblock *superblock, vsfs_issues *issues);
void superblock_repair(vsfs_image *image);

/* bitmap.c */
int bitmap_get(const uint8_t *bitmap, uint32_t index);
void bitmap_set(uint8_t *bitmap, uint32_t index, int value);
void bitmap_clear_all(uint8_t *bitmap);
uint8_t *block_data(vsfs_image *image, uint32_t block);
const uint8_t *const_block_data(const vsfs_image *image, uint32_t block);

/* inode.c */
void inode_read(const vsfs_image *image, uint32_t index, vsfs_inode *inode);
void inode_write(vsfs_image *image, uint32_t index, const vsfs_inode *inode);
int inode_is_valid(const vsfs_inode *inode);
int inode_pointer_is_valid(uint32_t block);
void inode_scan_refs(vsfs_check *check, uint32_t index,
                     const vsfs_inode *inode);

/* check.c */
void issues_init(vsfs_issues *issues);
void issues_free(vsfs_issues *issues);
void issue_add(vsfs_issues *issues, vsfs_issue_type type, const char *format,
               ...);
void check_init(vsfs_check *check, vsfs_image *image);
void check_run(vsfs_check *check);
void check_free(vsfs_check *check);

/* report.c */
void report_print(const vsfs_check *check);
size_t report_issue_count(const vsfs_check *check, vsfs_issue_type type);

/* repair.c */
int repair_image(vsfs_check *check);

#endif
