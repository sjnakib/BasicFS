#!/usr/bin/env python3
"""Tests for the vsfsck command-line tool."""

from __future__ import annotations

import struct
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
BINARY = ROOT / "vsfsck"
IMAGE_DIR = ROOT / "tests" / "images"

BLOCK_SIZE = 4096
TOTAL_BLOCKS = 64
INODE_TABLE_BLOCK = 3
INODE_SIZE = 256
MAGIC = 0xD34D

PROVIDED_CASES = {
    "set-01.img": (
        2,
        [
            "inode 0 is valid but not marked in the inode bitmap",
            "inode 0 references block 80",
        ],
    ),
    "set-02.img": (
        5,
        [
            "inode 10 is valid but not marked in the inode bitmap",
            "inode 28 is marked in the inode bitmap but is not valid",
            "inode 31 is marked in the inode bitmap but is not valid",
        ],
    ),
    "set-03.img": (
        2,
        [
            "inode 13 is marked in the inode bitmap but is not valid",
            "data block 13 is marked in the data bitmap but not referenced",
        ],
    ),
    "set-04.img": (
        2,
        [
            "superblock magic is 0x3d4d, expected 0xd34d",
            "superblock first data block is 160, expected 8",
        ],
    ),
    "set-05.img": (
        2,
        [
            "superblock magic is 0xd39d, expected 0xd34d",
            "superblock inode table block is 160, expected 3",
        ],
    ),
    "set-06.img": (
        2,
        [
            "superblock inode table block is 160, expected 3",
            "superblock inode count is 96, expected 80",
        ],
    ),
    "set-07.img": (
        2,
        [
            "superblock total blocks is 80, expected 64",
            "superblock inode count is 96, expected 80",
        ],
    ),
    "set-08.img": (
        8,
        [
            "inode 8 is marked in the inode bitmap but is not valid",
            "data block 8 is marked in the data bitmap but not referenced",
            "data block 26 is marked in the data bitmap but not referenced",
        ],
    ),
    "set-09.img": (
        5,
        [
            "data block 40 is marked in the data bitmap but not referenced",
            "data bitmap marks block 64, outside the data range 8-63",
            "data bitmap marks block 66, outside the data range 8-63",
        ],
    ),
    "set-10.img": (
        4,
        [
            "inode 65 is marked in the inode bitmap but is not valid",
            "inode 74 is marked in the inode bitmap but is not valid",
        ],
    ),
    "set-11.img": (
        12,
        [
            "superblock block size is 1048628, expected 4096",
            "superblock inode count is 20480, expected 80",
            "data bitmap marks block 1, outside the data range 8-63",
            "data bitmap marks block 6, outside the data range 8-63",
        ],
    ),
    "set-12.img": (
        3,
        [
            "inode 72 is marked in the inode bitmap but is not valid",
            "inode 74 is marked in the inode bitmap but is not valid",
        ],
    ),
    "set-13.img": (
        4,
        [
            "data block 8 is marked in the data bitmap but not referenced",
            "data block 14 is marked in the data bitmap but not referenced",
        ],
    ),
    "set-14.img": (
        16,
        [
            "inode 25 is marked in the inode bitmap but is not valid",
            "inode bitmap bit 88 is outside the valid range 0-79",
            "inode bitmap bit 318 is outside the valid range 0-79",
        ],
    ),
}

failures: list[str] = []
passed = 0


def run(*args: str) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [str(BINARY), *args],
        capture_output=True,
        text=True,
        check=False,
    )


def expect(name: str, condition: bool, detail: str = "") -> None:
    global passed

    if condition:
        passed += 1
        print(f"PASS {name}")
    else:
        failures.append(f"{name}: {detail}")
        print(f"FAIL {name}: {detail}")


def blank_image() -> bytearray:
    image = bytearray(BLOCK_SIZE * TOTAL_BLOCKS)
    struct.pack_into(
        "<H2x8I",
        image,
        0,
        MAGIC,
        BLOCK_SIZE,
        TOTAL_BLOCKS,
        1,
        2,
        INODE_TABLE_BLOCK,
        8,
        INODE_SIZE,
        80,
    )
    return image


def add_inode(image: bytearray, index: int, *, direct: list[int] | None = None) -> None:
    offset = INODE_TABLE_BLOCK * BLOCK_SIZE + index * INODE_SIZE
    direct_values = (direct or []) + [0] * (12 - len(direct or []))
    used = sum(1 for value in direct_values if value)
    struct.pack_into("<10I", image, offset, 0, 0, 0, 0, 0, 0, 0, 0, 1, used)
    struct.pack_into("<15I", image, offset + 40, *direct_values, 0, 0, 0)


def set_bit(image: bytearray, block: int, index: int) -> None:
    image[block * BLOCK_SIZE + index // 8] |= 1 << (index % 8)


def write_image(directory: Path, name: str, image: bytes | bytearray) -> Path:
    path = directory / name
    path.write_bytes(image)
    return path


def test_usage() -> None:
    result = run()
    expect(
        "no arguments returns usage",
        result.returncode == 2 and "Usage:" in result.stderr,
        f"exit={result.returncode}",
    )

    result = run("--help")
    expect(
        "help exits cleanly",
        result.returncode == 0 and "Usage:" in result.stdout,
        f"exit={result.returncode}",
    )


def test_missing_and_truncated() -> None:
    result = run("/tmp/vsfsck-image-that-does-not-exist")
    expect(
        "missing image is rejected",
        result.returncode == 2 and "cannot open" in result.stderr,
        f"exit={result.returncode}",
    )

    with tempfile.TemporaryDirectory() as directory:
        path = write_image(Path(directory), "short.img", b"\0" * 100)
        result = run(str(path))
        expect(
            "truncated image is rejected",
            result.returncode == 2,
            f"exit={result.returncode}",
        )


def test_provided_images() -> None:
    for name, (count, snippets) in PROVIDED_CASES.items():
        path = IMAGE_DIR / name
        result = run(str(path))
        expect(
            f"{name} exit code",
            result.returncode == 1,
            f"exit={result.returncode}",
        )
        expect(
            f"{name} issue count",
            f"{count} issues found." in result.stdout,
            result.stdout,
        )
        for snippet in snippets:
            expect(
                f"{name} reports {snippet!r}",
                snippet in result.stdout,
                result.stdout,
            )

def test_clean_image() -> None:
    with tempfile.TemporaryDirectory() as directory:
        clean = blank_image()
        add_inode(clean, 0, direct=[8])
        set_bit(clean, 1, 0)
        set_bit(clean, 2, 8)
        path = write_image(Path(directory), "clean.img", clean)

        result = run(str(path))
        expect(
            "clean image passes",
            result.returncode == 0 and "No issues found." in result.stdout,
            result.stdout,
        )


def main() -> int:
    if not BINARY.exists():
        print(f"missing binary: {BINARY}", file=sys.stderr)
        return 2

    test_usage()
    test_missing_and_truncated()
    test_provided_images()
    test_clean_image()

    print()
    if failures:
        print(f"{len(failures)} failed, {passed} passed")
        for failure in failures:
            print(f"  {failure}")
        return 1

    print(f"All {passed} checks passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
