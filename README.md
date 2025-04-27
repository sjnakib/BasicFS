# BasicFS 🗂️

A small consistency checker for the VSFS disk image format used in CSE321 Project 2.

[![Stars](https://img.shields.io/github/stars/sjnakib/BasicFS?style=for-the-badge&logo=github&color=yellow)](https://github.com/sjnakib/BasicFS/stargazers)
![Forks](https://img.shields.io/github/forks/sjnakib/BasicFS?style=for-the-badge&logo=github)
![Last commit](https://img.shields.io/github/last-commit/sjnakib/BasicFS?style=for-the-badge&color=green)
![Top language](https://img.shields.io/github/languages/top/sjnakib/BasicFS?style=for-the-badge)

## About

VSFS is a teaching file system with a 256 KB image, 4 KB blocks, 80 inodes, and two bitmaps. This project reads such an image and reports where the superblock, the inode table, and the bitmaps disagree.

## What it checks

- Superblock fields and block pointers
- Inode bitmap entries
- Data bitmap entries
- Duplicate block references
- Block numbers outside the data area

## Tech stack

![C](https://img.shields.io/badge/C-00599C?style=for-the-badge&logo=c&logoColor=white)
![GNU Make](https://img.shields.io/badge/GNU_Make-064F8C?style=for-the-badge&logo=gnu&logoColor=white)
![Python](https://img.shields.io/badge/Python-3776AB?style=for-the-badge&logo=python&logoColor=white)
![Git](https://img.shields.io/badge/Git-F05032?style=for-the-badge&logo=git&logoColor=white)
![Linux](https://img.shields.io/badge/Linux-FCC624?style=for-the-badge&logo=linux&logoColor=black)

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

## Repair

Work on a copy. `--fix` changes the image in place.

```bash
cp tests/images/set-01.img /tmp/set-01.img
./vsfsck --fix /tmp/set-01.img
./vsfsck /tmp/set-01.img
```

Repair mode:

- writes the expected superblock fields
- rebuilds the inode bitmap from allocated inodes
- rebuilds the data bitmap from referenced blocks
- clears block pointers outside the data area
- keeps the first reference to a duplicated block
- updates each inode block count

The command prints the original findings, writes the repaired image, then runs the check again. A clean second report gives exit code `0`.

## Project structure

```text
.
├── include/vsfs.h
├── src/
│   ├── main.c
│   ├── image.c
│   ├── superblock.c
│   ├── bitmap.c
│   ├── inode.c
│   ├── check.c
│   ├── report.c
│   └── repair.c
├── tests/
│   ├── run_tests.py
│   └── images/
├── docs/
│   ├── project-02.pdf
│   ├── fs-blocks-structure.png
│   ├── design.md
│   └── test-sets.md
├── Makefile
└── README.md
```

## Tests

```bash
make test
```

The suite runs the checker on all fourteen supplied images, repairs each one on a copy, and rechecks the result. It also generates clean, duplicate, bad-block, indirect, short, and trailing-byte fixtures in a temporary directory. The current suite contains 149 checks.

For a clean rebuild followed by the full suite:

```bash
make check
```

The C sources compile with `-Wall -Wextra -Wpedantic` and no warnings. The repair tests never modify the images under `tests/images/`.

## Documentation

- [Design notes](docs/design.md)
- [Test image summary](docs/test-sets.md)

## Author

Shafaat Jamil

## License

This was a course project for CSE321. No license file is included.
