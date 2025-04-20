# BasicFS 🗂️

A small consistency checker for the VSFS disk image format used in CSE321 Project 2.

## About

VSFS is a teaching file system with a 256 KB image, 4 KB blocks, 80 inodes, and two bitmaps. This project reads such an image and reports where the superblock, the inode table, and the bitmaps disagree.

## What it will check

- Superblock fields and block pointers
- Inode bitmap entries
- Data bitmap entries
- Duplicate block references
- Block numbers outside the data area

## Build

```bash
make
./vsfsck tests/images/set-01.img
```

## Current state

The project layout and the image loader are in place. The individual checks are being added next.

## Author

Shafaat Jamil
