#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import re
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable


@dataclass(frozen=True)
class AddressId:
    name: str
    hash: int  # uint32_t stored as python int


def _default_library_path() -> Path:
    # tools/macos_resolve_addresses.py -> repo root
    root = Path(__file__).resolve().parents[1]
    return root / "src" / "Red" / "Addresses" / "Library.hpp"


def parse_library_hpp(path: Path) -> list[AddressId]:
    """
    Parse `src/Red/Addresses/Library.hpp` and return ordered (name, hash) pairs.

    Expected format:
        constexpr uint32_t Some_Name = 123456789;
    """
    text = path.read_text(encoding="utf-8", errors="replace")

    rx = re.compile(
        r"^\s*constexpr\s+uint32_t\s+(?P<name>[A-Za-z0-9_]+)\s*=\s*(?P<val>\d+)\s*;\s*(?://.*)?$",
        re.MULTILINE,
    )

    items: list[AddressId] = []
    for m in rx.finditer(text):
        items.append(AddressId(name=m.group("name"), hash=int(m.group("val"), 10)))

    if not items:
        raise RuntimeError(f"No 'constexpr uint32_t' entries found in {path}")

    # Sanity checks
    seen: set[int] = set()
    dup: list[int] = []
    for it in items:
        if it.hash in seen:
            dup.append(it.hash)
        seen.add(it.hash)
        if not (0 <= it.hash <= 0xFFFFFFFF):
            raise RuntimeError(f"Hash out of uint32 range for {it.name}: {it.hash}")

    if dup:
        raise RuntimeError(f"Duplicate hashes found in {path}: {dup[:10]} (total={len(dup)})")

    return items


def _emit_json(items: Iterable[AddressId]) -> str:
    return json.dumps([{"name": it.name, "hash": it.hash} for it in items], indent=2) + "\n"


def _emit_tsv(items: Iterable[AddressId]) -> str:
    lines = ["name\thash_dec\thash_hex"]
    for it in items:
        lines.append(f"{it.name}\t{it.hash}\t0x{it.hash:08X}")
    return "\n".join(lines) + "\n"


def main() -> int:
    ap = argparse.ArgumentParser(description="ArchiveXL macOS address resolver helper")
    ap.add_argument(
        "--library",
        type=Path,
        default=_default_library_path(),
        help="Path to ArchiveXL AddressLib header (default: <repo>/src/Red/Addresses/Library.hpp)",
    )
    ap.add_argument(
        "--format",
        choices=["json", "tsv"],
        default="tsv",
        help="Output format (default: tsv)",
    )
    ap.add_argument(
        "--out",
        type=Path,
        default=None,
        help="Optional output file path (default: stdout)",
    )
    args = ap.parse_args()

    items = parse_library_hpp(args.library)

    if args.format == "json":
        out = _emit_json(items)
    else:
        out = _emit_tsv(items)

    if args.out:
        args.out.write_text(out, encoding="utf-8")
    else:
        sys.stdout.write(out)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
