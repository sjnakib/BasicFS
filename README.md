# BasicFS 🗂️

A small consistency checker for the VSFS disk image format used in CSE321 Project 2.

## About

VSFS is a teaching file system with a 256 KB image, 4 KB blocks, 80 inodes, and two bitmaps. This project reads such an image and reports where the superblock, the inode table, and the bitmaps disagree.

## What it checks

- Superblock fields and block pointers
- Inode bitmap entries
- Data bitmap entries
- Duplicate block references
- Block numbers outside the data area

## Build

```bash
make
```

The binary is written to `./vsfsck`.

## Usage

```bash
./vsfsck tests/images/set-01.img
```

The checker prints one section for each group of checks. A clean section says `ok`; a failed section lists the problem.

```text
VSFS check: tests/images/set-01.img

Superblock
  ok

Inode bitmap
  error: inode 0 is valid but not marked in the inode bitmap

Data bitmap
  ok

Duplicate blocks
  ok

Bad blocks
  error: inode 0 references block 80, outside the data range 8-63

2 issues found.
```

Exit codes:

- `0`: the image is consistent
- `1`: the image has at least one finding
- `2`: a usage or file error occurred

Use `./vsfsck --help` for the short command summary.

## Author

Shafaat Jamil
