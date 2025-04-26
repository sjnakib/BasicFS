#!/usr/bin/env python3
"""Tests for the vsfsck command-line tool."""

from __future__ import annotations

import hashlib
import shutil
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


def add_inode(
    image: bytearray,
    index: int,
    *,
    direct: list[int] | None = None,
    single: int = 0,
    double: int = 0,
    triple: int = 0,
    links: int = 1,
    dtime: int = 0,
) -> None:
    offset = INODE_TABLE_BLOCK * BLOCK_SIZE + index * INODE_SIZE
    direct_values = (direct or []) + [0] * (12 - len(direct or []))
    used = sum(1 for value in direct_values if value)
    struct.pack_into(
        "<10I", image, offset, 0, 0, 0, 0, 0, 0, 0, dtime, links, used
    )
    struct.pack_into(
        "<15I", image, offset + 40, *direct_values, single, double, triple
    )


def set_bit(image: bytearray, block: int, index: int) -> None:
    image[block * BLOCK_SIZE + index // 8] |= 1 << (index % 8)


def write_image(directory: Path, name: str, image: bytes | bytearray) -> Path:
    path = directory / name
    path.write_bytes(image)
    return path

def write_u32(image: bytearray, block: int, index: int, value: int) -> None:
    struct.pack_into("<I", image, block * BLOCK_SIZE + index * 4, value)


def read_u32(image: bytes, block: int, index: int) -> int:
    return struct.unpack_from("<I", image, block * BLOCK_SIZE + index * 4)[0]


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
            result.returncode == 2 and "too small" in result.stderr,
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

        trailing = write_image(
            Path(directory), "trailing.img", bytes(clean) + b"\0" * 6 + b"junk"
        )
        result = run(str(trailing))
        expect(
            "trailing bytes are ignored",
            result.returncode == 0 and "No issues found." in result.stdout,
            result.stdout,
        )


def test_synthetic_blocks() -> None:
    with tempfile.TemporaryDirectory() as directory:
        work = Path(directory)

        duplicate = blank_image()
        add_inode(duplicate, 0, direct=[8])
        add_inode(duplicate, 1, direct=[8])
        set_bit(duplicate, 1, 0)
        set_bit(duplicate, 1, 1)
        set_bit(duplicate, 2, 8)
        path = write_image(work, "duplicate.img", duplicate)
        result = run(str(path))
        expect(
            "duplicate block is reported",
            result.returncode == 1
            and "block 8 is referenced by inode 0 and inode 1" in result.stdout,
            result.stdout,
        )

        bad_block = blank_image()
        add_inode(bad_block, 0, direct=[80])
        set_bit(bad_block, 1, 0)
        path = write_image(work, "bad-block.img", bad_block)
        result = run(str(path))
        expect(
            "bad block is reported",
            result.returncode == 1
            and "inode 0 references block 80" in result.stdout,
            result.stdout,
        )

        indirect = blank_image()
        add_inode(indirect, 0, single=8)
        write_u32(indirect, 8, 0, 9)
        set_bit(indirect, 1, 0)
        set_bit(indirect, 2, 8)
        set_bit(indirect, 2, 9)
        path = write_image(work, "indirect.img", indirect)
        expect(
            "single indirect blocks are followed",
            run(str(path)).returncode == 0,
            "indirect image reported an issue",
        )

        double = blank_image()
        add_inode(double, 0, double=8)
        write_u32(double, 8, 0, 9)
        write_u32(double, 9, 0, 10)
        set_bit(double, 1, 0)
        for block in (8, 9, 10):
            set_bit(double, 2, block)
        path = write_image(work, "double.img", double)
        expect(
            "double indirect blocks are followed",
            run(str(path)).returncode == 0,
            "double indirect image reported an issue",
        )

        triple = blank_image()
        add_inode(triple, 0, triple=8)
        write_u32(triple, 8, 0, 9)
        write_u32(triple, 9, 0, 10)
        write_u32(triple, 10, 0, 11)
        set_bit(triple, 1, 0)
        for block in (8, 9, 10, 11):
            set_bit(triple, 2, block)
        path = write_image(work, "triple.img", triple)
        expect(
            "triple indirect blocks are followed",
            run(str(path)).returncode == 0,
            "triple indirect image reported an issue",
        )

        bad_indirect = blank_image()
        add_inode(bad_indirect, 0, single=8)
        write_u32(bad_indirect, 8, 0, 80)
        set_bit(bad_indirect, 1, 0)
        set_bit(bad_indirect, 2, 8)
        path = write_image(work, "bad-indirect.img", bad_indirect)
        result = run(str(path))
        expect(
            "bad indirect entry is reported",
            result.returncode == 1
            and "inode 0 references block 80" in result.stdout,
            result.stdout,
        )


def test_superblock_repair() -> None:
    with tempfile.TemporaryDirectory() as directory:
        target = Path(directory) / "set-04.img"
        shutil.copy2(IMAGE_DIR / "set-04.img", target)

        fixed = run("--fix", str(target))
        expect(
            "superblock repair exits cleanly",
            fixed.returncode == 0,
            f"exit={fixed.returncode}",
        )
        recheck = run(str(target))
        expect(
            "superblock repair is clean on recheck",
            recheck.returncode == 0 and "No issues found." in recheck.stdout,
            recheck.stdout,
        )


def test_inode_bitmap_repair() -> None:
    for name in ("set-02.img", "set-10.img", "set-12.img", "set-14.img"):
        with tempfile.TemporaryDirectory() as directory:
            target = Path(directory) / name
            shutil.copy2(IMAGE_DIR / name, target)

            fixed = run("--fix", str(target))
            recheck = run(str(target))
            expect(
                f"{name} inode bitmap repair exits cleanly",
                fixed.returncode == 0,
                f"exit={fixed.returncode}",
            )
            expect(
                f"{name} is clean after inode bitmap repair",
                recheck.returncode == 0
                and "No issues found." in recheck.stdout,
                recheck.stdout,
            )


def test_data_bitmap_repair() -> None:
    for name in (
        "set-03.img",
        "set-08.img",
        "set-09.img",
        "set-11.img",
        "set-13.img",
    ):
        with tempfile.TemporaryDirectory() as directory:
            target = Path(directory) / name
            shutil.copy2(IMAGE_DIR / name, target)

            fixed = run("--fix", str(target))
            recheck = run(str(target))
            expect(
                f"{name} data bitmap repair exits cleanly",
                fixed.returncode == 0,
                f"exit={fixed.returncode}",
            )
            expect(
                f"{name} is clean after data bitmap repair",
                recheck.returncode == 0
                and "No issues found." in recheck.stdout,
                recheck.stdout,
            )


def test_pointer_repair() -> None:
    with tempfile.TemporaryDirectory() as directory:
        target = Path(directory) / "set-01.img"
        shutil.copy2(IMAGE_DIR / "set-01.img", target)

        fixed = run("--fix", str(target))
        recheck = run(str(target))
        expect(
            "set-01 pointer repair exits cleanly",
            fixed.returncode == 0,
            f"exit={fixed.returncode}",
        )
        expect(
            "set-01 is clean after pointer repair",
            recheck.returncode == 0 and "No issues found." in recheck.stdout,
            recheck.stdout,
        )

        bad_block = blank_image()
        add_inode(bad_block, 0, direct=[80])
        set_bit(bad_block, 1, 0)
        bad_path = write_image(Path(directory), "bad-block.img", bad_block)
        fixed = run("--fix", str(bad_path))
        expect(
            "bad block repair clears the pointer",
            fixed.returncode == 0
            and read_u32(bad_path.read_bytes(), INODE_TABLE_BLOCK, 10) == 0,
            fixed.stdout,
        )
        expect(
            "bad block repair clears the block count",
            read_u32(bad_path.read_bytes(), INODE_TABLE_BLOCK, 9) == 0,
            "inode 0 still counts a block",
        )
        expect(
            "bad block image is clean after repair",
            run(str(bad_path)).returncode == 0,
            "recheck failed",
        )

        bad_indirect = blank_image()
        add_inode(bad_indirect, 0, single=8)
        write_u32(bad_indirect, 8, 0, 80)
        set_bit(bad_indirect, 1, 0)
        set_bit(bad_indirect, 2, 8)
        bad_path = write_image(
            Path(directory), "bad-indirect.img", bad_indirect
        )
        fixed = run("--fix", str(bad_path))
        expect(
            "bad indirect entry is cleared",
            fixed.returncode == 0
            and read_u32(bad_path.read_bytes(), 8, 0) == 0,
            fixed.stdout,
        )
        expect(
            "bad indirect image is clean after repair",
            run(str(bad_path)).returncode == 0,
            "recheck failed",
        )


def test_duplicate_repair() -> None:
    with tempfile.TemporaryDirectory() as directory:
        duplicate = blank_image()
        add_inode(duplicate, 0, direct=[8])
        add_inode(duplicate, 1, direct=[8])
        set_bit(duplicate, 1, 0)
        set_bit(duplicate, 1, 1)
        set_bit(duplicate, 2, 8)
        target = write_image(Path(directory), "duplicate.img", duplicate)

        fixed = run("--fix", str(target))
        expect(
            "duplicate repair exits cleanly",
            fixed.returncode == 0,
            f"exit={fixed.returncode}",
        )
        expect(
            "duplicate repair clears the later pointer",
            read_u32(target.read_bytes(), INODE_TABLE_BLOCK, 1 * 64 + 10) == 0,
            "inode 1 still points at block 8",
        )
        expect(
            "duplicate repair clears the block count",
            read_u32(target.read_bytes(), INODE_TABLE_BLOCK, 1 * 64 + 9) == 0,
            "inode 1 still counts a block",
        )
        recheck = run(str(target))
        expect(
            "duplicate image is clean after repair",
            recheck.returncode == 0 and "No issues found." in recheck.stdout,
            recheck.stdout,
        )


def test_repair_all_provided() -> None:
    with tempfile.TemporaryDirectory() as directory:
        work = Path(directory)
        for name in PROVIDED_CASES:
            source = IMAGE_DIR / name
            target = work / name
            shutil.copy2(source, target)

            source_hash = hashlib.sha256(source.read_bytes()).hexdigest()
            fixed = run("--fix", str(target))
            expect(
                f"{name} full repair exits cleanly",
                fixed.returncode == 0,
                f"exit={fixed.returncode}",
            )
            recheck = run(str(target))
            expect(
                f"{name} is clean after full repair",
                recheck.returncode == 0
                and "No issues found." in recheck.stdout,
                recheck.stdout,
            )
            expect(
                f"{name} source image is unchanged",
                hashlib.sha256(source.read_bytes()).hexdigest() == source_hash,
                "a test modified the source image",
            )


def test_repair_idempotent() -> None:
    with tempfile.TemporaryDirectory() as directory:
        target = Path(directory) / "set-01.img"
        shutil.copy2(IMAGE_DIR / "set-01.img", target)

        first = run("--fix", str(target))
        before = hashlib.sha256(target.read_bytes()).hexdigest()
        second = run("--fix", str(target))
        after = hashlib.sha256(target.read_bytes()).hexdigest()

        expect(
            "second repair finds nothing",
            first.returncode == 0
            and second.returncode == 0
            and "No repairs needed." in second.stdout,
            second.stdout,
        )
        expect(
            "second repair keeps the image bytes",
            before == after,
            "image changed on the second repair",
        )


def main() -> int:
    if not BINARY.exists():
        print(f"missing binary: {BINARY}", file=sys.stderr)
        return 2

    test_usage()
    test_missing_and_truncated()
    test_provided_images()
    test_clean_image()
    test_synthetic_blocks()
    test_superblock_repair()
    test_inode_bitmap_repair()
    test_data_bitmap_repair()
    test_pointer_repair()
    test_duplicate_repair()
    test_repair_all_provided()
    test_repair_idempotent()

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
